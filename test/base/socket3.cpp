#include <algorithm>
#include <array>
#include <cassert>
#include <csignal>
#include <forward_list>
#include <iostream>
#include <atomic>

#include <latch>
#include <mutex>
#include <print>
#include <string>
#include <thread>

#if defined(_MSC_VER)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <winsock2.h>
#include <mswsock.h>
#include <windows.h>
#include <ws2tcpip.h> // 包含 InetNtop 函数的头文件

#pragma comment(lib, "ws2_32.lib")

namespace HEAP
{

    template <typename T>
    struct SafeManager
    {
        struct Chunk;
        struct BlockHeader
        {
            Chunk *chunk;
            uint8_t index;
        };

        // 侵入式链表节点
        struct ListNode
        {
            ListNode *prev{};
            ListNode *next{};
        };
        enum ListType : std::uint8_t
        {
            FREE,
            TMP,
            WAIT_DELETE,
            NONE
        };
        // NOLINTBEGIN
        struct Chunk
        {
            ListNode list_node;

            static Chunk *from_list_node(ListNode *node) noexcept
            {
                return static_cast<Chunk *>(static_cast<void *>(node));
            }

            using status_type = std::uint64_t;
            using index_type = std::uint8_t;
            static constexpr size_t BIT_COUNT = sizeof(status_type) * CHAR_BIT;
            static constexpr status_type INIT_STATUS = ~status_type{0};
            static constexpr size_t BLOCK_ALIGN =
                (alignof(BlockHeader) > alignof(T)) ? alignof(BlockHeader) : alignof(T);

            status_type status{INIT_STATUS};
            ListType current_list{NONE};
            alignas(BLOCK_ALIGN) std::array<std::byte, (sizeof(T) + sizeof(BlockHeader)) *
                                                           BIT_COUNT> block;
            char padding[BIT_COUNT - (sizeof(ListNode) % BIT_COUNT)];

            static BlockHeader *get_header(T *ptr) noexcept
            {
                return reinterpret_cast<BlockHeader *>(
                    reinterpret_cast<std::byte *>(ptr) + sizeof(T));
            }

            [[nodiscard]] index_type find_free_slot() const noexcept
            {
                assert(status != 0);
                return std::countl_zero(status);
            }

            T *allocate(index_type index) noexcept
            {
                const status_type mask = status_type{1} << (BIT_COUNT - 1 - index);
                status &= ~mask;
                size_t offset = index * (sizeof(T) + sizeof(BlockHeader));
                new (&block[offset]) T();
                new (&block[offset + sizeof(T)]) BlockHeader{this, index};
                return reinterpret_cast<T *>(&block[offset]);
            }

            void deallocate(index_type index) noexcept
            {
                const status_type mask = status_type{1} << (BIT_COUNT - 1 - index);
                status |= mask;
            }

            [[nodiscard]] bool no_available() const noexcept
            {
                return status == 0;
            }
            [[nodiscard]] bool all_available() const noexcept
            {
                return status == INIT_STATUS;
            }
        };
        // NOLINTEND

        static_assert(offsetof(Chunk, list_node) == 0,
                      "list_node must be the first member");
        static_assert(std::is_standard_layout_v<Chunk>,
                      "Chunk must have standard layout");

        struct ChunkList
        {
            ListNode head;      // NOLINT
            ListType list_type; // NOLINT

            explicit ChunkList(ListType type = NONE) : list_type(type)
            {
                head.prev = &head;
                head.next = &head;
            }

            [[nodiscard]] bool empty() const noexcept
            {
                return head.next == &head;
            }

            void push_front(Chunk *chunk) noexcept // NOLINT
            {
                assert(chunk->current_list == NONE);
                ListNode *node = &chunk->list_node;
                node->prev = &head;
                node->next = head.next;
                head.next->prev = node;
                head.next = node;
                chunk->current_list = list_type;
            }

            void push_back(Chunk *chunk) noexcept // NOLINT
            {
                assert(chunk->current_list == ListType::NONE);
                ListNode *node = &chunk->list_node;
                node->next = &head;
                node->prev = head.prev;
                head.prev->next = node;
                head.prev = node;
                chunk->current_list = list_type;
            }

            [[nodiscard]] Chunk *header() const noexcept
            {
                if (empty())
                    return nullptr;
                return Chunk::from_list_node(head.next);
            }

            Chunk *pop_front() noexcept // NOLINT
            {
                if (empty())
                    return nullptr;
                ListNode *node = head.next;
                node->prev->next = node->next;
                node->next->prev = node->prev;
                node->prev = node->next = nullptr;
                Chunk *chunk = Chunk::from_list_node(node);
                chunk->current_list = ListType::NONE;
                return chunk;
            }

            [[nodiscard]] bool contains(const Chunk *chunk) const noexcept
            {
                return chunk->current_list == list_type;
            }

            Chunk *remove(Chunk *chunk) noexcept
            {
                assert(chunk->current_list == list_type);

                ListNode *node = &chunk->list_node;
                node->prev->next = node->next;
                node->next->prev = node->prev;
                node->prev = node->next = nullptr;

                chunk->current_list = ListType::NONE;
                return chunk;
            }
        };
        // NOLINTBEGIN
        ChunkList free_list{ListType::FREE};
        ChunkList tmp_list{ListType::TMP};
        ChunkList wait_delete_list{ListType::WAIT_DELETE};
        uint64_t chunk_count{0};
        std::mutex mutex;
        // NOLINTEND
        T *allocate()
        {
            std::lock_guard<std::mutex> lock(mutex);
            // free_list 为空则 new / 保证一直有值
            if (free_list.header() == nullptr)
            {
                auto *new_chunk = new Chunk();
                free_list.push_front(new_chunk);
                chunk_count++;
                return new_chunk->allocate(0);
            }

            auto *chunk = free_list.header();
            T *obj = chunk->allocate(chunk->find_free_slot());
            if (chunk->no_available())
            {
                tmp_list.push_front(free_list.pop_front());
            }
            return obj;
        }

        void deallocate(T *ptr) noexcept
        {
            std::lock_guard<std::mutex> lock(mutex);

            BlockHeader *header = Chunk::get_header(ptr);
            Chunk *chunk = header->chunk;
            ptr->~T();
            chunk->deallocate(header->index);

            switch (chunk->current_list)
            {
            case ListType::FREE:
                // free_list -> wait_delete_list
                if (chunk_count > 1 && chunk->all_available())
                    wait_delete_list.push_back(free_list.remove(chunk));
                break;
            case ListType::TMP:
                // tmp_list -> wait_delete_list || free_list
                if (chunk_count > 1 && chunk->all_available())
                    wait_delete_list.push_back(tmp_list.remove(chunk));
                else
                    free_list.push_back(tmp_list.remove(chunk));
                break;
            case ListType::WAIT_DELETE:
            case ListType::NONE:
                std::abort();
                break;
            default:
                break;
            }
            if (not wait_delete_list.empty())
            {
                chunk_count--;
                delete wait_delete_list.pop_front();
            }
        }
        SafeManager() = default;
        SafeManager(const SafeManager &) = delete;
        SafeManager &operator=(const SafeManager &) = delete;
        SafeManager(SafeManager &&) = delete;
        SafeManager &operator=(SafeManager &&) = delete;
        ~SafeManager() noexcept
        {
            auto delete_chunks = [](ChunkList &list) noexcept {
                while (auto *chunk = list.pop_front())
                {
                    delete chunk;
                }
            };

            delete_chunks(free_list);
            delete_chunks(tmp_list);
            delete_chunks(wait_delete_list);
        }
    };
}; // namespace HEAP

namespace WAIT
{
    static constexpr std::int8_t NUM_THREAD = 3; // NOLINT
    static std::latch &wait_net_done() noexcept  // NOLINT
    {
        static std::latch work_done{NUM_THREAD};
        return work_done;
    }
    static auto &registration_list() noexcept // NOLINT
    {
        static std::forward_list<HANDLE> list;
        return list;
    }
    static void register_wait_done(HANDLE iocp) noexcept // NOLINT
    {
        registration_list().push_front(iocp);
    }
    // 定义一个原子标志位，用于指示是否退出循环
    inline static std::atomic<bool> &get_exit_flag() noexcept // NOLINT
    {
        static std::atomic<bool> exit_flag(false);
        return exit_flag;
    }

    // 信号处理函数
    inline static void signal_handler(int signal) noexcept // NOLINT
    {
        if (signal == SIGINT) // 捕获Ctrl+C信号
        {
            std::println("\nReceived SIGINT (Ctrl+C). Exiting...\n");
            get_exit_flag().store(true, std::memory_order_release); // 设置标志位为true
            for (auto *iocp : registration_list())
                ::PostQueuedCompletionStatus(iocp, 0, 0, nullptr);
            WAIT::wait_net_done().wait();
        }
    }
}; // namespace WAIT

namespace NET
{

    enum IO_OP_TYPE : std::int8_t
    {
        IO_ACCEPT,
        IO_READ,
        IO_WRITE,
    };

    struct IOCP_IO_CONTEXT
    {
        static constexpr int MAX_BUFF_SIZE = 4096; // NOLINT
        static constexpr size_t MAX_ADDR_LEN =     // NOLINT
            sizeof(sockaddr_storage) + 16;         // 支持 IPv4 和 IPv6
        static constexpr auto MIN_ADDR_LENGTH = sizeof(sockaddr_in) + 16; // NOLINT

        OVERLAPPED overlapped;                 // NOLINT
        char buffer[MAX_BUFF_SIZE];            // NOLINT
        IO_OP_TYPE io_op_type;                 // NOLINT
        WSABUF wsabuf;                         // NOLINT
        SOCKET socket_accept;                  // NOLINT
        DWORD flags;                           // NOLINT
        IOCP_IO_CONTEXT *p_io_context_forward; // NOLINT
        sockaddr_storage local_addr;           // NOLINT
        sockaddr_storage remote_addr;          // NOLINT
        bool client_keep_alive;                // NOLINT

        // for read/write thread
        DWORD total_bytes; // NOLINT
        DWORD sent_bytes;  // NOLINT

        IOCP_IO_CONTEXT() noexcept
            : overlapped{}, buffer{}, io_op_type{IO_OP_TYPE::IO_ACCEPT}, wsabuf{},
              socket_accept{::WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0,
                                         WSA_FLAG_OVERLAPPED)},
              flags{}, p_io_context_forward{}, local_addr{}, remote_addr{},
              client_keep_alive{}, total_bytes{}, sent_bytes{}
        {
            if (socket_accept == INVALID_SOCKET)
            {
                std::cerr << "WSASocket failed: " << WSAGetLastError() << '\n';
                std::abort();
            }
            // {} 等价于 ZeroMemory， 0 初始化
            wsabuf.buf = buffer;
            wsabuf.len = MAX_BUFF_SIZE;
            assert(overlapped.Internal == 0);
            assert(wsabuf.buf[1] == 0);
            assert(p_io_context_forward == nullptr);
        }
        ~IOCP_IO_CONTEXT() noexcept
        {
            printBytes();
            ::closesocket(socket_accept);
        }
        IOCP_IO_CONTEXT(const IOCP_IO_CONTEXT &) = delete;
        IOCP_IO_CONTEXT(IOCP_IO_CONTEXT &&) = delete;
        IOCP_IO_CONTEXT &operator=(const IOCP_IO_CONTEXT &) = delete;
        IOCP_IO_CONTEXT &operator=(IOCP_IO_CONTEXT &&) = delete;

        void printBytes() const noexcept
        {
            std::cout << "Socket closed. Total bytes received: " << total_bytes
                      << ", sent: " << sent_bytes << '\n';
        }
    };

    static auto &global_io_ctx_pool() noexcept // NOLINT
    {
        static HEAP::SafeManager<IOCP_IO_CONTEXT> pool;
        return pool;
    }

}; // namespace NET

namespace IOCP
{
    static constexpr auto GetAddressString = // NOLINT
        [](const ::sockaddr_storage &addr) -> std::string {
        char ipStr[INET6_ADDRSTRLEN]; // NOLINT
        uint16_t port = 0;

        if (addr.ss_family == AF_INET)
        {
            auto *sin = reinterpret_cast<const sockaddr_in *>(&addr); // NOLINT
            inet_ntop(AF_INET, &sin->sin_addr, ipStr, sizeof(ipStr));
            port = ntohs(sin->sin_port);
        }
        else if (addr.ss_family == AF_INET6)
        {
            auto *sin6 = reinterpret_cast<const sockaddr_in6 *>(&addr); // NOLINT
            inet_ntop(AF_INET6, &sin6->sin6_addr, ipStr, sizeof(ipStr));
            port = ntohs(sin6->sin6_port);
        }
        else
        {
            return "Unknown Address Family";
        }
        return std::string(ipStr) + ":" + std::to_string(port);
    };

    static BOOL PostAsyncAccept(NET::IOCP_IO_CONTEXT &per_io_context, // NOLINT
                                SOCKET &listenSocket, LPFN_ACCEPTEX &pfnAcceptEx) noexcept
    {
        // 投递 AcceptEx 请求
        DWORD bytes_received = 0;
        BOOL result =
            pfnAcceptEx(listenSocket,                 // 监听套接字
                        per_io_context.socket_accept, // 接受套接字
                        per_io_context.buffer,        // 接收缓冲区
                        0, // 接收缓冲区中用于存储本地地址和远程地址的空间大小
                        NET::IOCP_IO_CONTEXT::MIN_ADDR_LENGTH,          // 本地地址长度
                        NET::IOCP_IO_CONTEXT::MIN_ADDR_LENGTH,          // 远程地址长度
                        &bytes_received,                                // 接收的字节数
                        reinterpret_cast<OVERLAPPED *>(&per_io_context) // NOLINT
            );

        if (FALSE == result)
        {
            int error = ::WSAGetLastError();
            if (error != ERROR_IO_PENDING)
            {
                std::cerr << "AcceptEx failed: " << error << '\n';
                return FALSE;
            }
        }

        return TRUE;
    }

    static BOOL PostAsyncRecv(NET::IOCP_IO_CONTEXT &per_io_context) noexcept // NOLINT
    {

        // 投递异步接收操作
        int result = ::WSARecv(per_io_context.socket_accept, // 目标 socket
                               &per_io_context.wsabuf,       // 接收缓冲区
                               1,                            // 缓冲区数量
                               nullptr,               // 接收的字节数（异步操作时不使用）
                               &per_io_context.flags, // 标志位
                               &per_io_context.overlapped, // 重叠结构
                               nullptr                     // 完成例程（IOCP 中不使用）
        );

        if (result == SOCKET_ERROR)
        {
            int error = WSAGetLastError();
            if (error != WSA_IO_PENDING)
            {
                std::cerr << "WSARecv failed: " << error << '\n';
                return FALSE;
            }
        }
        return TRUE;
    }

    static BOOL PostAsyncSend(NET::IOCP_IO_CONTEXT &per_io_context) noexcept // NOLINT
    {

        int result = WSASend(per_io_context.socket_accept, // 目标 socket
                             &per_io_context.wsabuf,       // 发送缓冲区
                             1,                            // 缓冲区数量
                             nullptr,              // 发送的字节数（异步操作时不使用）
                             per_io_context.flags, // 标志位
                             &per_io_context.overlapped, // 重叠结构
                             nullptr                     // 完成例程（IOCP 中不使用）
        );

        if (result == SOCKET_ERROR)
        {
            int error = WSAGetLastError();
            if (error != WSA_IO_PENDING)
            {
                std::cerr << "WSASend failed: " << error << '\n';
                return FALSE;
            }
        }

        return TRUE;
    }
}; // namespace IOCP

namespace HTTP
{

    static bool ParseKeepAlive(const std::string &request) noexcept // NOLINT
    {
        size_t pos = request.find("Connection: ");
        if (pos != std::string::npos)
        {
            pos += 12; // "Connection: " 的长度 // NOLINT
            size_t end = request.find("\r\n", pos);
            std::string connection = request.substr(pos, end - pos);
            return (connection.find("keep-alive") != std::string::npos);
        }
        return false; // 默认关闭（HTTP/1.1 默认 keep-alive，可根据协议版本调整）
    }

}; // namespace HTTP

namespace NET
{
    // NOLINTBEGIN
    static void handle_accept(IOCP_IO_CONTEXT *io_ctx) noexcept;
    static void handle_http_read(IOCP_IO_CONTEXT *io_ctx,
                                 DWORD bytesTransferred) noexcept;
    static void handle_http_write(IOCP_IO_CONTEXT *io_ctx,
                                  DWORD bytesTransferred) noexcept;
    // NOLINTEND

    class TcpReadWriteServer
    {
        HANDLE m_iocp{};
        std::thread::id m_id;
        std::size_t m_index{};

        void clear() noexcept
        {
            if (m_iocp != nullptr)
            {
                ::CloseHandle(m_iocp);
                m_iocp = nullptr;
            }
        }

      public:
        TcpReadWriteServer(const TcpReadWriteServer &) = delete;
        TcpReadWriteServer &operator=(const TcpReadWriteServer &) = delete;
        TcpReadWriteServer(TcpReadWriteServer &&) = default;
        TcpReadWriteServer &operator=(TcpReadWriteServer &&) = default;
        TcpReadWriteServer() noexcept
        {
            try
            {
                if (m_iocp =
                        ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
                    m_iocp == nullptr)
                    throw std::runtime_error("CreateIoCompletionPort error");
            }
            catch (const std::runtime_error &e)
            {
                clear();
                std::cerr << "Caught exception in anotherFunction: " << e.what() << '\n';
                std::abort();
            }
        }

        ~TcpReadWriteServer() noexcept
        {
            clear();
        }

        struct env
        {
            HANDLE iocp{};
            std::thread::id id;
            std::size_t index{};
        };
        [[nodiscard]] auto getEnv() const noexcept
        {
            return env{.iocp = m_iocp, .id = m_id, .index = m_index};
        }

        static void run(TcpReadWriteServer &server, std::size_t index) noexcept
        {
            WAIT::register_wait_done(server.m_iocp);
            std::println("TcpReadWriteServer::run start");
            server.m_id = std::this_thread::get_id();
            server.m_index = index;
            DWORD bytesTransferred{};
            ULONG_PTR completionKey{};
            IOCP_IO_CONTEXT *io_ctx{};
            BOOL ret{};
            while (true)
            {
                ret = ::GetQueuedCompletionStatus(
                    server.m_iocp, &bytesTransferred, &completionKey,
                    reinterpret_cast<OVERLAPPED **>(&io_ctx), INFINITE); // NOLINT
                if (WAIT::get_exit_flag().load(std::memory_order_relaxed))
                {
                    WAIT::wait_net_done().count_down();
                    break;
                }

                if (FALSE == ret)
                {
                    auto Error = ::GetLastError();
                    if (Error == WAIT_TIMEOUT)
                    {
                        std::cout << "GetQueuedCompletionStatus  WAIT_TIMEOUT" << '\n';
                        continue;
                    }
                    if (Error == ERROR_NETNAME_DELETED)
                    {
                        std::cout << "socket disconnection: " << io_ctx->socket_accept
                                  << '\n';
                        global_io_ctx_pool().deallocate(io_ctx);
                        continue;
                    }
                    std::cerr << "GetQueuedCompletionStatus error\n";
                    break;
                }

                // 在IO_READ处理逻辑中添加响应
                if (io_ctx->io_op_type == IO_OP_TYPE::IO_READ)
                {
                    ::NET::handle_http_read(io_ctx, bytesTransferred);
                    continue;
                } // 在main loop中增加IO_WRITE处理

                if (io_ctx->io_op_type == IO_OP_TYPE::IO_WRITE)
                {

                    NET::handle_http_write(io_ctx, bytesTransferred);
                    continue;
                }

                std::cout << "Error: IO_OP_TYPE\n";
                break;
            }
            std::println("TcpReadWriteServer::run end. exit_flag: {}",
                         WAIT::get_exit_flag().load(std::memory_order_relaxed));
        }
    };

    struct WorkerManager
    {
        static constexpr size_t worker_count = 2; // NOLINT

      public:
        WorkerManager() noexcept
        {
            for (size_t i = 0; i < WorkerManager::worker_count; ++i)
            {
                std::thread([&worker = workers[i], index = i]() { // NOLINT
                    TcpReadWriteServer::run(worker, index);
                })
                    .detach(); // 启动线程
            }
        }

        std::array<TcpReadWriteServer, worker_count> workers; // NOLINT
    };

    static auto &getOneWorker() noexcept // NOLINT
    {
        // 使用 RAII 封装类初始化 workers
        static WorkerManager workerManager;
        // 记录每个线程的负载
        static std::array<size_t, WorkerManager::worker_count> load = {0, 0};
        static constexpr size_t resetThreshold = 1000000; // 重置阈值 // NOLINT

        // 选择负载最低的线程
        size_t minIndex = 0;
        for (size_t i = 1; i < WorkerManager::worker_count; ++i)
        {
            if (load[i] < load[minIndex]) // NOLINT
            {
                minIndex = i;
            }
        }

        // 增加负载
        load[minIndex]++; // NOLINT

        // 定期重置负载计数
        if (load[minIndex] >= resetThreshold) // NOLINT
        {
            std::ranges::fill(load, 0);
        }

        return workerManager.workers[minIndex]; // NOLINT
    }

    class TcpListenServer
    {

        HANDLE m_iocp{};
        SOCKET m_listenSocket{};
        LPFN_ACCEPTEX m_pfnAcceptEx{};
        LPFN_GETACCEPTEXSOCKADDRS m_pfnGetAcceptExSockaddrs{};

        void clear() noexcept
        {
            if (m_listenSocket != INVALID_SOCKET)
            {
                ::closesocket(m_listenSocket);
                m_listenSocket = INVALID_SOCKET;
            }
            if (m_iocp != nullptr)
            {
                ::CloseHandle(m_iocp);
                m_iocp = nullptr;
            }
            ::WSACleanup();
        }
        void record_connect(IOCP_IO_CONTEXT *io_ctx) const noexcept
        {
            const auto &fnGetAcceptExSockaddr = m_pfnGetAcceptExSockaddrs;
            // 正常接受
            std::cout << "do IO_OP_TYPE::IO_ACCEPT " << '\n';
            // NOLINTNEXTLINE
            sockaddr *pLocalAddr = nullptr, *pRemoteAddr = nullptr; // NOLINT
            int localLen = 0, remoteLen = 0;                        // NOLINT
            // 调用GetAcceptExSockaddrs时使用正确的长度参数
            fnGetAcceptExSockaddr(io_ctx->buffer, 0,
                                  IOCP_IO_CONTEXT::MIN_ADDR_LENGTH, // 本地地址保留长度
                                  IOCP_IO_CONTEXT::MIN_ADDR_LENGTH, // 远程地址保留长度
                                  &pLocalAddr, &localLen, &pRemoteAddr, &remoteLen);

            ::setsockopt(io_ctx->socket_accept, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,
                         reinterpret_cast<char *>(&io_ctx->socket_accept), // NOLINT
                         sizeof(io_ctx->socket_accept));

            // 将获取的地址复制到结构体中
            memcpy(&io_ctx->local_addr, pLocalAddr, localLen);
            memcpy(&io_ctx->remote_addr, pRemoteAddr, remoteLen);
            std::println(
                "New connection accepted: Socket={}, Local Address={}, Remote Address={}",
                io_ctx->socket_accept,
                IOCP::GetAddressString(io_ctx->local_addr), // 本地IP地址
                IOCP::GetAddressString(io_ctx->remote_addr) // 远程IP地址
            );
        }

      public:
        TcpListenServer(const TcpListenServer &) = delete;
        TcpListenServer &operator=(const TcpListenServer &) = delete;
        TcpListenServer(TcpListenServer &&) = default;
        TcpListenServer &operator=(TcpListenServer &&) = default;

        explicit TcpListenServer(int port = 8080) noexcept // NOLINT
        {
            try
            {
                if (WSADATA wsaData; ::WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
                    throw std::runtime_error("WSAStartup error");
                if (m_iocp =
                        ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
                    m_iocp == nullptr)
                    throw std::runtime_error("CreateIoCompletionPort error");
                if (m_listenSocket = // NOLINTNEXTLINE
                    ::WSASocketW(AF_INET, SOCK_STREAM, 0, nullptr, 0,
                                 WSA_FLAG_OVERLAPPED);
                    m_listenSocket == INVALID_SOCKET)
                    throw std::runtime_error("WSASocket error");

                // Set IO to NBIO
                if (u_long u1 = 1;
                    ::ioctlsocket(m_listenSocket, FIONBIO, &u1) == SOCKET_ERROR)
                    throw std::runtime_error("ioctlsocket error");
                // m_listenFd close send/reciver
                int size = 0;
                ::setsockopt(m_listenSocket, SOL_SOCKET, SO_SNDBUF,
                             reinterpret_cast<const char *>(&size), // NOLINT
                             sizeof(size));                         // NOLINT
                ::setsockopt(m_listenSocket, SOL_SOCKET, SO_RCVBUF,
                             reinterpret_cast<const char *>(&size), // NOLINT
                             sizeof(size));                         // NOLINT

                // bind and listen
                sockaddr_in address{};
                address.sin_family = AF_INET;
                address.sin_port = ::htons(port);
                address.sin_addr.s_addr = INADDR_ANY;
                if (::bind(m_listenSocket,
                           reinterpret_cast<const sockaddr *>(&address), // NOLINT
                           sizeof(address)) != 0)
                    throw std::runtime_error("socket bind error");
                if (::listen(m_listenSocket, SOMAXCONN) != 0)
                    throw std::runtime_error("socket listen error");

                // bind m_listenFd and  m_completePort
                if (::CreateIoCompletionPort(
                        reinterpret_cast<HANDLE>(m_listenSocket), // NOLINT
                        m_iocp, 0, 0) == nullptr)
                    throw std::runtime_error("bind socket and completePort error");

                GUID GuidAcceptEx = WSAID_ACCEPTEX;
                DWORD dwBytes = 0;
                if (SOCKET_ERROR ==
                    ::WSAIoctl(m_listenSocket, SIO_GET_EXTENSION_FUNCTION_POINTER,
                               &GuidAcceptEx, sizeof(GuidAcceptEx),
                               (void *)&m_pfnAcceptEx, // NOLINT
                               sizeof(m_pfnAcceptEx), &dwBytes, nullptr, nullptr))
                    throw std::runtime_error("set m_pfnAcceptEx error");

                GUID guidGetAcceptExSockaddrs = WSAID_GETACCEPTEXSOCKADDRS;
                if (SOCKET_ERROR ==
                    WSAIoctl(m_listenSocket, SIO_GET_EXTENSION_FUNCTION_POINTER,
                             &guidGetAcceptExSockaddrs, sizeof(guidGetAcceptExSockaddrs),
                             &m_pfnGetAcceptExSockaddrs,
                             sizeof(m_pfnGetAcceptExSockaddrs), &dwBytes, nullptr,
                             nullptr))
                    throw std::runtime_error("set m_pfnGetAcceptExSockaddrs error");
            }
            catch (const std::runtime_error &e)
            {
                clear();
                std::cerr << "Caught exception in anotherFunction: " << e.what() << '\n';
                std::abort();
            }
        }
        ~TcpListenServer() noexcept
        {
            clear();
        }

        static void run(TcpListenServer &server) noexcept
        {
            WAIT::register_wait_done(server.m_iocp);
            // init worker
            getOneWorker();
            if (IOCP_IO_CONTEXT *ctx = global_io_ctx_pool().allocate();
                FALSE ==
                IOCP::PostAsyncAccept(*ctx, server.m_listenSocket, server.m_pfnAcceptEx))
                std::abort();

            std::println("TcpListenServer::run start");

            DWORD bytesTransferred{};
            ULONG_PTR completionKey{};
            IOCP_IO_CONTEXT *io_ctx{};
            BOOL ret{};
            while (true)
            {
                ret = ::GetQueuedCompletionStatus(
                    server.m_iocp, &bytesTransferred, &completionKey,
                    reinterpret_cast<OVERLAPPED **>(&io_ctx), INFINITE); // NOLINT

                if (WAIT::get_exit_flag().load(std::memory_order_relaxed))
                {
                    WAIT::wait_net_done().count_down();
                    break;
                }

                if (FALSE == ret)
                {
                    auto Error = ::GetLastError();
                    if (Error == WAIT_TIMEOUT)
                    {
                        std::cout << "GetQueuedCompletionStatus  WAIT_TIMEOUT" << '\n';
                        continue;
                    }
                    if (Error == ERROR_NETNAME_DELETED)
                    {
                        std::cout << "socket disconnection: " << io_ctx->socket_accept
                                  << '\n';
                        global_io_ctx_pool().deallocate(io_ctx);
                        continue;
                    }
                    std::cerr << "GetQueuedCompletionStatus error not handle\n";
                    std::abort();
                    break;
                }

                if (io_ctx->io_op_type == IO_OP_TYPE::IO_ACCEPT)
                {
                    server.record_connect(io_ctx);
                    NET::handle_accept(io_ctx);
                    if (io_ctx = global_io_ctx_pool().allocate();
                        FALSE == IOCP::PostAsyncAccept(*io_ctx, server.m_listenSocket,
                                                       server.m_pfnAcceptEx))
                        std::abort();
                    continue;
                }
                std::cerr << "io_op_type error\n";
                break;
            }
            std::println("TcpListenServer::run end. exit_flag: {}",
                         WAIT::get_exit_flag().load(std::memory_order_relaxed));
        }
    };
    // NOLINTNEXTLINE
    static void handle_accept(IOCP_IO_CONTEXT *io_ctx) noexcept
    {
        // (mcs): 转发线程
        // 2. 获取工作线程的IOCP句柄
        auto &worker = getOneWorker();
        HANDLE workerIOCP = worker.getEnv().iocp;

        // 3.
        // 将socket绑定到工作线程的IOCP，完成键必须为socket句柄或RW_IO_CONTEXT指针
        HANDLE hResult = ::CreateIoCompletionPort(
            (HANDLE)io_ctx->socket_accept, workerIOCP,
            static_cast<ULONG_PTR>(io_ctx->socket_accept), // 或 (ULONG_PTR)newRWCtx
            0);
        if (hResult != workerIOCP)
        {
            std::cerr << "Bind to worker IOCP failed: " << GetLastError() << "\n";
            global_io_ctx_pool().deallocate(io_ctx);
            return;
        }

        // 4. 直接投递异步读取操作（无需IO_TRANSFER）
        io_ctx->io_op_type = IO_OP_TYPE::IO_READ;
        io_ctx->flags = 0;
        if (FALSE == IOCP::PostAsyncRecv(*io_ctx))
        {
            std::cerr << "PostAsyncRecv failed: " << GetLastError() << "\n";
            global_io_ctx_pool().deallocate(io_ctx);
            return;
        }
    }
    // NOLINTNEXTLINE
    static void handle_http_read(IOCP_IO_CONTEXT *io_ctx, DWORD bytesTransferred) noexcept
    {
        std::cout << "do IO_OP_TYPE::IO_READ " << '\n';
        io_ctx->total_bytes += bytesTransferred; // 累加接收字节数

        if (bytesTransferred == 0) // TODO(mcs): 说明要断开连接？
        {
            std::cerr << "bytesTransferred == 0 error\n";
            global_io_ctx_pool().deallocate(io_ctx);
            return;
        }

        // 解析HTTP请求
        std::string request(io_ctx->buffer, bytesTransferred);
        std::cout << "Received data: \n" << request << '\n';

        bool clientKeepAlive = HTTP::ParseKeepAlive(request);
        io_ctx->client_keep_alive = clientKeepAlive; // 保存到上下文
        // 构造响应
        std::string responseBody = "Hello World!";
        std::string response = "HTTP/1.1 200 OK\r\n"
                               "Content-Type: text/plain\r\n"
                               "Content-Length: " +
                               std::to_string(responseBody.length()) +
                               "\r\n"
                               "Connection: " +
                               (clientKeepAlive ? "keep-alive" : "close") +
                               "\r\n"
                               "\r\n" // 头与正文的空行
                               + responseBody;

        memcpy(io_ctx->buffer, response.c_str(), response.size());
        io_ctx->wsabuf.buf = io_ctx->buffer;                      // 确保指针正确
        io_ctx->wsabuf.len = static_cast<ULONG>(response.size()); // 明确转换长度

        // 投递 IO_WRITE
        io_ctx->io_op_type = IO_OP_TYPE::IO_WRITE;
        io_ctx->flags = 0;
        if (FALSE == IOCP::PostAsyncSend(*io_ctx))
        {
            global_io_ctx_pool().deallocate(io_ctx);
            std::abort();
        }
    }
    // NOLINTNEXTLINE
    static void handle_http_write(IOCP_IO_CONTEXT *io_ctx,
                                  DWORD bytesTransferred) noexcept
    {
        std::cout << "do IO_OP_TYPE::IO_WRITE " << '\n';
        io_ctx->sent_bytes += bytesTransferred; // 累加发送字节数
        // 关闭连接
        if (io_ctx->client_keep_alive) // 使用上下文中的标志
        {
            io_ctx->flags = 0;
            io_ctx->io_op_type = IO_OP_TYPE::IO_READ;
            if (FALSE == IOCP::PostAsyncRecv(*io_ctx))
            {
                std::cout << "closesocket and delete pAcceptOv" << '\n';
                global_io_ctx_pool().deallocate(io_ctx);
            }
        }
        else
        {
            std::cout << "closesocket and delete pAcceptOv" << '\n';
            global_io_ctx_pool().deallocate(io_ctx);
        }
    }

}; // namespace NET

int main()
{
    (void)std::signal(SIGINT, WAIT::signal_handler);
    NET::TcpListenServer server{};
    std::jthread l([&server]() { NET::TcpListenServer::run(server); });

    return 0;
}

#else

inline static std::atomic<bool> &get_exit_flag() noexcept // NOLINT
{
    static std::atomic<bool> exit_flag(false);
    return exit_flag;
}

inline static void signal_handler(int signal) noexcept // NOLINT
{
    if (signal == SIGINT) // 捕获Ctrl+C信号
    {
        std::println("\nReceived SIGINT (Ctrl+C). Exiting...\n");
        get_exit_flag().store(true, std::memory_order_release); // 设置标志位为true
    }
};
int main()
{
    (void)std::signal(SIGINT, signal_handler);
    while (!get_exit_flag())
    {
        std::cout << "do ... \n";
    }
    std::cout << "main done\n";
    return 0;
}
#endif
