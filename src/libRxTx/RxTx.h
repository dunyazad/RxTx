#pragma once

#define NOMINMAX

#include "libRxTx.h"
#include <algorithm>
#include <atomic>
#include <mutex>
#include <set>
#include <string>
#include <thread>

struct lws;
enum lws_callback_reasons;

class RxTx : public IRxTx {
public:
    RxTx();
    ~RxTx() override;

    void DoSomething() override;
    void StartServer(int port = 9000) override;
    void StartClient(const std::string& address = "localhost", int port = 9000) override;

    bool SendToClients(const std::string& msg) override;

    static int ServerCallback(struct lws* wsi, enum lws_callback_reasons reason, void* user, void* in, size_t len);
    static int ClientCallback(struct lws* wsi, enum lws_callback_reasons reason, void* user, void* in, size_t len);

private:
    std::thread serverThread;
    std::thread clientThread;

    std::atomic<bool> serverExit{ false };
    std::atomic<bool> clientExit{ false };

    struct lws_context* serverContext = nullptr;
    struct lws_context* clientContext = nullptr;

    std::set<lws*> clients;
    std::mutex clientMutex;
    std::mutex serverMutex;
    std::mutex contextMutex;
};
