

#include "stdafx.h"
#include "Win32Application.h"
#include "Dx12SampleBase.h"
#include <chrono>

HWND Win32Application::m_hwnd = nullptr;

int Win32Application::Run(Dx12SampleBase* pSample, HINSTANCE hInstance, int nCmdShow)
{
	//@todo command line arguments

	///@todo understand this
	CoInitializeEx(nullptr, COINIT_MULTITHREADED);


	//Create window
	WNDCLASSEX windowClass = {};
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = WindowProc;
	windowClass.hInstance = hInstance;
	windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	windowClass.lpszClassName = L"dx12Examples";
	RegisterClassEx(&windowClass);

	//@todo hard coded size, get it with command line arguments and a default value
	RECT windowRect = { 0, 0, 1280, 720 };
	AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

	m_hwnd = CreateWindow(
		windowClass.lpszClassName,
		L"Dx12 Examples",						///@todo use app name
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		windowRect.right - windowRect.left,
		windowRect.bottom - windowRect.top,
		nullptr,
		nullptr,
		hInstance,
		nullptr 								///@todo need this in the callback to WindowProc
	);

	ShowWindow(m_hwnd, nCmdShow);

	pSample->OnInit(m_hwnd);

	pSample->PreRun();

	//Main message loop
	MSG msg = {};
	using clock = std::chrono::high_resolution_clock;
	using durationf32 = std::chrono::duration<float>;
	auto previousTime = clock::now();

	while (msg.message != WM_QUIT)
	{
		
		//process messages in the queue
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		auto currentTime = clock::now();

		// deltaTime in seconds
		 durationf32 duration = (currentTime - previousTime);

		pSample->Run(duration.count());
		previousTime = currentTime;
	}

	pSample->PostRun();

	// Return this part of the WM_QUIT message to Windows.
	return static_cast<char>(msg.wParam);
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