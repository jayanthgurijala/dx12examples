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
	m_supportedAttributes({"POSITION", "NORMAL", "TEXCOORD_0"})
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
VOID DxGltfLoader::LoadMeshPrimitiveInfo(DxGltfPrimInfo& primInfo, UINT sceneIndex, UINT nodeIndex, UINT primitiveIndex)
{
	const tinygltf::Mesh& mesh = GetMesh(sceneIndex, nodeIndex);
	const tinygltf::Primitive& primitive = GetPrimitive(sceneIndex, nodeIndex, primitiveIndex);
	const UINT totalAttributesInPrimitive = min(m_supportedAttributes.size(), primitive.attributes.size());
	primInfo.vbInfo.resize(totalAttributesInPrimitive);
	primInfo.name = mesh.name;
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

					primInfo.extents.min[0] = (FLOAT)accessorDesc.minValues[0];
					primInfo.extents.min[1] = (FLOAT)accessorDesc.minValues[1];
					primInfo.extents.min[2] = (FLOAT)accessorDesc.minValues[2];

					primInfo.extents.max[0] = (FLOAT)accessorDesc.maxValues[0];
					primInfo.extents.max[1] = (FLOAT)accessorDesc.maxValues[1];
					primInfo.extents.max[2] = (FLOAT)accessorDesc.maxValues[2];

                    primInfo.extents.hasValidExtents = TRUE;

				}
				const int bufferViewIdx = accessorDesc.bufferView;
				const tinygltf::BufferView bufViewDesc = GetBufferView(bufferViewIdx);

				///@todo need to support interleaved data
				assert(bufViewDesc.byteStride == 0);

				const int    bufferIdx          = bufViewDesc.buffer;
				const size_t buflength          = bufViewDesc.byteLength;
				const size_t bufOffset          = bufViewDesc.byteOffset;
				const size_t accessorByteOffset = accessorDesc.byteOffset;

				primInfo.vbInfo[attrIndx].iaLayoutInfo.format = GltfGetDxgiFormat(componentDataType, componentVecType);
				primInfo.vbInfo[attrIndx].bufferIndex         = bufferIdx;
				primInfo.vbInfo[attrIndx].bufferOffsetInBytes = accessorByteOffset + bufOffset;
				primInfo.vbInfo[attrIndx].bufferSizeInBytes   = buflength;
				primInfo.vbInfo[attrIndx].bufferStrideInBytes = componentSizeInBytes * numComponents;

				auto& currentSemantic = primInfo.vbInfo[attrIndx].iaLayoutInfo;
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
		primInfo.drawInfo.numVertices = (UINT)(numTotalVertices);
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

			primInfo.ibInfo.indexFormat       = GltfGetDxgiFormat(accessorDesc.componentType, accessorDesc.type);
			primInfo.ibInfo.name =            "indices";
			primInfo.ibInfo.bufferIndex       = bufViewDesc.buffer;
			primInfo.ibInfo.bufferOffsetInBytes = byteOffsetIntoBuffer;
			primInfo.ibInfo.bufferSizeInBytes = bufferSizeInBytes;
			primInfo.drawInfo.numIndices      = (UINT)accessorDesc.count;
			primInfo.drawInfo.isIndexedDraw = TRUE;
		}
		else
		{
			primInfo.drawInfo.isIndexedDraw = FALSE;
		}
	}

	//@note load material info
	{
		const int materialIdx = primitive.material;
		if (materialIdx != -1)
		{
			const tinygltf::Material& materialDesc = GetMaterial(materialIdx);
			primInfo.materialInfo.name             = materialDesc.name;
			primInfo.materialInfo.doubleSided      = materialDesc.doubleSided;

			{
				auto& pbrRoughness              = primInfo.materialInfo.pbrMetallicRoughness;
				pbrRoughness.metallicFactor     = (FLOAT)materialDesc.pbrMetallicRoughness.metallicFactor;
				pbrRoughness.roughnessFactor    = (FLOAT)materialDesc.pbrMetallicRoughness.roughnessFactor;
				pbrRoughness.baseColorFactor[0] = (FLOAT)materialDesc.pbrMetallicRoughness.baseColorFactor[0];
				pbrRoughness.baseColorFactor[1] = (FLOAT)materialDesc.pbrMetallicRoughness.baseColorFactor[1];
				pbrRoughness.baseColorFactor[2] = (FLOAT)materialDesc.pbrMetallicRoughness.baseColorFactor[2];
				pbrRoughness.baseColorFactor[3] = (FLOAT)materialDesc.pbrMetallicRoughness.baseColorFactor[3];

				LoadGltfTextureInfo(pbrRoughness.baseColorTexture, materialDesc.pbrMetallicRoughness.baseColorTexture);
				LoadGltfTextureInfo(pbrRoughness.metallicRoughnessTexture, materialDesc.pbrMetallicRoughness.metallicRoughnessTexture);
			}

			{
				auto& dxNormalInfo		   = primInfo.materialInfo.normalInfo;
				const auto& gltfNormalInfo = materialDesc.normalTexture;
				dxNormalInfo.scale         = gltfNormalInfo.scale;
				LoadGltfTextureInfo(dxNormalInfo.normalTexture, { gltfNormalInfo.index, gltfNormalInfo.texCoord });
			}

			{
				auto& dxOcclusionInfo         = primInfo.materialInfo.occlusionInfo;
				const auto& gltfOcclusionInfo = materialDesc.occlusionTexture;
				dxOcclusionInfo.strength      = gltfOcclusionInfo.strength;
				LoadGltfTextureInfo(dxOcclusionInfo.occlusionTexture, { gltfOcclusionInfo.index, gltfOcclusionInfo.texCoord });
			}

			{
				auto& dxEmissiveTextureInfo  = primInfo.materialInfo.emissiveInfo;
				dxEmissiveTextureInfo.emissiveFactor[0] = (FLOAT)materialDesc.emissiveFactor[0];
				dxEmissiveTextureInfo.emissiveFactor[1] = (FLOAT)materialDesc.emissiveFactor[1];
				dxEmissiveTextureInfo.emissiveFactor[2] = (FLOAT)materialDesc.emissiveFactor[2];
				LoadGltfTextureInfo(dxEmissiveTextureInfo.emissiveTexture, materialDesc.emissiveTexture);
			}

			if (materialDesc.alphaMode == "MASK")
			{
				primInfo.materialInfo.alphaMode = DxMask;
			}
			else if (materialDesc.alphaMode == "BLEND")
			{
				primInfo.materialInfo.alphaMode = DxBlend;
			}
			else
			{
				primInfo.materialInfo.alphaMode = DxOpaque;
			}

			primInfo.materialInfo.alphaCutOff = materialDesc.alphaCutoff;
		}
			
	}

}

VOID DxGltfLoader::LoadGltfTextureInfo(DxGltfTextureInfo& dxTextureInfo, const tinygltf::TextureInfo& gltfTextureInfo)
{
	const int textureIndex      = gltfTextureInfo.index;
	dxTextureInfo.texCoordIndex = gltfTextureInfo.texCoord;

	if (textureIndex != -1)
	{
		const tinygltf::Texture& textureInfo = GetTexture(textureIndex);
		const int samplerIdx                 = textureInfo.sampler;
		const int imageIdx                   = textureInfo.source;

		// Load sampler info
		if (samplerIdx != -1)
		{
			const tinygltf::Sampler& samplerDesc = GetSampler(samplerIdx);
			dxTextureInfo.texture.samplerInfo.minFilter = samplerDesc.minFilter;
			dxTextureInfo.texture.samplerInfo.magFilter = samplerDesc.magFilter;
			dxTextureInfo.texture.samplerInfo.wrapS = samplerDesc.wrapS;
			dxTextureInfo.texture.samplerInfo.wrapT = samplerDesc.wrapT;
		}
		else
		{
			// Set default sampler values if not specified
			dxTextureInfo.texture.samplerInfo.minFilter = TINYGLTF_TEXTURE_FILTER_LINEAR;
			dxTextureInfo.texture.samplerInfo.magFilter = TINYGLTF_TEXTURE_FILTER_LINEAR;
			dxTextureInfo.texture.samplerInfo.wrapS     = TINYGLTF_TEXTURE_WRAP_REPEAT;
			dxTextureInfo.texture.samplerInfo.wrapT     = TINYGLTF_TEXTURE_WRAP_REPEAT;
		}

		// Load image buffer info
		if (imageIdx != -1)
		{
			const tinygltf::Image& imageInfo               = GetImage(imageIdx);
			dxTextureInfo.texture.imageBufferInfo.name     = imageInfo.name;
			dxTextureInfo.texture.imageBufferInfo.mimeType = imageInfo.mimeType;

			assert(imageInfo.bufferView != -1);

			const tinygltf::BufferView& bufViewDesc = GetBufferView(imageInfo.bufferView);
			assert(bufViewDesc.byteStride == 0);

			dxTextureInfo.texture.imageBufferInfo.bufferIndex         = bufViewDesc.buffer;
			dxTextureInfo.texture.imageBufferInfo.bufferOffsetInBytes = bufViewDesc.byteOffset;
			dxTextureInfo.texture.imageBufferInfo.bufferSizeInBytes   = bufViewDesc.byteLength;
			dxTextureInfo.texture.imageBufferInfo.bufferStrideInBytes = 0;
		}
		else
		{
			dxTextureInfo.texture.imageBufferInfo.bufferSizeInBytes = 0;
		}
	}
	else
	{
		dxTextureInfo.texture.imageBufferInfo.bufferSizeInBytes = 0;
	}



}





