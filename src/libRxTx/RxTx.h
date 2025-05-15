// RxTx.h
#pragma once

#include "libRxTx.h"
#include <libwebsockets.h>
#include <iostream>
#include <set>
#include <mutex>
#include <thread>
#include <atomic>

class RxTx : public IRxTx {
public:
    RxTx();
    ~RxTx() override;

    void DoSomething() override;
    void StartServer(int port = 9000) override;
    void StartClient(const std::string& address = "localhost", int port = 9000) override;

    static int ServerCallback(struct lws* wsi, enum lws_callback_reasons reason, void* user, void* in, size_t len);

private:
    std::thread serverThread;
    std::atomic<bool> shouldExit{ false };
    struct lws_context* serverContext = nullptr;
    std::set<lws*> clients;
    std::mutex clientMutex;

    // 클라이언트 관련
    std::thread clientThread;
    std::atomic<bool> clientExit{ false };
    struct lws_context* clientContext = nullptr;
    struct lws* clientWsi = nullptr;
};
