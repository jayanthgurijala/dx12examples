// base.cpp : Defines the entry point for the application.
//

#include "Dx12HelloWorld.h"

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    Dx12HelloWorld* pSample = new Dx12HelloWorld(1280, 720);
    int retval = pSample->RunApp(hInstance, nCmdShow);
    delete(pSample);

    return retval;
}