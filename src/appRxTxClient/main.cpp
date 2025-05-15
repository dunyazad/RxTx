#include <libRxTx.h>
#include <windows.h>
#include <iostream>

int main(int argc, char** argv)
{
    std::cout << "appRxClient starting..." << std::endl;

    RxTxLoader loader;
    if (!loader.IsValid()) {
        std::cerr << "Failed to load libRxTx.dll" << std::endl;
        return 1;
    }

    IRxTx* rxtx = loader.Create();
    if (rxtx) {
        rxtx->DoSomething();  // �׽�Ʈ ���
        rxtx->StartClient("localhost", 9000);  // ������ ����

        std::cout << "Client connected to ws://localhost:9000\n";
        std::cout << "Press Enter to quit..." << std::endl;
        std::cin.get();  // Ŭ���̾�Ʈ ����

        loader.Destroy(rxtx);
    }

    return 0;
}
