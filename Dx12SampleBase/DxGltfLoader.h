/*
* Copyright (C) 2026 by Jayanth Gurijala
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/
#pragma once

#include <string>
#include "dxtypes.h"
#include "tiny_gltf.h"
#include "DxPrintUtils.h"
#include <queue>
#include "Dx12SampleBase.h"


struct DxGltfStatus
{
	std::string        m_err;
	std::string        m_warn;
};

struct GltfParsingInfo
{
	UINT primIndexInModelAsset;
};

class DxGltfLoader
{
public:
	DxGltfLoader(Dx12SampleBase* pSampleBase);
	
	VOID LoadGltfModelAsset(DxModelAsset& modelAsset);

	HRESULT LoadModel(std::string modelPath);
	UINT NumScenes();
	
	BOOL GetNodeTransformInfo(DxTransformInfo& transformInfo, tinygltf::Node* nodeDesc);

	inline unsigned char* GetBufferData(UINT index)
	{
		return m_model.buffers[index].data.data();
	}

	inline BOOL IsNodeMeshInfoValid(UINT nodeIndex)
	{
		return (GetNode(nodeIndex).mesh != -1);
	}

	inline UINT NumPrimitives(UINT nodeIndex)
	{
		return GetMesh(nodeIndex).primitives.size();
	}

	inline std::string GetNodeName(UINT nodeIndex)
	{
		return GetNode(nodeIndex).name;
	}

	inline UINT NumNodesInScene()
	{
		return m_model.scenes[0].nodes.size();
	}

private:


	///@note deprecate, this is too complex, need to simplify and directly Get the Node with plain index.
	inline tinygltf::Node& GetNode(UINT nodeIndex)
	{

		return m_model.nodes[
			m_model.scenes[0].nodes[nodeIndex]
		];
	}

	inline tinygltf::Node& GetNodeWithIndex(UINT nodeIndex)
	{
		return m_model.nodes[nodeIndex];
	}

	inline UINT GetNodeIndexInScene(UINT idx)
	{
		///@todo support multiple scenes
		assert(m_model.scenes.size() == 1);

		const UINT nodeIdx = m_model.scenes[0].nodes[idx];
		return nodeIdx;
	}

	inline UINT GetChildNodeIndex(UINT nodeIdx, UINT idx)
	{
		auto& nodeDesc = GetNodeWithIndex(nodeIdx);

		assert(nodeDesc.children.size() < idx);
		return nodeDesc.children[idx];
	}

	inline tinygltf::Mesh& GetMesh(UINT nodeIndex)
	{
		auto& node = GetNode(nodeIndex);
		int meshIdx =node.mesh;
		return m_model.meshes[meshIdx];
	}

	inline tinygltf::Primitive& GetPrimitive(UINT nodeIndex, UINT primitiveIndex)
	{
		return GetMesh(nodeIndex).primitives[primitiveIndex];
	}

	inline tinygltf::Accessor& GetAccessor(UINT index)
	{
		return m_model.accessors[index];
	}

	inline tinygltf::BufferView& GetBufferView(UINT index)
	{
		return m_model.bufferViews[index];
	}

	inline tinygltf::Material& GetMaterial(UINT index)
	{
		return m_model.materials[index];
	}

	inline tinygltf::Texture& GetTexture(UINT index)
	{
		return m_model.textures[index];
	}

	inline tinygltf::Image& GetImage(UINT index)
	{
		return m_model.images[index];
	}

	inline tinygltf::Sampler& GetSampler(UINT index)
	{
		return m_model.samplers[index];
	}

	inline VOID FillPrimitiveExtents(DxExtents& outDxExtents, const tinygltf::Accessor& accessorDesc)
	{
		outDxExtents.hasValidExtents = TRUE;

		outDxExtents.min[0] = (FLOAT)accessorDesc.minValues[0];
		outDxExtents.min[1] = (FLOAT)accessorDesc.minValues[1];
		outDxExtents.min[2] = (FLOAT)accessorDesc.minValues[2];
		outDxExtents.max[0] = (FLOAT)accessorDesc.maxValues[0];
		outDxExtents.max[1] = (FLOAT)accessorDesc.maxValues[1];
		outDxExtents.max[2] = (FLOAT)accessorDesc.maxValues[2];
	}

	inline VOID CreateVertexBufferResourceAndView(DxPrimVertexData& outDxPrim, BYTE* const data, SIZE_T bufferSizeInBytes, UINT strideInBytes, const char* name)
	{
		outDxPrim.modelVbBuffer           = m_pSampleBase->CreateBufferWithData(data, bufferSizeInBytes, name);
		outDxPrim.modelVbv.BufferLocation = outDxPrim.modelVbBuffer->GetGPUVirtualAddress();
		outDxPrim.modelVbv.SizeInBytes    = bufferSizeInBytes;
		outDxPrim.modelVbv.StrideInBytes  = strideInBytes;
	}

	inline VOID CreateIndexBufferResourceAndView(DxPrimIndexData& outDxIbInfo, BYTE* const data, SIZE_T bufferSizeInBytes, UINT strideInBytes, DXGI_FORMAT format, const char* name)
	{
		outDxIbInfo.indexBuffer             = m_pSampleBase->CreateBufferWithData(data, bufferSizeInBytes, name);
		outDxIbInfo.bufferStrideInBytes     = strideInBytes;
		outDxIbInfo.modelIbv.BufferLocation = outDxIbInfo.indexBuffer->GetGPUVirtualAddress();
		outDxIbInfo.modelIbv.Format         = format;
		outDxIbInfo.modelIbv.SizeInBytes    = bufferSizeInBytes;
	}

	inline VOID FillIaLayoutInfo(DxIASemantic& iaSemantic, std::string attributeName)
	{
		iaSemantic.name = attributeName;
		///@todo make a string utils class to check for stuff
		auto pos = attributeName.find("_");

		if (pos != std::string::npos)
		{
			std::string left  = attributeName.substr(0, pos);
			std::string right = attributeName.substr(pos + 1);
			int index         = std::stoi(right);
			iaSemantic.name   = left;
			iaSemantic.index  = index;
			iaSemantic.isIndexValid = TRUE;
		}
		else
		{
			iaSemantic.name = attributeName;
			iaSemantic.isIndexValid = FALSE;
		}
	}

	BOOL LoadGltfTextureInfo(DxTextureSamplerInfo& dxTextureInfo, int textureIndex, int texcoord);


	VOID ParseNodes(std::queue<tinygltf::Node*>& nodeList, DxModelAsset& modelAsset);
	VOID ParseMeshInfo(const tinygltf::Mesh& inGltfMesh, DxModelAsset& outDxModelAssetInfo, DxTransformInfo& nodeTransformInfo);
	VOID ParsePrimitiveInfo(const tinygltf::Primitive& inGltfPrim, DxPrimitiveInfo& outDxPrimInfo);
	VOID ParsePrimitiveVertexInfo(const tinygltf::Accessor& inGltfAccessorDesc, DxPrimVertexData& outDxVbInfo, const std::string& attributeName);
	VOID ParsePrimitiveIndexBufferInfo(const tinygltf::Accessor& inGltfAccessorDesc, DxPrimIndexData& outDxIbInfo);
	VOID ParseSamplerDescription(const int samplerIdx, D3D12_SAMPLER_DESC& samplerDesc);

	std::vector<std::string> m_supportedAttributes;

	tinygltf::Model    m_model;
	tinygltf::TinyGLTF m_loader;

	DxGltfStatus    m_loadStatus;
	Dx12SampleBase* m_pSampleBase;

	GltfParsingInfo m_modelAssetParsingInfo;
};


