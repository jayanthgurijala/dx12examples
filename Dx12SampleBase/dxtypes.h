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

#include <DirectXMath.h>


using namespace Microsoft::WRL;
using namespace DirectX;

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

struct DxTextureSamplerInfo
{
	ComPtr<ID3D12Resource> textureInfo;
	D3D12_SAMPLER_DESC     samplerInfo;
};

struct DxMaterialResourceInfo
{
	DxTextureSamplerInfo pbrBaseColorTexture;
	DxTextureSamplerInfo pbrMetallicRoughnessTexture;
	DxTextureSamplerInfo normalTexture;
	DxTextureSamplerInfo occlusionTexture;
	DxTextureSamplerInfo emissiveTexture;

	//@note Resource holds data for all the primitives and it is chunked per primitive
	D3D12_GPU_VIRTUAL_ADDRESS meterialCb;

	//Descriptor heap is organized as numSrvsPerPrim per Primitive in the order of scene elements loaded.
	//e.g. 1) terrain_gridlines, 2) deer, 3) chinesedragon, 4) oaktree
	//     [ 5 SRVs for (1) | 5 SRVs for (2) | 5 + 5 SRVs for (3) | 5 + 5 SRVs for oaktree ]
	//     0                5                10                  20
	//Base index is needed in scene load element as each loaded element can have N prims and M nodes
	UINT descriptorHeapOffset;
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
	DxIASemantic             iaSemantic;
};

struct DxPrimIndexData
{
	ComPtr<ID3D12Resource>        indexBuffer;
	D3D12_INDEX_BUFFER_VIEW       modelIbv;
};

struct DxPrimitiveInfo
{
	DxDrawPrimitive               modelDrawPrimitive;
								  
	std::string                   name;
	std::vector<DxPrimVertexData> vertexBufferInfo;
	DxPrimIndexData			      indexBufferInfo;
								  
	ComPtr<ID3D12PipelineState>   pipelineState;
	//CD3DX12_ROOT_PARAMETER        descriptorTable;
								  
	DxMaterialCB                  materialCbData;
	DxMaterialResourceInfo        materialTextures;
	DxExtents                     meshExtents;
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
};

struct DxSceneElements
{
	std::vector<DxNodeInfo> nodes;
	UINT numTotalPrimitivesInScene;
};


struct DxSceneElementTRS
{
	FLOAT translation[3];
	FLOAT rotationInDegrees[3];
	FLOAT scale[3];
};

struct DxSceneElementInstance
{
	UINT sceneElementIdx;
	UINT numInstances;
	std::vector<DxSceneElementTRS> trsMatrix;
	BOOL addToExtents;
};

struct DxSceneLoadInfo
{
	UINT sceneElementIdx;
	//A scene element can have > 1 nodes. 
	//e.g. A oaktree has 2 nodes and say 100 instances
	//Need to store 200 GPUVAs organized as gpuVA_1_node1, gpuVA_1_node2 .... gpuVA_100_node1, gpuVA_100_node2
	UINT numInstances;

	std::vector<D3D12_GPU_VIRTUAL_ADDRESS> instanceCameraGpuVa;
};

struct DxCBSceneData
{
	XMFLOAT4X4 viewProjMatrix;
	XMFLOAT4X4 invViewProj;
	XMFLOAT4   cameraPosition;
	XMFLOAT4   cameraFovY;
};

struct DxCBPerInstanceData
{
	XMFLOAT4X4 modelMatrix;
	XMFLOAT4X4 normalMatrix;
};

struct DxSceneInfo
{
	std::vector<DxSceneLoadInfo> sceneLoadInfo;
};







