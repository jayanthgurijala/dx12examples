#pragma once

#include <string.h>
#include <d3d12.h>
#include <DirectXMath.h>

using namespace DirectX;


namespace PrintUtils
{
	void PrintXMMatrix(LPCSTR matrixName, const XMMATRIX& matrix);
	void PrintString(LPCSTR stringMessage);
	void PrintBufferAddressRange(ID3D12Resource* pResource);
}


