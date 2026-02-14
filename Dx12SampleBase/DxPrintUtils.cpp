/*
* Copyright (C) 2026 by Jayanth Gurijala
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/
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

	void PrintBufferAddressRange(ID3D12Resource* pResource)
	{
		UINT64 startGpuVA = pResource->GetGPUVirtualAddress();
		const auto& resourceDesc = pResource->GetDesc();
		UINT64 sizeInBytes = resourceDesc.Width;
		wchar_t buf[128];
		swprintf_s(buf, L"Buffer GpuVA Start: 0x%llx End: 0x%llx Size: 0x%llu\n", startGpuVA, startGpuVA + resourceDesc.Width, resourceDesc.Width);
		OutputDebugString(buf);
	}
}
