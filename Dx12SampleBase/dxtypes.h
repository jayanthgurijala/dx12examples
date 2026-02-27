/*
* Copyright (C) 2026 by Jayanth Gurijala
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/
#pragma once

#include <d3d12.h>
#include <vector>
#include <string>
#include <wrl.h>
#include <d3dx12.h>

using namespace Microsoft::WRL;


struct SimpleVertex
{
	FLOAT position[3];
	FLOAT color[3];
	FLOAT texCoords[2];
};

struct DxExtents
{
	FLOAT min[3];
	FLOAT max[3];
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
	DxGltfSampler       samplerInfo;
	DxGltfTextureBuffer imageBufferInfo;
};

struct DxGltfTextureInfo
{
	DxGltfTexture texture;
	UINT          texCoordIndex;
};

struct DxPbrMetallicRoughness
{
	DxGltfTextureInfo baseColorTexture;
	DxGltfTextureInfo metallicRoughnessTexture;
	FLOAT             baseColorFactor[4];
	FLOAT             metallicFactor;
	FLOAT             roughnessFactor;

};

struct DxNormalTextureInfo
{
	DxGltfTextureInfo normalTexture;
	FLOAT scale;
};

struct DxOcclusionTextureInfo
{
	DxGltfTextureInfo occlusionTexture;
	FLOAT strength;
};

struct DxEmissiveTextureInfo
{
	DxGltfTextureInfo emissiveTexture;
	FLOAT emissiveFactor[3];
};

enum DxAlphaMode
{
	DxOpaque,
	DxMask,
	DxBlend
};

struct DxGltfMaterial
{
	std::string            name;
	DxPbrMetallicRoughness pbrMetallicRoughness;
	DxNormalTextureInfo    normalInfo;
	DxOcclusionTextureInfo occlusionInfo;
	DxEmissiveTextureInfo  emissiveInfo;
	DxAlphaMode            alphaMode;
	FLOAT                  alphaCutOff;
	BOOL                   doubleSided;
};

struct DxGltfPrimInfo
{
	std::string                     name;
	std::vector<DxGltfVertexBuffer> vbInfo;
	DxGltfIndexBuffer               ibInfo;
	DxGltfMaterial					materialInfo;
	DxDrawPrimitive                 drawInfo;
	DxExtents                       extents;
};

struct DxMaterialCB
{
	FLOAT baseColorFactor[4];

	FLOAT emissiveFactor[3];
	FLOAT metallicFactor;

	FLOAT roughnessFactor;
	FLOAT normalScale;
	FLOAT occlusionStrength;
	FLOAT alphaCutoff;

	UINT  flags;
	FLOAT padding[3];
};

struct DxMaterialResourceInfo
{
	ComPtr<ID3D12Resource> pbrBaseColorTexture;
	ComPtr<ID3D12Resource> pbrMetallicRoughnessTexture;
	ComPtr<ID3D12Resource> normalTexture;
	ComPtr<ID3D12Resource> occlusionTexture;
	ComPtr<ID3D12Resource> emissiveTexture;

	//@note Resource holds data for all the primitives and it is chunked per primitive
	D3D12_GPU_VIRTUAL_ADDRESS meterialCb;
};

enum MaterialFlags
{
	HasBaseColorTexture = 1 << 0,
	HasNormalTexture = 1 << 1,
	HasMetallicRoughnessTex = 1 << 2,
	HasOcclusionTexture = 1 << 3,
	HasEmissiveTexture = 1 << 4,
	AlphaModeMask = 1 << 5,
	AlphaModeBlend = 1 << 6,
	DoubleSided = 1 << 7
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

struct DxPrimitiveInfo
{
	DxDrawPrimitive                       modelDrawPrimitive;

	std::string                           name;
	std::vector<DxPrimVertexData>         vertexBufferInfo;
	std::vector<D3D12_INPUT_ELEMENT_DESC> modelIaSemantics;
	DxPrimIndexData			              indexBufferInfo;


	ComPtr<ID3D12PipelineState>           pipelineState;
	CD3DX12_ROOT_PARAMETER                descriptorTable;

	DxMaterialCB                          materialCbData;
	DxMaterialResourceInfo                materialTextures;
};

struct DxMeshInfo
{
	std::vector<DxPrimitiveInfo> primitives;
};

struct DxNodeInfo
{
	std::string name;
	BOOL                      hasMeshInfo;
	DxNodeTransformInfo	      transformInfo;
	DxMeshInfo		          meshInfo;
	D3D12_GPU_VIRTUAL_ADDRESS gpuCameraData;
};

struct DxSceneInfo
{
	std::vector<DxNodeInfo> nodes;
	UINT numTotalPrimitivesInScene;
};




