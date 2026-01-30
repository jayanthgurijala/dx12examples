#include "pch.h"
#include "DxPrintUtils.h"
#include "stdafx.h"


namespace PrintUtils
{
	void PrintXMMatrix(LPCSTR matrixName, const XMMATRIX& matrix)
	{
		XMFLOAT4X4 matrixData;
		XMStoreFloat4x4(&matrixData, matrix);
		OutputDebugStringA("\n");
		OutputDebugStringA(matrixName);
		OutputDebugStringA("\n");
		for (int i = 0; i < 4; i++)
		{
			wchar_t buf[128];
			swprintf_s(buf, L"%f %f %f %f\n",
				matrixData.m[i][0], matrixData.m[i][1], matrixData.m[i][2], matrixData.m[i][3]);
			OutputDebugString(buf);
		}
	}

	void PrintString(LPCSTR stringMessage)
	{
		OutputDebugStringA(stringMessage);
		OutputDebugStringA("\n");
	}
}