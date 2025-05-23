#include "RxTx.h"
#include <libwebsockets.h>
#include <iostream>
#include <vector>
#include <cstring>
#include <chrono>
#include <thread>

static struct lws_protocols serverProtocols[] = {
    { "ws-protocol", RxTx::ServerCallback, 0, 1024 },
    { nullptr, nullptr, 0, 0 }
};

static struct lws_protocols clientProtocols[] = {
    { "ws-protocol", RxTx::ClientCallback, 0, 1024 },
    { nullptr, nullptr, 0, 0 }
};

RxTx::RxTx() {
    lws_set_log_level(LLL_ERR | LLL_WARN | LLL_NOTICE, nullptr);
}

RxTx::~RxTx()
{
    serverExit = true;
    clientExit = true;

    if (serverThread.joinable()) serverThread.join();
    if (clientThread.joinable()) clientThread.join();

    std::lock_guard<std::mutex> lock(contextMutex);
    if (serverContext) {
        lws_context_destroy(serverContext);
        serverContext = nullptr;
    }
    if (clientContext) {
        lws_context_destroy(clientContext);
        clientContext = nullptr;
    }
}

void RxTx::DoSomething()
{
    std::cout << "RxTx::DoSomething() from DLL" << std::endl;
}

void RxTx::StartServer(int port)
{
    serverThread = std::thread([this, port]() {
        struct lws_context_creation_info info = {};
        info.port = port;
        info.protocols = serverProtocols;
        info.gid = -1;
        info.uid = -1;
        info.user = this;

        {
            std::lock_guard<std::mutex> lock(contextMutex);
            serverContext = lws_create_context(&info);
        }
        if (!serverContext) return;

        while (!serverExit)
        {
            lws_service(serverContext, 100);
        }
        });
}

void RxTx::StartClient(const std::string& address, int port)
{
    clientThread = std::thread([this, address, port]() {
        std::cout << "[Client] StartClient thread started\n";

        struct lws_context_creation_info info = {};
        info.port = CONTEXT_PORT_NO_LISTEN;
        info.protocols = clientProtocols;
        info.gid = -1;
        info.uid = -1;

        {
            std::lock_guard<std::mutex> lock(contextMutex);
            clientContext = lws_create_context(&info);
        }

        if (!clientContext) {
            std::cerr << "[Client] Failed to create context" << std::endl;
            return;
        }

        int retryDelay = 2;
        const int maxDelay = 30;

        while (!clientExit)
        {
            struct lws_client_connect_info conn = {};
            conn.context = clientContext;
            conn.address = address.c_str();
            conn.port = port;
            conn.path = "/";
            conn.host = address.c_str();
            conn.origin = address.c_str();
            conn.protocol = "ws-protocol";
            conn.pwsi = nullptr;

            auto* wsi = lws_client_connect_via_info(&conn);
            if (!wsi) {
                std::cerr << "[Client] Connection failed. Retrying in " << retryDelay << " seconds...\n";
                std::this_thread::sleep_for(std::chrono::seconds(retryDelay));
                retryDelay = std::min(retryDelay * 2, maxDelay);
                continue;
            }

            retryDelay = 2; // 연결 성공 시 초기화

            while (!clientExit && lws_service(clientContext, 100) >= 0) {
                // 연결이 유지되는 동안 서비스 루프
            }

            std::cerr << "[Client] Disconnected. Attempting to reconnect...\n";
            std::this_thread::sleep_for(std::chrono::seconds(retryDelay));
            retryDelay = std::min(retryDelay * 2, maxDelay);
        }

        std::lock_guard<std::mutex> lock(contextMutex);
        if (clientContext) {
            lws_context_destroy(clientContext);
            clientContext = nullptr;
        }
        });
}

bool RxTx::SendToClients(const std::string& msg)
{
    std::lock_guard<std::mutex> lock(clientMutex);
    if (clients.empty()) return false;

    for (lws* client : clients)
    {
        std::vector<unsigned char> buf(LWS_PRE + msg.length());
        std::memcpy(&buf[LWS_PRE], msg.data(), msg.length());
        lws_write(client, &buf[LWS_PRE], msg.length(), LWS_WRITE_TEXT);
    }
    return true;
}

int RxTx::ServerCallback(struct lws* wsi, enum lws_callback_reasons reason,
    void* user, void* in, size_t len)
{
    RxTx* instance = static_cast<RxTx*>(lws_context_user(lws_get_context(wsi)));

    switch (reason)
    {
    case LWS_CALLBACK_ESTABLISHED:
    {
        std::lock_guard<std::mutex> lock(instance->clientMutex);
        instance->clients.insert(wsi);
        std::cout << "[Server] Client connected." << std::endl;
    }
    break;

    case LWS_CALLBACK_CLOSED:
    {
        std::lock_guard<std::mutex> lock(instance->clientMutex);
        instance->clients.erase(wsi);
        std::cout << "[Server] Client disconnected." << std::endl;
    }
    break;

    case LWS_CALLBACK_RECEIVE:
    {
        std::string msg(reinterpret_cast<char*>(in), len);
        std::cout << "[Server] Received: " << msg << std::endl;

        std::lock_guard<std::mutex> lock(instance->clientMutex);
        for (lws* client : instance->clients)
        {
            std::vector<unsigned char> buf(LWS_PRE + msg.length());
            std::memcpy(&buf[LWS_PRE], msg.data(), msg.length());
            lws_write(client, &buf[LWS_PRE], msg.length(), LWS_WRITE_TEXT);
        }
    }
    break;

    default:
        break;
    }

    return 0;
}

int RxTx::ClientCallback(struct lws* wsi, enum lws_callback_reasons reason,
    void* user, void* in, size_t len)
{
    std::cout << "[ClientCallback] reason = " << reason << std::endl;

    switch (reason)
    {
    case LWS_CALLBACK_CLIENT_ESTABLISHED:
        std::cout << "[Client] Connected to server!" << std::endl;
        break;

    case LWS_CALLBACK_CLIENT_RECEIVE:
        if (in && len > 0) {
            try {
                std::string msg(static_cast<char*>(in), len);
                std::cout << "[Client] Received: " << msg << std::endl;
            }
            catch (...) {
                std::cerr << "[Client] Error converting message to string" << std::endl;
            }
        }
        break;

    case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
        std::cerr << "[Client] Connection error!" << std::endl;
        break;

    case LWS_CALLBACK_CLIENT_CLOSED:
        std::cout << "[Client] Disconnected from server." << std::endl;
        break;

    default:
        break;
    }

    return 0;
}

extern "C" LIBRXTX_API IRxTx * CreateRxTx()
{
    return new RxTx();
}

extern "C" LIBRXTX_API void DestroyRxTx(IRxTx * ptr)
{
    delete ptr;
}
