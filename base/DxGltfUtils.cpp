#include "DxGltfUtils.h"
#include "tiny_gltf.h"

namespace DxGltfUtils
{

	DXGI_FORMAT GetDxgiFloatFormat(int numComponents)
	{
		DXGI_FORMAT dxgiFormat = DXGI_FORMAT_UNKNOWN;

		switch (numComponents)
		{
		case TINYGLTF_TYPE_SCALAR:
			dxgiFormat = DXGI_FORMAT_R32_FLOAT;
			break;
		case TINYGLTF_TYPE_VEC2:
			dxgiFormat = DXGI_FORMAT_R32G32_FLOAT;
			break;
		case TINYGLTF_TYPE_VEC3:
			dxgiFormat = DXGI_FORMAT_R32G32B32_FLOAT;
			break;
		case TINYGLTF_TYPE_VEC4:
			dxgiFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
			break;
		default:
			break;
		}

		return dxgiFormat;
	}

	DXGI_FORMAT GetDxgiUnsignedShortFormat(int numComponents)
	{
		DXGI_FORMAT dxgiFormat = DXGI_FORMAT_UNKNOWN;

		switch (numComponents)
		{
		case TINYGLTF_TYPE_SCALAR:
			dxgiFormat = DXGI_FORMAT_R16_UINT;
			break;
		case TINYGLTF_TYPE_VEC2:
			dxgiFormat = DXGI_FORMAT_R16G16_UINT;
			break;
		default:
			break;
		}

		return dxgiFormat;
	}

	DXGI_FORMAT GltfGetDxgiFormat(int tinyGltfComponentType, int components)
	{
		DXGI_FORMAT dxgiFormat = DXGI_FORMAT_UNKNOWN;

		switch (tinyGltfComponentType)
		{
		case TINYGLTF_COMPONENT_TYPE_FLOAT:
			dxgiFormat = GetDxgiFloatFormat(components);
			break;

		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
			dxgiFormat = GetDxgiUnsignedShortFormat(components);
			break;
		default:
			break;
		}

		return dxgiFormat;
	}

	UINT GetComponentTypeSizeInBytes(UINT componentType)
	{
		return tinygltf::GetComponentSizeInBytes(componentType);
	}

	UINT GetNumComponentsInType(UINT type)
	{
		return tinygltf::GetNumComponentsInType(type);
	}

}


