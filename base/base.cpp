// base.cpp : Defines the entry point for the application.
//

#include "Win32Application.h"
#include "Dx12HelloWorld.h"

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    Dx12HelloWorld sample(1280, 720);
    return Win32Application::Run(&sample, hInstance, nCmdShow);
}