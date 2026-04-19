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
#include <map>

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

enum DxRotationMode
{
	DxRotationModeInvalid,
	DxRotationModeQuaternion,
	DxRotationModeDegree,
	DxRotationModeRadians
};

struct DxRotationQuaternion
{
	FLOAT x;
	FLOAT y;
	FLOAT z;
	FLOAT w;
};

struct DxRotationDegrees
{
	FLOAT xDeg;
	FLOAT yDeg;
	FLOAT zDeg;
};

struct DxRotationRadians
{
	FLOAT xRad;
	FLOAT yRad;
	FLOAT zRAD;
};

struct DxTRSInfo
{
	FLOAT translation[3];
	FLOAT scale[3];
	DxRotationMode rotationMode;
	union
	{
		DxRotationQuaternion quaterion;
		DxRotationDegrees    degree;
		DxRotationRadians    radians;
	};
};

struct DxTransformInfo
{
	BOOL hasMatrix;
	union
	{
		DxTRSInfo trsInfo;
		FLOAT     matrix[16];
	};
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

	UINT flags;
	FLOAT padding[2];
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
	std::string name;
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
	MaterialFlagsHasBaseColorTexture     = 1 << 0,
	MaterialFlagsHasNormalTexture        = 1 << 1,
	MaterialFlagsHasMetallicRoughnessTex = 1 << 2,
	MaterialFlagsHasOcclusionTexture     = 1 << 3,
	MaterialFlagsHasEmissiveTexture      = 1 << 4,
	MaterialFlagsAlphaModeMask           = 1 << 5,
	MaterialFlagsAlphaModeBlend          = 1 << 6,
	MaterialFlagsDoubleSided             = 1 << 7
};

enum SceneRenderFlags
{
	RenderFlagsUsePBR      = 1 << 0,
	RenderFlagsTessEnabled = 1 << 1
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


///@note PrimitiveInfo has TransformInfo as design is to flatten the gltf information.
///      In simple cases, it made sense to have a hierarchy, but if a node has a child,
///      it gets complicated.
///      There could be redundant data, need to address this later.
struct DxPrimitiveInfo
{
	DxDrawPrimitive               modelDrawPrimitive;
	std::string                   meshName;
	std::vector<DxPrimVertexData> vertexBufferInfo;
	DxPrimIndexData			      indexBufferInfo;	  
	ComPtr<ID3D12PipelineState>   pipelineState; 
	DxMaterialCB                  materialCbData;
	DxMaterialResourceInfo        materialTextures;
	DxExtents                     primitiveExtents;
	//DxTransformInfo	              transformInfo;
	XMMATRIX                      worldMatrix;
	BOOL                          doubleSided;

	///@note this is required to index into descriptor heap
	UINT                          primLinearIdxInModelAssets;
};

///@note Change to DxModelAsset, it should have all the info
///      e.g. Oaktree and ChineseDragon should have flattened 2 primitives
///           Lantern should also have flattened prims
struct DxModelAsset
{
	std::string name;
	std::vector<DxPrimitiveInfo> primitives;
};


///@note Collection of assets from multiple files.
///      This should not be flattened because scene description granularity
///      is model asset.
struct DxModelAssets 
{
	std::vector<DxModelAsset> modelAssetList;
	UINT numTotalPrimitivesInAllAssets;
};

struct DxSceneElementInstance
{
	UINT sceneElementIdx;
	UINT numInstances;
	std::vector<DxTransformInfo> trsMatrix;
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

	UINT       renderflags;
	FLOAT      padding[3];
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

struct DxModelAssetBlasInfo
{
	UINT linearBlasFirstPrimIndex;
	std::vector<DxASDesc> modelAssetBlas;
};

///@note In present implementation, each primitive is a node with a transform
///      But we do need to group them together as an asset used while scene construction
struct DxSceneBlasDesc
{
	std::vector<DxModelAssetBlasInfo> modelAssetBlas;
};


struct DxSceneTlasDesc
{
	DxASDesc sceneTlas;
};
