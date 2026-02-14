/*
* Copyright (C) 2026 by Jayanth Gurijala
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/
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



