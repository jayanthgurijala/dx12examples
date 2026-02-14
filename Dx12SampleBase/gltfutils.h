/*
* Copyright (C) 2026 by Jayanth Gurijala
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/
#pragma once

#include <d3d12.h>

namespace GltfUtils
{
	DXGI_FORMAT GetDxgiFloatFormat(int numComponents);
	DXGI_FORMAT GetDxgiUnsignedShortFormat(int numComponents);
	DXGI_FORMAT GltfGetDxgiFormat(int tinyGltfComponentType, int components);

	//e.g. FLOAT has 4 bytes
	UINT GetComponentTypeSizeInBytes(UINT componentType);

	//e.g "VEC3" has 3 components
	UINT GetNumComponentsInType(UINT type);
}

