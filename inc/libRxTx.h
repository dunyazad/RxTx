// libRxTx.h
#pragma once

#include <string>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <windows.h>

#ifdef LIBRXTX_EXPORTS
#define LIBRXTX_API __declspec(dllexport)
#else
#define LIBRXTX_API __declspec(dllimport)
#endif

class LIBRXTX_API IRxTx {
public:
    virtual void DoSomething() = 0;
    virtual void StartServer(int port = 9000) = 0;
    virtual void StartClient(const std::string& address = "localhost", int port = 9000) = 0; // Ãß°¡
    virtual ~IRxTx() = default;
};

extern "C" {
    LIBRXTX_API IRxTx* CreateRxTx();
    LIBRXTX_API void DestroyRxTx(IRxTx*);
}

class RxTxLoader {
public:
    RxTxLoader(const std::string& dllName = "libRxTx.dll") {
        hModule = LoadLibraryA(dllName.c_str());
        if (!hModule) return;

        create = reinterpret_cast<CreateFunc>(GetProcAddress(hModule, "CreateRxTx"));
        destroy = reinterpret_cast<DestroyFunc>(GetProcAddress(hModule, "DestroyRxTx"));
    }

    ~RxTxLoader() {
        if (hModule) FreeLibrary(hModule);
    }

    bool IsValid() const { return hModule && create && destroy; }

    IRxTx* Create() const { return create ? create() : nullptr; }
    void Destroy(IRxTx* ptr) const { if (destroy) destroy(ptr); }

private:
    using CreateFunc = IRxTx * (*)();
    using DestroyFunc = void (*)(IRxTx*);

    HMODULE hModule = nullptr;
    CreateFunc create = nullptr;
    DestroyFunc destroy = nullptr;
};
