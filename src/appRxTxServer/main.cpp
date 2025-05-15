#include <libRxTx.h>
#include <windows.h>
#include <iostream>

int main(int argc, char** argv)
{
    std::cout << "appRxTx starting..." << std::endl;

    RxTxLoader loader;
    if (!loader.IsValid()) {
        std::cerr << "Failed to load libRxTx.dll" << std::endl;
        return 1;
    }

    IRxTx* rxtx = loader.Create();
    if (rxtx) {
        rxtx->DoSomething();
        rxtx->StartServer(9000);  // 서버 시작

        std::cout << "Server running on port 9000. Press Enter to exit." << std::endl;
        std::cin.get();  // 서버 유지

        loader.Destroy(rxtx);
    }

    return 0;
}
