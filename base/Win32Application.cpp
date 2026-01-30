

#include "stdafx.h"
#include "Win32Application.h"
#include "Dx12SampleBase.h"
#include <chrono>


int Win32Application::Run(Dx12SampleBase* pSample, HINSTANCE hInstance, int nCmdShow)
{
	pSample->SetupWindow(hInstance, nCmdShow);
	pSample->OnInit();
	pSample->PreRun();
	int retval = pSample->RenderLoop();
	pSample->PostRun();

	return retval;
}

LRESULT CALLBACK Win32Application::WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE)
			PostQuitMessage(0);
		break;
	
	case WM_DESTROY:
		//Post WM_QUIT with the exit code
		PostQuitMessage(0);
		break;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}