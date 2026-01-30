#pragma once

#include "Dx12SampleBase.h"
#include "stdafx.h"

class Win32Application
{
public:
	static int Run(Dx12SampleBase* pSample, HINSTANCE hInstance, int nCmdShow);

protected:
	static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
};

