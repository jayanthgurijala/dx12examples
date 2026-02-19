/*
* Copyright (C) 2026 by Jayanth Gurijala
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/
#include "pch.h"
#include "DxGltfLoader.h"
#include "gltfutils.h"

using namespace GltfUtils;

DxGltfLoader::DxGltfLoader(std::string modelPath) :
	m_modelPath(modelPath),
	m_supportedAttributes({"POSITION", "NORMAL", "TEXCOORD_0", "TEXCOORD_1", "TEXCOORD_2"})
{
}

HRESULT DxGltfLoader::LoadModel()
{
	bool ret = m_loader.LoadASCIIFromFile(&m_model, &m_loadStatus.m_err, &m_loadStatus.m_warn, m_modelPath);
	return (ret == TRUE) ? S_OK : E_FAIL;
}

UINT DxGltfLoader::NumScenes()
{
	return m_model.scenes.size();
}

UINT DxGltfLoader::NumNodesInScene(UINT index)
{
	return m_model.scenes[index].nodes.size();
}

VOID DxGltfLoader::GetNodeTransformInfo(DxNodeTransformInfo& meshTransformInfo, UINT sceneIndex, UINT nodeIndex)
{
	tinygltf::Node& nodeDesc = GetNode(sceneIndex, nodeIndex);

	meshTransformInfo.hasScale       = (nodeDesc.scale.size() != 0);
	meshTransformInfo.hasTranslation = (nodeDesc.translation.size() != 0);
	meshTransformInfo.hasRotation    = (nodeDesc.rotation.size() != 0);
	meshTransformInfo.hasMatrix      = (nodeDesc.matrix.size() != 0);
	meshTransformInfo.translation    = nodeDesc.translation;
	meshTransformInfo.rotation       = nodeDesc.rotation;
	meshTransformInfo.scale          = nodeDesc.scale;
}


/*
* 
* Loads
* 1. IALayout - POSITION (always index 0), TEXCOORD_0 becomes TEXCOORD 0 etc, format e.g. R32G32B32_FLOAT
* 2. Vertex Buffers - (size, stride) (bufferIndex, Offset)
* 3. Index Buffers
*/
VOID DxGltfLoader::LoadMeshPrimitiveInfo(DxGltfMeshPrimInfo& meshInfo, UINT sceneIndex, UINT nodeIndex, UINT primitiveIndex)
{
	const tinygltf::Primitive& primitive = GetPrimitive(sceneIndex, nodeIndex, primitiveIndex);
	const UINT totalAttributesInPrimitive = min(m_supportedAttributes.size(), primitive.attributes.size());
	meshInfo.vbInfo.resize(totalAttributesInPrimitive);

	//@note load vertex buffers
	{
		UINT attrIndx = 0;
		SIZE_T numTotalVertices = 0;
		auto attributeIt = m_supportedAttributes.begin();
		while (attributeIt != m_supportedAttributes.end())
		{
			auto it = primitive.attributes.find(*attributeIt);
			if (it != primitive.attributes.end())
			{
				const std::string attributeName        = it->first;
				const int accessorIdx                  = it->second;
				const tinygltf::Accessor& accessorDesc = GetAccessor(accessorIdx);
				const UINT componentDataType           = accessorDesc.componentType;
				const UINT componentVecType            = accessorDesc.type;
				const UINT componentSizeInBytes        = GetComponentTypeSizeInBytes(componentDataType);
				const UINT numComponents               = GetNumComponentsInType(componentVecType);

				//@note for POSITION attriubte is always at zero index, placed at 0 in supported attributes
				if (attrIndx == 0)
				{
					numTotalVertices += accessorDesc.count;
				}
				const int bufferViewIdx = accessorDesc.bufferView;
				const tinygltf::BufferView bufViewDesc = GetBufferView(bufferViewIdx);

				///@todo need to support interleaved data
				assert(bufViewDesc.byteStride == 0);

				const int    bufferIdx          = bufViewDesc.buffer;
				const size_t buflength          = bufViewDesc.byteLength;
				const size_t bufOffset          = bufViewDesc.byteOffset;
				const size_t accessorByteOffset = accessorDesc.byteOffset;

				meshInfo.vbInfo[attrIndx].iaLayoutInfo.format = GltfGetDxgiFormat(componentDataType, componentVecType);
				meshInfo.vbInfo[attrIndx].bufferIndex         = bufferIdx;
				meshInfo.vbInfo[attrIndx].bufferOffsetInBytes   = accessorByteOffset + bufOffset;
				meshInfo.vbInfo[attrIndx].bufferSizeInBytes   = buflength;
				meshInfo.vbInfo[attrIndx].bufferStrideInBytes = componentSizeInBytes * numComponents;

				auto& currentSemantic = meshInfo.vbInfo[attrIndx].iaLayoutInfo;
				///@todo make a string utils class to check for stuff
				auto pos = attributeName.find("_");

				if (pos != std::string::npos)
				{
					std::string left = attributeName.substr(0, pos);
					std::string right = attributeName.substr(pos + 1);
					int index = std::stoi(right);
					currentSemantic.name = left;
					currentSemantic.index = index;
					currentSemantic.isIndexValid = TRUE;
				}
				else
				{
					currentSemantic.name = attributeName;
					currentSemantic.isIndexValid = FALSE;
				}
				attrIndx++;
			}
			attributeIt++;
		}
		meshInfo.drawInfo.numVertices = (UINT)(numTotalVertices);
	}


	//@note load index buffer
	{
		const int accessorIdx = primitive.indices;
		if (accessorIdx != -1)
		{
			const tinygltf::Accessor& accessorDesc  = GetAccessor(accessorIdx);
			const int bufferViewIdx                 = accessorDesc.bufferView;
			const tinygltf::BufferView& bufViewDesc = GetBufferView(bufferViewIdx);

			const size_t accessorByteOffset = accessorDesc.byteOffset;
			assert(accessorByteOffset == 0);

			const size_t bufferViewOffset     = bufViewDesc.byteOffset;
			const size_t bufferSizeInBytes    = bufViewDesc.byteLength;
			const size_t byteOffsetIntoBuffer = accessorByteOffset + bufferViewOffset;

			meshInfo.ibInfo.indexFormat       = GltfGetDxgiFormat(accessorDesc.componentType, accessorDesc.type);
			meshInfo.ibInfo.bufferIndex       = bufViewDesc.buffer;
			meshInfo.ibInfo.bufferOffsetInBytes = byteOffsetIntoBuffer;
			meshInfo.ibInfo.bufferSizeInBytes = bufferSizeInBytes;
			meshInfo.drawInfo.numIndices      = (UINT)accessorDesc.count;
			meshInfo.drawInfo.isIndexedDraw = TRUE;
		}
		else
		{
			meshInfo.drawInfo.isIndexedDraw = FALSE;
		}
	}

	//@note load material info
	{
		const int materialIdx = primitive.material;
		if (materialIdx != -1)
		{
			const tinygltf::Material& materialDesc = GetMaterial(materialIdx);
			meshInfo.materialInfo.name             = materialDesc.name;
			meshInfo.materialInfo.doubleSided      = materialDesc.doubleSided;

			auto& pbrRoughness = meshInfo.materialInfo.pbrMetallicRoughness;
			pbrRoughness.metallicFactor                 = (FLOAT)materialDesc.pbrMetallicRoughness.metallicFactor;
			pbrRoughness.roughnessFactor                = (FLOAT)materialDesc.pbrMetallicRoughness.roughnessFactor;
			pbrRoughness.baseColorTexture.texCoordIndex = materialDesc.pbrMetallicRoughness.baseColorTexture.texCoord;
			const int textureIndex                      = materialDesc.pbrMetallicRoughness.baseColorTexture.index;

			auto& textureInfo = GetTexture(textureIndex);
			int sampler       = textureInfo.sampler; //indexes into samplers
			int source        = textureInfo.source; //indexes into images

			assert(sampler == -1); //sampler info is not supported yet

			const tinygltf::Image& imageInfo = GetImage(source);
			pbrRoughness.baseColorTexture.texture.imageBufferInfo.mimeType = imageInfo.name;
			pbrRoughness.baseColorTexture.texture.imageBufferInfo.name = imageInfo.mimeType;

			const tinygltf::BufferView& bufViewDesc = GetBufferView(imageInfo.bufferView);
			assert(bufViewDesc.byteStride == 0);

			pbrRoughness.baseColorTexture.texture.imageBufferInfo.bufferIndex = bufViewDesc.buffer;
			pbrRoughness.baseColorTexture.texture.imageBufferInfo.bufferOffsetInBytes = bufViewDesc.byteOffset;
			pbrRoughness.baseColorTexture.texture.imageBufferInfo.bufferSizeInBytes = bufViewDesc.byteLength;
			pbrRoughness.baseColorTexture.texture.imageBufferInfo.bufferStrideInBytes = 0; //not supported yet
			meshInfo.materialInfo.isMaterialDefined = TRUE;
		}
		else
		{
			meshInfo.materialInfo.isMaterialDefined = FALSE;
		}
	}

}





