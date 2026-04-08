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
	std::string uri;
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


///@todo remove this and use a map
enum GltfVertexAttribIndex
{
	GltfVertexAttribPosition  = 0,
	GltfVertexAttribNormal    = 1,
	GltfVertexAttribTexcoord0 = 2,
	GltfVertexAttribMax       = UINT_MAX
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

struct DxLightDataCB
{
	FLOAT lightDirection[3];
    FLOAT lightColor[3];
	FLOAT lightPosition[3];
    FLOAT lightIntensity;
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
	ComPtr<ID3D12Resource>  indexBuffer;
	D3D12_INDEX_BUFFER_VIEW modelIbv;
	UINT bufferStrideInBytes;
};

struct DxPrimitiveInfo
{
	DxDrawPrimitive               modelDrawPrimitive;
	std::string                   name;
	std::vector<DxPrimVertexData> vertexBufferInfo;
	DxPrimIndexData			      indexBufferInfo;	  
	ComPtr<ID3D12PipelineState>   pipelineState; 
	DxMaterialCB                  materialCbData;
	DxMaterialResourceInfo        materialTextures;
	DxExtents                     meshExtents;

	///@note this is required to index into descriptor heap
	UINT                          primLinearIdxInSceneElements;
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

struct DxSizeAlignedSize
{
	UINT   count;
	UINT64 size;
	UINT64 alignedSize;
	UINT64 totalAlignedSize;

public:
	inline VOID CalcTotalSize()
	{
		totalAlignedSize = alignedSize * count;
	}
};

enum DxRangeTypeSrvUavCbv
{
	DxRangeTypeRtvAsSrv = 0,
	DxRangeTypeDsvAsSrv,
	DxRangeTypeUav,
	DxRangeTypeUavAsSrv,
	DxRangeTypePerPrimSrv,
	DxRangeTypeMax
};

enum DxDescriptorType
{
	DxDescriptorTypeRtvSrv = 0,
	DxDescriptorTypeDsvSrv = 1,
	DxDescriptorTypeUavSrv = 2,
	DxDescriptorTypeMax
};



struct DxResourceDescriptorInfo
{
	ID3D12Resource* pResource;
	BOOL isActive;
};

struct DxDescriptorHeap
{
	ComPtr<ID3D12DescriptorHeap> descriptorHeap = nullptr;
	UINT descriptorSize = 0;
	UINT descriptorCount = 0;
	std::vector<DxResourceDescriptorInfo> resourceDescInfo;
};

struct DxSrvUavCbvOffsetsInfo
{
	UINT srvForRtvStartOffset     = 0;
	UINT srvForDsvStartOffset     = 0;
	UINT uavStartOffset           = 0;
	UINT srvForUavStartOffset     = 0;
	UINT perPrimitiveSrvOffset    = 0;
	UINT appSrvOffsetInPerPrimSrv = 0;

	UINT numPerPrimSrvs           = 0;
	UINT numSrvsForPbrPerPrim     = 5;
};

struct DxAppFrameInfo
{
	DxAppFrameInfo(DxDescriptorType type, UINT heapOffset)
	{
		this->descriptorType = type;
		this->heapOffset     = heapOffset;
	}
	DxDescriptorType descriptorType;
	UINT             heapOffset;
};

struct DxASDesc
{
	ComPtr<ID3D12Resource> resultBuffer;
	ComPtr<ID3D12Resource> scratchBuffer;
};

///@note Every gltf primitive needs a D3D12_RAYTRACING_GEOMETRY_DESC e.g. (1) Just the deer (2) Oak Tree - bark and leaves
///      Each gltf mesh needs a BLAS, containing multiple geometries.
/// 
struct DxSceneBlasDesc
{
	//Complete objects like Deer, OakTree terrain etc
	std::vector<DxASDesc> sceneElementsBlas;
};


struct DxSceneTlasDesc
{
	DxASDesc sceneTlas;
};
