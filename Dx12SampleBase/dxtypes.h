/*
* Copyright (C) 2026 by Jayanth Gurijala
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/
#pragma once

#include <d3d12.h>
#include <vector>
#include <string>
#include <wrl.h>

using namespace Microsoft::WRL;


struct SimpleVertex
{
	FLOAT position[3];
	FLOAT color[3];
	FLOAT texCoords[2];
};

struct DxExtents
{
	double min[3];
	double max[3];
	BOOL hasValidExtents;
};

struct DxNodeTransformInfo
{
	BOOL hasTranslation;
	BOOL hasRotation;
	BOOL hasScale;
	BOOL hasMatrix;

	std::vector<double> translation;
	std::vector<double> rotation;
	std::vector<double> scale;
	std::vector<double> matrix;
};

struct DxIASemantic
{
	std::string name;
	UINT        index;
	BOOL        isIndexValid;
	DXGI_FORMAT format;
};

struct DxDrawPrimitive
{
	UINT numVertices = 0;
	UINT numIndices = 0;

	//@remove
	BOOL isIndexedDraw = FALSE;
};

enum DxAppFrameType
{
	DxFrameRTVIndex = 0,
	DxFrameResource = 1,
	DxFrameInvalid  = 2
};

struct DxAppFrameInfo
{
	UINT rtvIndex;
	ID3D12Resource* pFrameResource;
	D3D12_RESOURCE_STATES pResState;
	DxAppFrameType type;
};



struct DxGltfBuffer
{
	UINT         bufferIndex;
	UINT64       bufferOffsetInBytes;
	UINT	     bufferStrideInBytes;
	UINT64       bufferSizeInBytes;
};


struct DxGltfVertexBuffer : public DxGltfBuffer
{
	DxIASemantic iaLayoutInfo;
};

struct DxGltfIndexBuffer : public DxGltfBuffer
{
	std::string name;
	DXGI_FORMAT indexFormat;
};

struct DxGltfTextureBuffer : public DxGltfBuffer
{
	std::string mimeType;
	std::string name;
};

struct DxGltfSampler
{
	UINT minFilter;
	UINT magFilter;
	UINT wrapS;
	UINT wrapT;
};


struct DxGltfTexture
{
	DxGltfSampler samplerInfo;
	DxGltfTextureBuffer imageBufferInfo;
};

struct DxGltfTextureInfo
{
	DxGltfTexture texture;
	UINT texCoordIndex;
};

struct DxPbrMetallicRoughness
{
	DxGltfTextureInfo baseColorTexture;
	FLOAT metallicFactor;
	FLOAT roughnessFactor;
};

struct DxGltfMaterial
{
	BOOL        isMaterialDefined;
	std::string name;
	BOOL        doubleSided;
	DxPbrMetallicRoughness pbrMetallicRoughness;
};

struct DxGltfPrimInfo
{
	std::string                     name;
	std::vector<DxGltfVertexBuffer> vbInfo;
	DxGltfIndexBuffer               ibInfo;
	DxGltfMaterial					materialInfo;
	DxDrawPrimitive                 drawInfo;
};

struct DxPrimVertexData
{
	ComPtr<ID3D12Resource>   modelVbBuffer;
	D3D12_VERTEX_BUFFER_VIEW modelVbv;
	std::string              semanticName;
};

struct DxPrimIndexData
{
	ComPtr<ID3D12Resource>        indexBuffer;
	D3D12_INDEX_BUFFER_VIEW       modelIbv;
};

struct DxPrimMaterialInfo
{
	DxGltfMaterial         materialInfo;
	ComPtr<ID3D12Resource> modelBaseColorTex2D;
};


struct DxPrimitiveInfo
{
	std::string                           name;
	std::vector<DxPrimVertexData>         vertexBufferInfo;
	std::vector<D3D12_INPUT_ELEMENT_DESC> modelIaSemantics;
	DxPrimIndexData			              indexBufferInfo;
	DxPrimMaterialInfo			          materialInfo;
	DxDrawPrimitive                       modelDrawPrimitive;

};

struct DxMeshInfo
{
	std::vector<DxPrimitiveInfo> primitives;
};

struct DxNodeInfo
{
	std::string name;
	DxNodeTransformInfo	      transformInfo;
	DxMeshInfo		          meshInfo;
	D3D12_GPU_VIRTUAL_ADDRESS gpuCameraData;
};

struct DxSceneInfo
{
	std::vector<DxNodeInfo> nodes;
};




