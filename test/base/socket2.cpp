#include <algorithm>
#include <array>
#include <cassert>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <atomic>

#include <new>
#include <string>
#include <thread>

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
        std::cout << "\nReceived SIGINT (Ctrl+C). Exiting...\n";
        get_exit_flag() = true; // 设置标志位为true
        exit(0);
    }
}

#if defined(_MSC_VER)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <winsock2.h>
#include <mswsock.h>
#include <windows.h>
#include <ws2tcpip.h> // 包含 InetNtop 函数的头文件

#pragma comment(lib, "ws2_32.lib")

namespace NET
{

    enum IO_OP_TYPE : std::int8_t
    {
        IO_INIT,
        IO_ACCEPT,
        IO_READ,
        IO_WRITE,
    };

    struct PER_IO_CONTEXT
    {
        static constexpr int MAX_BUFF_SIZE = 4096; // NOLINT
        static constexpr size_t MAX_ADDR_LEN =     // NOLINT
            sizeof(sockaddr_storage) + 16;         // 支持 IPv4 和 IPv6
        static constexpr auto MIN_ADDR_LENGTH = sizeof(sockaddr_in) + 16; // NOLINT

        OVERLAPPED overlapped;                // NOLINT
        char buffer[MAX_BUFF_SIZE];           // NOLINT
        IO_OP_TYPE io_op_type;                // NOLINT
        WSABUF wsabuf;                        // NOLINT
        SOCKET socket_accept;                 // NOLINT
        DWORD flags;                          // NOLINT
        PER_IO_CONTEXT *p_io_context_forward; // NOLINT
        sockaddr_storage local_addr;          // NOLINT
        sockaddr_storage remote_addr;         // NOLINT
        bool client_keep_alive;               // NOLINT

        PER_IO_CONTEXT() noexcept
            : overlapped{}, buffer{}, io_op_type{IO_OP_TYPE::IO_INIT}, wsabuf{},
              socket_accept{INVALID_SOCKET}, flags{}, p_io_context_forward{},
              local_addr{}, remote_addr{}, client_keep_alive{}
        {
            // {} 等价于 ZeroMemory， 0 初始化
            wsabuf.buf = buffer;
            wsabuf.len = MAX_BUFF_SIZE;
            assert(overlapped.Internal == 0);
            assert(wsabuf.buf[1] == 0);
            assert(p_io_context_forward == nullptr);
        }
    };

    // 当处理接收操作时，将bytesTransferred累加到total_bytes
    // 当处理发送操作时，将bytesTransferred累加到sent_bytes
    struct RW_IO_CONTEXT : public PER_IO_CONTEXT
    {
        std::thread::id thread_id; // NOLINT
        DWORD total_bytes;         // NOLINT
        DWORD sent_bytes;          // NOLINT
        RW_IO_CONTEXT()
            : thread_id(std::this_thread::get_id()), total_bytes{}, sent_bytes{}
        {
        }

        void printBytes() const noexcept
        {
            std::cout << "Socket closed. Total bytes received: " << total_bytes
                      << ", sent: " << sent_bytes << '\n';
        }

        RW_IO_CONTEXT &operator=(const PER_IO_CONTEXT &per_io_context)
        {
            if (this == &per_io_context)
                return *this;

            // 显式复制OVERLAPPED字段（避免内存错乱）
            memcpy(&this->overlapped, &per_io_context.overlapped, sizeof(OVERLAPPED));

            // 复制基本类型和简单成员
            io_op_type = per_io_context.io_op_type;
            socket_accept = per_io_context.socket_accept;
            flags = per_io_context.flags;
            local_addr = per_io_context.local_addr;
            remote_addr = per_io_context.remote_addr;
            client_keep_alive = per_io_context.client_keep_alive;

            // 深拷贝buffer内容
            std::memcpy(buffer, per_io_context.buffer, MAX_BUFF_SIZE);

            // 重新设置wsabuf指针指向自己的buffer
            wsabuf.buf = buffer;
            wsabuf.len = per_io_context.wsabuf.len;

            // 深拷贝p_io_context_forward
            if (per_io_context.p_io_context_forward != nullptr)
            {
                p_io_context_forward = new PER_IO_CONTEXT();
                *p_io_context_forward = *per_io_context.p_io_context_forward;
            }
            else
            {
                p_io_context_forward = nullptr;
            }

            // thread_id保持原值（符合原始设计意图）
            return *this;
        }
    };
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

    static BOOL PostAsyncAccept(NET::PER_IO_CONTEXT &per_io_context, // NOLINT
                                SOCKET &listenSocket, LPFN_ACCEPTEX &pfnAcceptEx) noexcept
    {
        // 投递 AcceptEx 请求
        DWORD bytes_received = 0;
        BOOL result =
            pfnAcceptEx(listenSocket,                 // 监听套接字
                        per_io_context.socket_accept, // 接受套接字
                        per_io_context.buffer,        // 接收缓冲区
                        0, // 接收缓冲区中用于存储本地地址和远程地址的空间大小
                        NET::PER_IO_CONTEXT::MIN_ADDR_LENGTH,           // 本地地址长度
                        NET::PER_IO_CONTEXT::MIN_ADDR_LENGTH,           // 远程地址长度
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

    static BOOL PostAsyncRecv(NET::PER_IO_CONTEXT &per_io_context) noexcept // NOLINT
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

    static BOOL PostAsyncSend(NET::PER_IO_CONTEXT &per_io_context) noexcept // NOLINT
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

        static RW_IO_CONTEXT *updateIoCxt(PER_IO_CONTEXT *accept_ctx) noexcept
        {
            auto *newRWCtx = new (std::nothrow) RW_IO_CONTEXT;
            if (newRWCtx == nullptr)
                std::abort();
            *newRWCtx = *accept_ctx;
            delete accept_ctx;
            return newRWCtx;
        };

        static void run(TcpReadWriteServer &server, std::size_t index) noexcept
        {
            ::std::cout << "TcpReadWriteServer::run start.\n";
            server.m_id = std::this_thread::get_id();
            server.m_index = index;
            DWORD bytesTransferred{};
            ULONG_PTR completionKey{};
            RW_IO_CONTEXT *pRWOv{};
            BOOL ret{};
            while (not get_exit_flag().load(std::memory_order_relaxed))
            {
                ret = ::GetQueuedCompletionStatus(
                    server.m_iocp, &bytesTransferred, &completionKey,
                    reinterpret_cast<OVERLAPPED **>(&pRWOv), INFINITE); // NOLINT

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
                        std::cout << "socket disconnection: " << pRWOv->socket_accept
                                  << '\n';
                        ::closesocket(pRWOv->socket_accept);
                        pRWOv->printBytes();
                        delete pRWOv;
                        continue;
                    }
                    std::cerr << "GetQueuedCompletionStatus error\n";
                    break;
                }

                // 在IO_READ处理逻辑中添加响应
                if (pRWOv->io_op_type == IO_OP_TYPE::IO_READ)
                {
                    std::cout << "do IO_OP_TYPE::IO_READ " << '\n';
                    pRWOv->total_bytes += bytesTransferred; // 累加接收字节数

                    if (bytesTransferred == 0) // TODO(mcs): 说明要端口连接？
                    {
                        std::cerr << "bytesTransferred == 0 error\n";
                        ::closesocket(pRWOv->socket_accept);
                        pRWOv->printBytes();
                        delete pRWOv;
                        continue;
                    }

                    // 解析HTTP请求
                    std::string request(pRWOv->buffer, bytesTransferred);
                    std::cout << "Received data: \n" << request << '\n';

                    bool clientKeepAlive = HTTP::ParseKeepAlive(request);
                    pRWOv->client_keep_alive = clientKeepAlive; // 保存到上下文
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

                    memcpy(pRWOv->buffer, response.c_str(), response.size());
                    pRWOv->wsabuf.buf = pRWOv->buffer; // 确保指针正确
                    pRWOv->wsabuf.len =
                        static_cast<ULONG>(response.size()); // 明确转换长度

                    // 投递 IO_WRITE
                    pRWOv->io_op_type = IO_OP_TYPE::IO_WRITE;
                    pRWOv->flags = 0;
                    if (FALSE == IOCP::PostAsyncSend(*pRWOv))
                    {
                        closesocket(pRWOv->socket_accept);
                        pRWOv->printBytes();
                        delete pRWOv;
                        break;
                    }

                    continue;
                } // 在main loop中增加IO_WRITE处理

                if (pRWOv->io_op_type == IO_OP_TYPE::IO_WRITE)
                {
                    std::cout << "do IO_OP_TYPE::IO_WRITE " << '\n';
                    pRWOv->sent_bytes += bytesTransferred; // 累加发送字节数
                    // 关闭连接
                    if (pRWOv->client_keep_alive) // 使用上下文中的标志
                    {
                        pRWOv->flags = 0;
                        pRWOv->io_op_type = IO_OP_TYPE::IO_READ;
                        if (FALSE == IOCP::PostAsyncRecv(*pRWOv))
                        {
                            std::cout << "closesocket and delete pAcceptOv" << '\n';
                            closesocket(pRWOv->socket_accept);
                            pRWOv->printBytes();
                            delete pRWOv;
                        }
                    }
                    else
                    {
                        std::cout << "closesocket and delete pAcceptOv" << '\n';
                        closesocket(pRWOv->socket_accept);
                        pRWOv->printBytes();
                        delete pRWOv;
                    }
                    continue;
                }

                std::cout << "Error: IO_OP_TYPE\n";
                break;
            }
            ::std::cout << "TcpReadWriteServer::run end. "
                        << "flag: " << get_exit_flag().load(std::memory_order_relaxed)
                        << '\n';
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

    static auto &getOneWorker()
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

        static PER_IO_CONTEXT *createPerIoCtx(
            IO_OP_TYPE type = IO_OP_TYPE::IO_INIT) noexcept
        {
            auto *per_io_context = new PER_IO_CONTEXT; // NOLINT
            per_io_context->io_op_type = type;
            // 初始化接收缓冲区
            ZeroMemory(per_io_context->buffer, sizeof(per_io_context->buffer));
            per_io_context->wsabuf.buf = per_io_context->buffer;
            per_io_context->wsabuf.len = PER_IO_CONTEXT::MAX_BUFF_SIZE;
            return per_io_context;
        }

        static PER_IO_CONTEXT *newCtxSocket(PER_IO_CONTEXT *ctx) noexcept
        {
            ctx->socket_accept = ::WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr,
                                              0, WSA_FLAG_OVERLAPPED);
            if (ctx->socket_accept == INVALID_SOCKET)
            {
                std::cerr << "WSASocket failed: " << WSAGetLastError() << '\n';
                std::abort();
            }
            return ctx;
        }

        static void run(TcpListenServer &server) noexcept
        {
            getOneWorker(); // init worker
            // IO_ACCEPT
            PER_IO_CONTEXT *ctx = TcpListenServer::createPerIoCtx(IO_OP_TYPE::IO_ACCEPT);
            newCtxSocket(ctx);
            IOCP::PostAsyncAccept(*ctx, server.m_listenSocket, server.m_pfnAcceptEx);

            ::std::cout << "TcpListenServer::run start\n";

            DWORD bytesTransferred{};
            ULONG_PTR completionKey{};
            PER_IO_CONTEXT *pAcceptOv{};
            BOOL ret{};
            auto fnGetAcceptExSockaddr = server.m_pfnGetAcceptExSockaddrs;
            while (not get_exit_flag().load(std::memory_order_relaxed))
            {
                ret = ::GetQueuedCompletionStatus(
                    server.m_iocp, &bytesTransferred, &completionKey,
                    reinterpret_cast<OVERLAPPED **>(&pAcceptOv), INFINITE); // NOLINT

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
                        std::cout << "socket disconnection: " << pAcceptOv->socket_accept
                                  << '\n';
                        ::closesocket(pAcceptOv->socket_accept);
                        delete pAcceptOv;
                        continue;
                    }
                    std::cerr << "GetQueuedCompletionStatus error\n";
                    break;
                }

                if (pAcceptOv->io_op_type == IO_OP_TYPE::IO_ACCEPT)
                {
                    // 正常接受
                    std::cout << "do IO_OP_TYPE::IO_ACCEPT " << '\n';
                    // NOLINTNEXTLINE
                    sockaddr *pLocalAddr = nullptr, *pRemoteAddr = nullptr; // NOLINT
                    int localLen = 0, remoteLen = 0;                        // NOLINT
                    // 调用GetAcceptExSockaddrs时使用正确的长度参数
                    fnGetAcceptExSockaddr(
                        pAcceptOv->buffer, 0,
                        PER_IO_CONTEXT::MIN_ADDR_LENGTH, // 本地地址保留长度
                        PER_IO_CONTEXT::MIN_ADDR_LENGTH, // 远程地址保留长度
                        &pLocalAddr, &localLen, &pRemoteAddr, &remoteLen);

                    ::setsockopt(pAcceptOv->socket_accept, SOL_SOCKET,
                                 SO_UPDATE_ACCEPT_CONTEXT,
                                 (char *)&pAcceptOv->socket_accept,
                                 sizeof(pAcceptOv->socket_accept));

                    // 将获取的地址复制到结构体中
                    memcpy(&pAcceptOv->local_addr, pLocalAddr, localLen);
                    memcpy(&pAcceptOv->remote_addr, pRemoteAddr, remoteLen);
                    printf("New connection accepted: Socket=%lld, Local Address=%s, "
                           "Remote Address=%s\n",
                           pAcceptOv->socket_accept,
                           IOCP::GetAddressString(pAcceptOv->local_addr)
                               .c_str(), // 本地IP地址
                           IOCP::GetAddressString(pAcceptOv->remote_addr)
                               .c_str() // 远程IP地址
                    );

                    // TODO(mcs): 转发线程
                    // ============= 关键修改点 =============
                    // 1. 将PER_IO_CONTEXT转换为RW_IO_CONTEXT
                    auto *newRWCtx = TcpReadWriteServer::updateIoCxt(pAcceptOv);

                    // 2. 获取工作线程的IOCP句柄
                    auto &worker = getOneWorker();
                    HANDLE workerIOCP = worker.getEnv().iocp;

                    // 3.
                    // 将socket绑定到工作线程的IOCP，完成键必须为socket句柄或RW_IO_CONTEXT指针
                    HANDLE hResult = ::CreateIoCompletionPort(
                        (HANDLE)newRWCtx->socket_accept, workerIOCP,
                        (ULONG_PTR)newRWCtx->socket_accept, // 或 (ULONG_PTR)newRWCtx
                        0);
                    if (hResult != workerIOCP)
                    {
                        std::cerr << "Bind to worker IOCP failed: " << GetLastError()
                                  << "\n";
                        closesocket(newRWCtx->socket_accept);
                        delete newRWCtx;
                        continue;
                    }

                    // 4. 直接投递异步读取操作（无需IO_TRANSFER）
                    newRWCtx->io_op_type = IO_OP_TYPE::IO_READ;
                    newRWCtx->flags = 0;
                    if (FALSE == IOCP::PostAsyncRecv(*newRWCtx))
                    {
                        closesocket(newRWCtx->socket_accept);
                        delete newRWCtx;
                    }
                    // ============= 修改结束 =============

                    // 重新投递AcceptEx请求
                    newCtxSocket(pAcceptOv);
                    pAcceptOv->flags = 0;
                    IOCP::PostAsyncAccept(*pAcceptOv, server.m_listenSocket,
                                          server.m_pfnAcceptEx);

                    continue;
                }

                std::cerr << "io_op_type error\n";
                break;
            }

            ::std::cout << "TcpListenServer::run end. "
                        << "flag: " << get_exit_flag().load(std::memory_order_relaxed)
                        << '\n';
        }
    };
}; // namespace NET

int main()
{
    (void)std::signal(SIGINT, signal_handler);
    NET::TcpListenServer server{};
    std::jthread l([&server]() { NET::TcpListenServer::run(server); });

    return 0;
}

#else

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
