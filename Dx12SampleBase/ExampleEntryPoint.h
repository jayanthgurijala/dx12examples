#pragma once

#define DX_ENTRY_POINT(AppClass)                                      \
                                                                        \
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,                        \
                     _In_opt_ HINSTANCE hPrevInstance,                  \
                     _In_ LPWSTR    lpCmdLine,                          \
                     _In_ int       nCmdShow)                           \
{                                                                       \
    UNREFERENCED_PARAMETER(hPrevInstance);                              \
    UNREFERENCED_PARAMETER(lpCmdLine);                                   \
                                                                        \
    AppClass* pSample = new AppClass(1280, 720);                        \
    int retval = pSample->RunApp(hInstance, nCmdShow);                  \
    delete pSample;                                                     \
    return retval;                                                      \
}
\
