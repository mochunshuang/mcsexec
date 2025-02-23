#include <cassert>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_set>

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

/**
 * 客户端连接流程：

投递ConnectEx → 完成通知 → 成功则投递WSARecv。

服务端接受流程：

投递AcceptEx → 完成通知 → 初始化新Socket → 投递WSARecv。

数据读写流程：

通过WSARecv/WSASend投递请求 → 完成通知 → 处理数据 → 重新投递
 *
 */

//
enum IO_OP_TYPE : std::int8_t
{
    IO_INIT,
    IO_ACCEPT,
    IO_READ,
    IO_WRITE,
};

//  data to be associated for every I/O operation on a socket
//  无论怎么样，tcp是流式传输数据，必须在请求头找到这个完整请求的 total_bytes
struct PER_IO_CONTEXT
{
    static constexpr int MAX_BUFF_SIZE = 4096;                        // NOLINT
    static constexpr size_t MAX_ADDR_LEN =                            // NOLINT
        sizeof(sockaddr_storage) + 16;                                // 支持 IPv4 和 IPv6
    static constexpr auto MIN_ADDR_LENGTH = sizeof(sockaddr_in) + 16; // NOLINT

    OVERLAPPED overlapped;                // NOLINT
    char buffer[MAX_BUFF_SIZE];           // NOLINT
    IO_OP_TYPE io_op_type;                // NOLINT
    WSABUF wsabuf;                        // NOLINT
    int total_bytes;                      // NOLINT
    int sent_bytes;                       // NOLINT
    SOCKET socket_accept;                 // NOLINT
    DWORD flags;                          // NOLINT
    PER_IO_CONTEXT *p_io_context_forward; // NOLINT
    sockaddr_storage local_addr;          // NOLINT
    sockaddr_storage remote_addr;         // NOLINT
    bool client_keep_alive;               // NOLINT

    PER_IO_CONTEXT() noexcept
        : overlapped{}, buffer{}, io_op_type{IO_OP_TYPE::IO_INIT}, wsabuf{},
          total_bytes{}, sent_bytes{}, socket_accept{INVALID_SOCKET}, flags{},
          p_io_context_forward{}, local_addr{}, remote_addr{}, client_keep_alive{}
    {
        // {} 等价于 ZeroMemory， 0 初始化
        wsabuf.buf = buffer;
        wsabuf.len = MAX_BUFF_SIZE;
        assert(overlapped.Internal == 0);
        assert(wsabuf.buf[1] == 0);
        assert(p_io_context_forward == nullptr);
    }
};

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

    static BOOL PostAsyncAccept(PER_IO_CONTEXT &per_io_context, SOCKET &listenSocket,
                                LPFN_ACCEPTEX &pfnAcceptEx) noexcept
    {
        // 投递 AcceptEx 请求
        DWORD bytes_received = 0;
        BOOL result =
            pfnAcceptEx(listenSocket,                 // 监听套接字
                        per_io_context.socket_accept, // 接受套接字
                        per_io_context.buffer,        // 接收缓冲区
                        0, // 接收缓冲区中用于存储本地地址和远程地址的空间大小
                        PER_IO_CONTEXT::MIN_ADDR_LENGTH,                // 本地地址长度
                        PER_IO_CONTEXT::MIN_ADDR_LENGTH,                // 远程地址长度
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

    static void PostAsyncRecvBeffor(PER_IO_CONTEXT &per_io_context) noexcept
    {
        per_io_context.flags = 0;
    }

    static BOOL PostAsyncRecv(PER_IO_CONTEXT &per_io_context) noexcept
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

    static void PostAsyncSendBeffor(PER_IO_CONTEXT &per_io_context) noexcept
    {
        per_io_context.flags = 0;
    }
    static BOOL PostAsyncSend(PER_IO_CONTEXT &per_io_context) noexcept
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

    static bool ParseKeepAlive(const std::string &request)
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
            if (m_iocp = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
                m_iocp == nullptr)
                throw std::runtime_error("CreateIoCompletionPort error");
            if (m_listenSocket = // NOLINTNEXTLINE
                ::WSASocketW(AF_INET, SOCK_STREAM, 0, nullptr, 0, WSA_FLAG_OVERLAPPED);
                m_listenSocket == INVALID_SOCKET)
                throw std::runtime_error("WSASocket error");

            // Set IO to NBIO
            if (u_long u1 = 1;
                ::ioctlsocket(m_listenSocket, FIONBIO, &u1) == SOCKET_ERROR)
                throw std::runtime_error("ioctlsocket error");
            // m_listenFd close send/reciver
            int size = 0;
            ::setsockopt(m_listenSocket, SOL_SOCKET, SO_SNDBUF,
                         reinterpret_cast<const char *>(&size), sizeof(size)); // NOLINT
            ::setsockopt(m_listenSocket, SOL_SOCKET, SO_RCVBUF,
                         reinterpret_cast<const char *>(&size), sizeof(size)); // NOLINT

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
                         &m_pfnGetAcceptExSockaddrs, sizeof(m_pfnGetAcceptExSockaddrs),
                         &dwBytes, nullptr, nullptr))
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

    static PER_IO_CONTEXT *createPerIoCtx(IO_OP_TYPE type = IO_OP_TYPE::IO_INIT) noexcept
    {
        auto *per_io_context = new PER_IO_CONTEXT; // NOLINT
        per_io_context->socket_accept = ::WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP,
                                                     nullptr, 0, WSA_FLAG_OVERLAPPED);
        if (per_io_context->socket_accept == INVALID_SOCKET)
        {
            std::cerr << "WSASocket failed: " << WSAGetLastError() << '\n';
            return FALSE;
        }

        per_io_context->io_op_type = type;

        // 初始化接收缓冲区
        ZeroMemory(per_io_context->buffer, sizeof(per_io_context->buffer));
        per_io_context->wsabuf.buf = per_io_context->buffer;
        per_io_context->wsabuf.len = PER_IO_CONTEXT::MAX_BUFF_SIZE;
        return per_io_context;
    }

    static void run(TcpListenServer &server) noexcept
    {
        // IO_ACCEPT
        PER_IO_CONTEXT *ctx = TcpListenServer::createPerIoCtx(IO_OP_TYPE::IO_ACCEPT);
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
                fnGetAcceptExSockaddr(pAcceptOv->buffer, 0,
                                      PER_IO_CONTEXT::MIN_ADDR_LENGTH, // 本地地址保留长度
                                      PER_IO_CONTEXT::MIN_ADDR_LENGTH, // 远程地址保留长度
                                      &pLocalAddr, &localLen, &pRemoteAddr, &remoteLen);

                ::setsockopt(pAcceptOv->socket_accept, SOL_SOCKET,
                             SO_UPDATE_ACCEPT_CONTEXT, (char *)&pAcceptOv->socket_accept,
                             sizeof(pAcceptOv->socket_accept));

                ::CreateIoCompletionPort((HANDLE)pAcceptOv->socket_accept, server.m_iocp,
                                         (ULONG_PTR)pAcceptOv->socket_accept, 0);

                // 将获取的地址复制到结构体中
                memcpy(&pAcceptOv->local_addr, pLocalAddr, localLen);
                memcpy(&pAcceptOv->remote_addr, pRemoteAddr, remoteLen);
                printf(
                    "New connection accepted: Socket=%lld, Local Address=%s, "
                    "Remote Address=%s\n",
                    pAcceptOv->socket_accept,
                    IOCP::GetAddressString(pAcceptOv->local_addr).c_str(), // 本地IP地址
                    IOCP::GetAddressString(pAcceptOv->remote_addr).c_str() // 远程IP地址
                );

                // TODO: 触发新连接处理逻辑
                // ============= 新增以下代码 =============
                // 创建新的IO上下文用于接收数据
                pAcceptOv->io_op_type = IO_OP_TYPE::IO_READ;
                // 要不要处理 buffer?
                // 投递异步接收请求
                IOCP::PostAsyncRecvBeffor(*pAcceptOv);
                if (FALSE == IOCP::PostAsyncRecv(*pAcceptOv))
                {
                    std::cerr << "PostAsyncRecv error\n";
                    ::closesocket(pAcceptOv->socket_accept);
                    delete pAcceptOv;
                    break;
                }
                // ============= 新增结束 =============

                // 重新投递AcceptEx请求
                PER_IO_CONTEXT *newAcceptCtx =
                    TcpListenServer::createPerIoCtx(IO_OP_TYPE::IO_ACCEPT);
                IOCP::PostAsyncAccept(*newAcceptCtx, server.m_listenSocket,
                                      server.m_pfnAcceptEx);

                continue;
            }
            // 在IO_READ处理逻辑中添加响应
            if (pAcceptOv->io_op_type == IO_OP_TYPE::IO_READ)
            {
                std::cout << "do IO_OP_TYPE::IO_READ " << '\n';
                if (bytesTransferred == 0) // TODO 说明要端口连接？
                {
                    std::cerr << "bytesTransferred == 0 error\n";
                    ::closesocket(pAcceptOv->socket_accept);
                    delete pAcceptOv;
                    continue;
                }

                // 解析HTTP请求
                std::string request(pAcceptOv->buffer, bytesTransferred);
                std::cout << "Received data: \n" << request << '\n';

                bool clientKeepAlive = HTTP::ParseKeepAlive(request);
                pAcceptOv->client_keep_alive = clientKeepAlive; // 保存到上下文
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

                memcpy(pAcceptOv->buffer, response.c_str(), response.size());
                pAcceptOv->wsabuf.buf = pAcceptOv->buffer; // 确保指针正确
                pAcceptOv->wsabuf.len =
                    static_cast<ULONG>(response.size()); // 明确转换长度

                // 投递 IO_WRITE
                pAcceptOv->io_op_type = IO_OP_TYPE::IO_WRITE;
                IOCP::PostAsyncSendBeffor(*pAcceptOv);
                if (FALSE == IOCP::PostAsyncSend(*pAcceptOv))
                {
                    closesocket(pAcceptOv->socket_accept);
                    delete pAcceptOv;
                    break;
                }

                continue;
            } // 在main loop中增加IO_WRITE处理

            if (pAcceptOv->io_op_type == IO_OP_TYPE::IO_WRITE)
            {
                std::cout << "do IO_OP_TYPE::IO_WRITE " << '\n';
                // 关闭连接
                if (pAcceptOv->client_keep_alive) // 使用上下文中的标志
                {
                    IOCP::PostAsyncRecvBeffor(*pAcceptOv);
                    pAcceptOv->io_op_type = IO_OP_TYPE::IO_READ;
                    if (FALSE == IOCP::PostAsyncRecv(*pAcceptOv))
                    {
                        std::cout << "closesocket and delete pAcceptOv" << '\n';
                        closesocket(pAcceptOv->socket_accept);
                        delete pAcceptOv;
                    }
                }
                else
                {
                    std::cout << "closesocket and delete pAcceptOv" << '\n';
                    closesocket(pAcceptOv->socket_accept);
                    delete pAcceptOv;
                }
                continue;
            }

            std::cerr << "io_op_type error\n";
            break;
        }

        ::std::cout << "TcpListenServer::run end. "
                    << "flag: " << get_exit_flag().load(std::memory_order_relaxed);
    }
};

int main()
{
    (void)std::signal(SIGINT, signal_handler);
    TcpListenServer server{};
    // std::jthread l(&TcpListenServer::run, server); // 不行
    std::jthread l([&server]() { TcpListenServer::run(server); }); // 可以

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

// Note: 没想到浏览器： http://127.0.0.1:8080 会占用两个端口
/*

26	2.262347	127.0.0.1	127.0.0.1	TCP	44	8080 → 60931 [ACK] Seq=1 Ack=785
Win=2619648 Len=0 25	2.262326	127.0.0.1	127.0.0.1	HTTP	828	GET / HTTP/1.1
24	2.262131	127.0.0.1	127.0.0.1	TCP	44	60932 → 8080 [ACK] Seq=1 Ack=1 Win=2619648
Len=0

23	2.262109	127.0.0.1	127.0.0.1	TCP	56	8080 → 60932 [SYN, ACK] Seq=0 Ack=1
Win=65535 Len=0 MSS=65495 WS=256 SACK_PERM=1

22	2.262071	127.0.0.1	127.0.0.1	TCP	56	60932 → 8080 [SYN] Seq=0 Win=65535 Len=0
MSS=65495 WS=256 SACK_PERM=1

21	2.261908	127.0.0.1	127.0.0.1	TCP	44	60931 → 8080 [ACK] Seq=1 Ack=1 Win=2619648
Len=0

20	2.261878	127.0.0.1	127.0.0.1	TCP	56	8080 → 60931 [SYN, ACK] Seq=0 Ack=1
Win=65535 Len=0 MSS=65495 WS=256 SACK_PERM=1

19	2.261815	127.0.0.1	127.0.0.1	TCP	56	60931 → 8080 [SYN] Seq=0 Win=65535 Len=0
MSS=65495 WS=256 SACK_PERM=1




*/

/**
//Note: 然后看到两个 Accept 请求得到处理
ok
New connection accepted: Socket=284, Local Address=127.0.0.1:8080, Remote
Address=127.0.0.1:60931

ok
New connection accepted: Socket=288, Local Address=127.0.0.1:8080, Remote
Address=127.0.0.1:60932

 */