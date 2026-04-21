/*
* Copyright (C) 2026 by Jayanth Gurijala
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/
#include "pch.h"
#include "DxGltfLoader.h"
#include "gltfutils.h"
#include "DxTransformHelper.h"
#include <stack>
#include "WICImageLoader.h"

using namespace GltfUtils;

DxGltfLoader::DxGltfLoader(Dx12SampleBase* pSampleBase) :
	m_pSampleBase(pSampleBase),
	m_supportedAttributes({"POSITION", "NORMAL", "TEXCOORD_0", "TANGENT"}),
	m_modelAssetParsingInfo({})
{
}

HRESULT DxGltfLoader::LoadModel(std::string modelPath)
{
	bool ret = m_loader.LoadASCIIFromFile(&m_model, &m_loadStatus.m_err, &m_loadStatus.m_warn, modelPath);
	PrintUtils::PrintString(m_loadStatus.m_err.c_str());
	assert(ret == true);
	return (ret == TRUE) ? S_OK : E_FAIL;
}

VOID DxGltfLoader::ParsePrimitiveVertexInfo(const tinygltf::Accessor& inGltfAccessorDesc, DxPrimVertexData& outDxVbInfo, const std::string& attributeName)
{
	const UINT componentDataType    = inGltfAccessorDesc.componentType;
	const UINT componentVecType     = inGltfAccessorDesc.type;
	const UINT componentSizeInBytes = GetComponentTypeSizeInBytes(componentDataType);
	const UINT numComponents        = GetNumComponentsInType(componentVecType);

	const int bufferViewIdx = inGltfAccessorDesc.bufferView;
	const tinygltf::BufferView bufViewDesc = GetBufferView(bufferViewIdx);

	

	const DXGI_FORMAT vbFormat = GltfGetDxgiFormat(componentDataType, componentVecType);

	const int    bufferIdx = bufViewDesc.buffer;
	const size_t buflength = bufViewDesc.byteLength;
	const size_t bufOffset = bufViewDesc.byteOffset;

	const size_t accessorByteOffset     = inGltfAccessorDesc.byteOffset;
	const size_t dataOffsetInBuffer     = accessorByteOffset + bufOffset;
	const UINT  bufferStrideInBytes     = componentSizeInBytes * numComponents;
	const UINT  bufferViewStrideInBytes = bufViewDesc.byteStride;
	const UINT  finalStrideInBytes      = ((bufferStrideInBytes == 0) ? bufferViewStrideInBytes : bufferStrideInBytes);

	BYTE* const bufferData = m_model.buffers[bufferIdx].data.data() + dataOffsetInBuffer;
	auto& currentSemantic  = outDxVbInfo.iaSemantic;
	bufViewDesc.target;

	///@todo need to support interleaved data
	//assert(bufViewDesc.byteStride == 0);

	CreateVertexBufferResourceAndView(outDxVbInfo, bufferData, buflength, finalStrideInBytes, attributeName.c_str());

	currentSemantic.format = vbFormat;
	FillIaLayoutInfo(currentSemantic, attributeName);
}

VOID DxGltfLoader::ParsePrimitiveIndexBufferInfo(const tinygltf::Accessor& inGltfAccessorDesc, DxPrimIndexData& outDxIbInfo)
{
	const int bufferViewIdx                 = inGltfAccessorDesc.bufferView;
	const tinygltf::BufferView& bufViewDesc = GetBufferView(bufferViewIdx);

	const size_t accessorByteOffset = inGltfAccessorDesc.byteOffset;
	//assert(accessorByteOffset == 0);

	const size_t bufferViewOffset     = bufViewDesc.byteOffset;
	const size_t bufferSizeInBytes    = bufViewDesc.byteLength;
	const size_t byteOffsetIntoBuffer = accessorByteOffset + bufferViewOffset;

	BYTE* const bufferData        = m_model.buffers[bufViewDesc.buffer].data.data() + byteOffsetIntoBuffer;
	DXGI_FORMAT indexBufferformat = GltfGetDxgiFormat(inGltfAccessorDesc.componentType, inGltfAccessorDesc.type);

	///e.g. accessorDesc.componentType = TINYGLTF_PARAMETER_TYPE_FLOAT (5126)
	///e.g. accessorDesc.type          = "type" : "VEC3"
	UINT componentSizeInBytes = GetComponentTypeSizeInBytes(inGltfAccessorDesc.componentType);
	UINT numComponentsInType  = GetNumComponentsInType(inGltfAccessorDesc.type);
	const UINT bufferStrideInBytes = componentSizeInBytes * numComponentsInType;

	///@note just to make sure it is not invalid
	assert(componentSizeInBytes < 10);
	assert(numComponentsInType < 10);

	CreateIndexBufferResourceAndView(outDxIbInfo, bufferData, bufferSizeInBytes, bufferStrideInBytes, indexBufferformat, "indices");
}

VOID DxGltfLoader::ParsePrimitiveInfo(const tinygltf::Primitive& inGltfPrim, DxPrimitiveInfo& outDxPrimInfo)
{
	UINT indexOfSupportedAttributes = 0;

	///@note block to parse vertex buffers, IA Layout and extents used in camera setup
	{
		outDxPrimInfo.vertexBufferInfo.clear();
		for (auto attrName : m_supportedAttributes)
		{
			auto it = inGltfPrim.attributes.find(attrName);
			if (it != inGltfPrim.attributes.end())
			{
				const std::string attributeName = it->first;
				const int         accessorIdx   = it->second;

				outDxPrimInfo.vertexBufferInfo.emplace_back();

				auto&                     outDxVbInfo  = outDxPrimInfo.vertexBufferInfo.back();
				const tinygltf::Accessor& accessorDesc = GetAccessor(accessorIdx);
				ParsePrimitiveVertexInfo(accessorDesc, outDxVbInfo, attributeName);

				///@note Attributes are filled in order of supported attributes and POSITION is at index 0
				const BOOL isPositionAttrib = (indexOfSupportedAttributes == 0);

				if (isPositionAttrib == TRUE)
				{
					outDxPrimInfo.modelDrawPrimitive.numVertices = accessorDesc.count;
					auto& outDxExtents                           = outDxPrimInfo.primitiveExtents;
					FillPrimitiveExtents(outDxExtents, accessorDesc);
				}
			}
			indexOfSupportedAttributes++;
		}
	}


	///@note fill in index buffer info
	{
		const int accessorIdx = inGltfPrim.indices;
		if (accessorIdx != -1)
		{
		
			auto& outDxIbInfo                       = outDxPrimInfo.indexBufferInfo;
			const tinygltf::Accessor& accessorDesc  = GetAccessor(accessorIdx);

			outDxPrimInfo.modelDrawPrimitive.isIndexedDraw = TRUE;
			outDxPrimInfo.modelDrawPrimitive.numIndices    = accessorDesc.count;

			ParsePrimitiveIndexBufferInfo(accessorDesc, outDxIbInfo);
		}
	}

	//@note load material info
	{
		const int materialIdx = inGltfPrim.material;
		if (materialIdx != -1)
		{
			const tinygltf::Material& materialDesc = GetMaterial(materialIdx);
			outDxPrimInfo.doubleSided              = materialDesc.doubleSided;

			auto& outDxInfo = outDxPrimInfo.materialTextures;

			const auto& pbrRoughnessInfo = materialDesc.pbrMetallicRoughness;
			BOOL hasPbrBaseColor = LoadGltfTextureInfo(outDxInfo.pbrBaseColorTexture, pbrRoughnessInfo.baseColorTexture.index, pbrRoughnessInfo.baseColorTexture.texCoord);
			BOOL hasPbrRoughness = LoadGltfTextureInfo(outDxInfo.pbrMetallicRoughnessTexture, pbrRoughnessInfo.metallicRoughnessTexture.index, pbrRoughnessInfo.metallicRoughnessTexture.texCoord);
			BOOL hasNormalTex    = LoadGltfTextureInfo(outDxInfo.normalTexture, materialDesc.normalTexture.index, materialDesc.normalTexture.texCoord);
			BOOL hasOcclusionTex = LoadGltfTextureInfo(outDxInfo.occlusionTexture, materialDesc.occlusionTexture.index, materialDesc.occlusionTexture.texCoord);
			BOOL hasEmissiveTex  = LoadGltfTextureInfo(outDxInfo.emissiveTexture, materialDesc.emissiveTexture.index, materialDesc.emissiveTexture.texCoord);


			{
				outDxPrimInfo.materialCbData.metallicFactor     = (FLOAT)materialDesc.pbrMetallicRoughness.metallicFactor;
				outDxPrimInfo.materialCbData.roughnessFactor    = (FLOAT)materialDesc.pbrMetallicRoughness.roughnessFactor;
				outDxPrimInfo.materialCbData.baseColorFactor[0] = (FLOAT)materialDesc.pbrMetallicRoughness.baseColorFactor[0];
				outDxPrimInfo.materialCbData.baseColorFactor[1] = (FLOAT)materialDesc.pbrMetallicRoughness.baseColorFactor[1];
				outDxPrimInfo.materialCbData.baseColorFactor[2] = (FLOAT)materialDesc.pbrMetallicRoughness.baseColorFactor[2];
				outDxPrimInfo.materialCbData.baseColorFactor[3] = (FLOAT)materialDesc.pbrMetallicRoughness.baseColorFactor[3];

				outDxPrimInfo.materialCbData.normalScale       = materialDesc.normalTexture.scale;
				outDxPrimInfo.materialCbData.occlusionStrength = materialDesc.occlusionTexture.strength;
				outDxPrimInfo.materialCbData.emissiveFactor[0] = materialDesc.emissiveFactor[0];
				outDxPrimInfo.materialCbData.emissiveFactor[1] = materialDesc.emissiveFactor[1];
				outDxPrimInfo.materialCbData.emissiveFactor[2] = materialDesc.emissiveFactor[2];

				outDxPrimInfo.materialCbData.alphaCutoff       = materialDesc.alphaCutoff;

				auto GetMaterialFlagValue = [](BOOL enabled, MaterialFlags flagValue)
					{
						UINT value = ((enabled == TRUE) ? flagValue : 0);
						return value;
					};

				
				//[DoubleSided | AlphaModeBlend | AlphaModeMask | HasEmissiveTexture | HasOcclusionTexture | HasMetallicRoughnessTex | HasNormalTexture | HasBaseColorTexture]
				const UINT doubleSided    = GetMaterialFlagValue(materialDesc.doubleSided == TRUE, MaterialFlagsDoubleSided);
				const UINT alphaModeBlend = GetMaterialFlagValue(materialDesc.alphaMode == "BLEND", MaterialFlagsAlphaModeBlend);
				const UINT alphaModeMask  = GetMaterialFlagValue(materialDesc.alphaMode == "MASK", MaterialFlagsAlphaModeMask);
				const UINT emissiveTex    = GetMaterialFlagValue(hasEmissiveTex, MaterialFlagsHasEmissiveTexture);
				const UINT occlusionTex   = GetMaterialFlagValue(hasOcclusionTex, MaterialFlagsHasOcclusionTexture);
				const UINT pbrRoughness   = GetMaterialFlagValue(hasPbrRoughness, MaterialFlagsHasMetallicRoughnessTex);
				const UINT normalTex      = GetMaterialFlagValue(hasNormalTex, MaterialFlagsHasNormalTexture);
				const UINT baseColor      = GetMaterialFlagValue(hasPbrBaseColor, MaterialFlagsHasBaseColorTexture);

				outDxPrimInfo.materialCbData.flags = baseColor | normalTex | pbrRoughness | occlusionTex | emissiveTex | alphaModeMask | alphaModeBlend | doubleSided;
			}

			
		}

	}
}



///@note Mesh has name and one or more primitives
VOID DxGltfLoader::ParseMeshInfo(const tinygltf::Mesh& inGltfMesh, DxModelAsset& outDxModelAssetInfo, const XMMATRIX& worldMatrix)
{
	///@todo use mesh name
	//inGltfMesh.name;
	outDxModelAssetInfo.name = inGltfMesh.name;
	for (const auto& inPrim : inGltfMesh.primitives)
	{
		outDxModelAssetInfo.primitives.emplace_back();
		auto& outDxPrim         = outDxModelAssetInfo.primitives.back();
		outDxPrim.meshName      = inGltfMesh.name;
		outDxPrim.worldMatrix   = worldMatrix;
		outDxPrim.primLinearIdxInModelAssets = m_modelAssetParsingInfo.primIndexInModelAsset;
		ParsePrimitiveInfo(inPrim, outDxPrim);
		m_modelAssetParsingInfo.primIndexInModelAsset += 1;
	}

}

VOID DxGltfLoader::ParseNodes(std::stack<std::unique_ptr<GltfNodeTransformInfo>>& nodeList, DxModelAsset& modelAsset)
{
	while (nodeList.empty() == FALSE)
	{
		auto nodeDesc = std::move(nodeList.top());
		nodeList.pop();

		DxTransformInfo nodeTransformInfo;

		///e.g. Deer has 1, Oaktree has 2, Lantern has 1 but that node has children nodes.
		///     Node hierarchy starts here with each node optionally having
		///        - TransformInfo
		///        - Mesh
		///        - Children
		///    Everything above is optional, Node can just be a container without its own mesh.
		const BOOL nodeHasMesh = (nodeDesc->nodeInfo->mesh != -1);
		const BOOL nodeHasChildren = (nodeDesc->nodeInfo->children.size() > 1);


		if (nodeHasMesh)
		{
			auto& gltfMesh = m_model.meshes[nodeDesc->nodeInfo->mesh];
			ParseMeshInfo(gltfMesh, modelAsset, nodeDesc->nodeTransformWorldMatrix);
		}

		if (nodeHasChildren)
		{
			const UINT numChildren = nodeDesc->nodeInfo->children.size();
			for (UINT i = 0; i < numChildren; i++)
			{
				UINT nodeIdx   = nodeDesc->nodeInfo->children[i];
				auto* childNodeDesc = &m_model.nodes[nodeIdx];
				PushNodeTransformInfo(nodeList, childNodeDesc);
			}
		}
	}
}


/*
* DxModelAsset mainly has a list of primitives
* Primitives need to be populated with VBs, IBs, MaterialInfo and flattened transform info
*/
VOID DxGltfLoader::LoadGltfModelAsset(DxModelAsset& modelAsset)
{
	//@todo support multiple scenes
	assert(m_model.scenes.size() == 1);

	auto& currentScene = m_model.scenes[0];
	const UINT numNodesInScene = currentScene.nodes.size();

	std::stack<std::unique_ptr<GltfNodeTransformInfo>> nodeList;

	for (UINT i = 0; i < numNodesInScene; i++)
	{
		///@note Scene has a list of nodes, node at 0th index can be 3rd in nodes array.
		const UINT nodeIdx  = currentScene.nodes[i];
		auto*      nodeDesc = &m_model.nodes[nodeIdx];
		PushNodeTransformInfo(nodeList, nodeDesc);
	}
	ParseNodes(nodeList, modelAsset);
}

UINT DxGltfLoader::NumScenes()
{
	assert(m_model.scenes.size() == 1);
	return 1;
}

BOOL DxGltfLoader::GetNodeTransformInfo(DxTransformInfo& transformInfo, tinygltf::Node* nodeDesc)
{

	BOOL hasScale       = (nodeDesc->scale.size() != 0);
	BOOL hasTranslation = (nodeDesc->translation.size() != 0);
	BOOL hasRotation    = (nodeDesc->rotation.size() != 0);
	BOOL hasMatrix      = (nodeDesc->matrix.size() != 0);

	///@todo returning identity if no transform, need to optimize
	///      Challenge: see ParseNodes
	BOOL hasTransform = TRUE;//(hasScale || hasTranslation || hasRotation || hasMatrix);

	
	if (hasTransform)
	{
		//need to test this path
		assert(hasMatrix == false);
		transformInfo.hasMatrix = hasMatrix;
		DxTransformHelper::SetTranslation(transformInfo, nodeDesc->translation, hasTranslation);
		DxTransformHelper::SetScale(transformInfo, nodeDesc->scale, hasScale);
		DxTransformHelper::SetQuaternionRotation(transformInfo, nodeDesc->rotation, hasRotation);
	}

	return hasTransform;
}

VOID DxGltfLoader::ParseSamplerDescription(const int samplerIdx, D3D12_SAMPLER_DESC& outdxSamplerDesc)
{
	///@todo implement when used
	//if (samplerIdx != -1)
	//{
	//	const tinygltf::Sampler& inGltfSamplerDesc = GetSampler(samplerIdx);
	//
	//	outdxSamplerDesc.Filter = 
	//
	//	samplerDesc.minFilter = samplerDesc.minFilter;
	//	samplerDesc.magFilter = samplerDesc.magFilter;
	//	samplerDesc.wrapS = samplerDesc.wrapS;
	//	samplerDesc.wrapT = samplerDesc.wrapT;
	//}
	//else
	//{
	//	// Set default sampler values if not specified
	//	dxTextureInfo.texture.samplerInfo.minFilter = TINYGLTF_TEXTURE_FILTER_LINEAR;
	//	dxTextureInfo.texture.samplerInfo.magFilter = TINYGLTF_TEXTURE_FILTER_LINEAR;
	//	dxTextureInfo.texture.samplerInfo.wrapS = TINYGLTF_TEXTURE_WRAP_REPEAT;
	//	dxTextureInfo.texture.samplerInfo.wrapT = TINYGLTF_TEXTURE_WRAP_REPEAT;
	//}
}

BOOL DxGltfLoader::LoadGltfTextureInfo(DxTextureSamplerInfo& dxTextureInfo, int textureIndex, int texcoord)
{
	dxTextureInfo = {};

	BOOL isTextureValid = FALSE;

	if (textureIndex != -1)
	{
		const tinygltf::Texture& textureInfo = GetTexture(textureIndex);
		const int samplerIdx = textureInfo.sampler;
		ParseSamplerDescription(samplerIdx, dxTextureInfo.samplerInfo);

		const int imageIdx = textureInfo.source;

		// Load image buffer info
		if (imageIdx != -1)
		{
			const tinygltf::Image& imageInfo = GetImage(imageIdx);

			std::string imageName = imageInfo.name;
			std::string mimeType  = imageInfo.mimeType;

			BYTE* bufferData          = nullptr;
			SIZE_T bufferSize         = 0;
			std::string fullAssetPath = {};

			if (imageInfo.bufferView != -1)
			{
				const tinygltf::BufferView& bufViewDesc = GetBufferView(imageInfo.bufferView);
				assert(bufViewDesc.byteStride == 0);

				const int    bufferIdx  = bufViewDesc.buffer;
				const SIZE_T byteOffset = bufViewDesc.byteOffset;

				bufferSize = bufViewDesc.byteLength;
				bufferData = m_model.buffers[bufferIdx].data.data() + byteOffset;
			}
			else
			{
				assert(imageInfo.uri.c_str() != nullptr);
				fullAssetPath = m_pSampleBase->GetFullPathForAsset(imageInfo.uri);
			}

			WICImageLoader::ImageData imageData = WICImageLoader::LoadImageFromMemory_WIC(bufferData, bufferSize, fullAssetPath.c_str());

			dxTextureInfo.textureInfo = m_pSampleBase->CreateTexture2DWithData(imageData.pixels.data(),
																			   imageData.pixels.size(),
																			   imageData.width,
																			   imageData.height,
																			   DXGI_FORMAT_R8G8B8A8_UNORM);

			isTextureValid = TRUE;
		}
	}
	return isTextureValid;
}







