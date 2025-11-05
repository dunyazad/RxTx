#include "RxTx/RxTx.h"

namespace libRxTx {

    RxTx::RxTx(Protocol proto)
        : protocol_(proto), udpMode_(UdpMode::Unicast), sock_(INVALID_SOCKET), running_(false) {
    }

    RxTx::~RxTx() {
        Stop();
    }

    bool RxTx::Init() {
        if (!InitSocket()) return false;
        int type = (protocol_ == Protocol::TCP) ? SOCK_STREAM : SOCK_DGRAM;
        sock_ = socket(AF_INET, type, 0);
        if (sock_ == INVALID_SOCKET) {
            std::cerr << "RxTx: socket creation failed\n";
            return false;
        }
        return true;
    }

    bool RxTx::Bind(int port, const std::string& localIp) {
        addr_.sin_family = AF_INET;
        addr_.sin_port = htons(port);
        addr_.sin_addr.s_addr = localIp.empty() ? INADDR_ANY : inet_addr(localIp.c_str());
        localInterfaceIp_ = localIp;

        if (bind(sock_, (sockaddr*)&addr_, sizeof(addr_)) == SOCKET_ERROR) {
            std::cerr << "RxTx: bind failed\n";
            return false;
        }

        // Broadcast 옵션
        if (protocol_ == Protocol::UDP && udpMode_ == UdpMode::Broadcast) {
            BOOL enable = TRUE;
            setsockopt(sock_, SOL_SOCKET, SO_BROADCAST, (char*)&enable, sizeof(enable));
            std::cout << "[UDP] Broadcast mode enabled\n";
        }

        std::cout << (protocol_ == Protocol::TCP ? "TCP" : "UDP")
            << " socket bound on port " << port
            << (localIp.empty() ? "" : (" (Interface: " + localIp + ")")) << std::endl;

        return true;
    }

    bool RxTx::Listen(int backlog) {
        if (protocol_ != Protocol::TCP) return false;
        if (listen(sock_, backlog) == SOCKET_ERROR) {
            std::cerr << "RxTx: listen failed\n";
            return false;
        }
        std::cout << "TCP listening..." << std::endl;
        return true;
    }

    bool RxTx::Connect(const std::string& host, int port) {
        addr_.sin_family = AF_INET;
        addr_.sin_port = htons(port);
        inet_pton(AF_INET, host.c_str(), &addr_.sin_addr);

        if (protocol_ == Protocol::TCP) {
            if (connect(sock_, (sockaddr*)&addr_, sizeof(addr_)) == SOCKET_ERROR) {
                std::cerr << "RxTx: connect failed\n";
                return false;
            }
            if (connectCallback_) connectCallback_(host);
            std::cout << "TCP connected to " << host << ":" << port << std::endl;
        }
        return true;
    }

    void RxTx::SetMode(UdpMode mode) {
        udpMode_ = mode;
        if (protocol_ != Protocol::UDP) return;
        if (udpMode_ == UdpMode::Broadcast) {
            BOOL enable = TRUE;
            setsockopt(sock_, SOL_SOCKET, SO_BROADCAST, (char*)&enable, sizeof(enable));
            std::cout << "[UDP] Broadcast mode activated\n";
        }
        else if (udpMode_ == UdpMode::Multicast) {
            std::cout << "[UDP] Multicast mode selected\n";
        }
        else {
            std::cout << "[UDP] Unicast mode\n";
        }
    }

    void RxTx::JoinMulticastGroup(const std::string& groupAddr) {
        if (protocol_ != Protocol::UDP) return;
        ip_mreq mreq{};
        mreq.imr_multiaddr.s_addr = inet_addr(groupAddr.c_str());
        mreq.imr_interface.s_addr = localInterfaceIp_.empty() ? htonl(INADDR_ANY) : inet_addr(localInterfaceIp_.c_str());
        setsockopt(sock_, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&mreq, sizeof(mreq));
        std::cout << "[UDP] Joined multicast group " << groupAddr << std::endl;
    }

    void RxTx::LeaveMulticastGroup(const std::string& groupAddr) {
        if (protocol_ != Protocol::UDP) return;
        ip_mreq mreq{};
        mreq.imr_multiaddr.s_addr = inet_addr(groupAddr.c_str());
        mreq.imr_interface.s_addr = htonl(INADDR_ANY);
        setsockopt(sock_, IPPROTO_IP, IP_DROP_MEMBERSHIP, (char*)&mreq, sizeof(mreq));
        std::cout << "[UDP] Left multicast group " << groupAddr << std::endl;
    }

    void RxTx::SendPacket(const std::string& msg, const std::string& host, int port) {
        std::lock_guard<std::mutex> lock(sendMutex_);
        sendQueue_.push({ msg, host, port });
        sendCond_.notify_one();
    }

    void RxTx::SendToAll(const std::string& msg) {
        if (protocol_ != Protocol::UDP) return;

        switch (udpMode_) {
        case UdpMode::Unicast:
            SendPacket(msg, "127.0.0.1", ntohs(addr_.sin_port));
            break;
        case UdpMode::Broadcast:
            SendPacket(msg, "255.255.255.255", ntohs(addr_.sin_port));
            break;
        case UdpMode::Multicast:
            SendPacket(msg, "239.1.1.1", ntohs(addr_.sin_port));
            break;
        }
    }

    void RxTx::OnReceive(std::function<void(const std::string&)> cb) {
        recvCallback_ = std::move(cb);
    }

    void RxTx::OnConnect(std::function<void(const std::string&)> cb) {
        connectCallback_ = std::move(cb);
    }

    void RxTx::OnDisconnect(std::function<void(const std::string&)> cb) {
        disconnectCallback_ = std::move(cb);
    }

    void RxTx::Start() {
        if (running_) return;
        running_ = true;
        recvThread_ = std::thread(&RxTx::RunReceiver, this);
        sendThread_ = std::thread(&RxTx::RunSender, this);
    }

    // --- 수신 스레드 ---
    void RxTx::RunReceiver() {
        char buffer[4096] = {};

        while (running_) {
            if (protocol_ == Protocol::TCP) {
                fd_set fds;
                timeval tv{ 1, 0 };
                FD_ZERO(&fds);
                FD_SET(sock_, &fds);

                int sel = select(0, &fds, nullptr, nullptr, &tv);
                if (sel > 0 && FD_ISSET(sock_, &fds)) {
                    sockaddr_in clientAddr{};
                    socklen_t len = sizeof(clientAddr);
                    SOCKET clientSock = accept(sock_, (sockaddr*)&clientAddr, &len);
                    if (clientSock == INVALID_SOCKET) continue;

                    std::string clientIp = inet_ntoa(clientAddr.sin_addr);
                    if (connectCallback_) connectCallback_(clientIp);

                    while (running_) {
                        fd_set cfds;
                        timeval ctv{ 1, 0 };
                        FD_ZERO(&cfds);
                        FD_SET(clientSock, &cfds);

                        int rc = select(0, &cfds, nullptr, nullptr, &ctv);
                        if (rc > 0 && FD_ISSET(clientSock, &cfds)) {
                            int headerLen = recv(clientSock, buffer, 4, 0);
                            if (headerLen <= 0) break;
                            int packetSize = ntohl(*(int*)buffer);
                            int bytes = recv(clientSock, buffer, packetSize, 0);
                            if (bytes <= 0) break;
                            buffer[bytes] = '\0';
                            if (recvCallback_) recvCallback_(std::string(buffer, bytes));
                        }
                        else if (!running_) {
                            break;
                        }
                    }

                    if (disconnectCallback_) disconnectCallback_(clientIp);
#ifdef _WIN32
                    closesocket(clientSock);
#else
                    close(clientSock);
#endif
                }
            }
            else {
                sockaddr_in from{};
                socklen_t fromLen = sizeof(from);
                fd_set fds;
                timeval tv{ 1, 0 };
                FD_ZERO(&fds);
                FD_SET(sock_, &fds);
                if (select(0, &fds, nullptr, nullptr, &tv) > 0) {
                    int bytes = recvfrom(sock_, buffer, sizeof(buffer) - 1, 0, (sockaddr*)&from, &fromLen);
                    if (bytes > 0) {
                        buffer[bytes] = '\0';
                        if (recvCallback_) recvCallback_(std::string(buffer, bytes));
                    }
                }
            }
        }
    }

    // --- 송신 스레드 ---
    void RxTx::RunSender() {
        while (running_) {
            std::unique_lock<std::mutex> lock(sendMutex_);
            sendCond_.wait(lock, [this] { return !sendQueue_.empty() || !running_; });
            if (!running_) break;
            Outgoing out = sendQueue_.front();
            sendQueue_.pop();
            lock.unlock();

            if (protocol_ == Protocol::TCP) {
                int len = htonl((int)out.msg.size());
                send(sock_, (char*)&len, 4, 0);
                send(sock_, out.msg.c_str(), (int)out.msg.size(), 0);
            }
            else {
                sockaddr_in dest{};
                dest.sin_family = AF_INET;
                dest.sin_port = htons(out.port);
                inet_pton(AF_INET, out.host.c_str(), &dest.sin_addr);
                sendto(sock_, out.msg.c_str(), (int)out.msg.size(), 0, (sockaddr*)&dest, sizeof(dest));
            }
        }
    }

    void RxTx::Stop() {
        running_ = false;
        sendCond_.notify_all();

        // ★ TCP 서버 깨우기용 더미 연결
        if (protocol_ == Protocol::TCP) {
            SOCKET tmp = socket(AF_INET, SOCK_STREAM, 0);
            sockaddr_in tempAddr{};
            tempAddr.sin_family = AF_INET;
            tempAddr.sin_port = addr_.sin_port;
            tempAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
            connect(tmp, (sockaddr*)&tempAddr, sizeof(tempAddr));
#ifdef _WIN32
            closesocket(tmp);
#else
            close(tmp);
#endif
        }

        if (recvThread_.joinable()) recvThread_.join();
        if (sendThread_.joinable()) sendThread_.join();

        Close();
    }

    void RxTx::Close() {
        if (sock_ != INVALID_SOCKET) {
#ifdef _WIN32
            closesocket(sock_);
#else
            close(sock_);
#endif
            sock_ = INVALID_SOCKET;
        }
        CloseSocket();
    }

} // namespace libRxTx
