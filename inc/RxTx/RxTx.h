#pragma once
#include "RxTx/RxTxCommon.h"
#include <thread>
#include <functional>
#include <atomic>
#include <queue>
#include <mutex>
#include <condition_variable>

namespace libRxTx {

    enum class Protocol {
        TCP,
        UDP
    };

    enum class UdpMode {
        Unicast,
        Broadcast,
        Multicast
    };

    struct NetworkInterface {
        std::string localIp; // ex) "192.168.0.5"
        int port = 0;
    };

    class RXTX_API RxTx {
    public:
        explicit RxTx(Protocol proto);
        ~RxTx();

        bool Init();
        bool Bind(int port, const std::string& localIp = "");
        bool Listen(int backlog = 5);
        bool Connect(const std::string& host, int port);

        // UDP 모드
        void SetMode(UdpMode mode);
        void JoinMulticastGroup(const std::string& groupAddr);
        void LeaveMulticastGroup(const std::string& groupAddr);

        // 패킷 단위 송신 (Send / SendToAll)
        void SendPacket(const std::string& msg, const std::string& host = "", int port = 0);
        void SendToAll(const std::string& msg); // 자동 모드 송신

        // 콜백
        void OnReceive(std::function<void(const std::string&)> cb);
        void OnConnect(std::function<void(const std::string&)> cb);
        void OnDisconnect(std::function<void(const std::string&)> cb);

        // 스레드 제어
        void Start();
        void Stop();

    private:
        void RunReceiver();
        void RunSender();
        void Close();

    private:
        Protocol protocol_;
        UdpMode udpMode_;
        SOCKET sock_;
        sockaddr_in addr_;

        std::string localInterfaceIp_; // 선택한 NIC IP

        std::thread recvThread_;
        std::thread sendThread_;
        std::atomic<bool> running_;

        std::function<void(const std::string&)> recvCallback_;
        std::function<void(const std::string&)> connectCallback_;
        std::function<void(const std::string&)> disconnectCallback_;

        struct Outgoing {
            std::string msg;
            std::string host;
            int port;
        };
        std::queue<Outgoing> sendQueue_;
        std::mutex sendMutex_;
        std::condition_variable sendCond_;
    };

} // namespace libRxTx
