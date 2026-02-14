/*
* Copyright (C) 2026 by Jayanth Gurijala
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/
#pragma once

#include <string>
#include "dxtypes.h"
#include "tiny_gltf.h"
#include "DxPrintUtils.h"

struct DxGltfStatus
{
	std::string        m_err;
	std::string        m_warn;
};

class DxGltfLoader
{
public:
	DxGltfLoader(std::string filename);
	HRESULT LoadModel();
	UINT NumScenes();
	UINT NumNodesInScene(UINT index);

	VOID GetNodeTransformInfo(DxNodeTransformInfo& meshTransformInfo, UINT sceneIndex, UINT nodeIndex);
	VOID LoadMeshPrimitiveInfo(DxGltfMeshPrimInfo& meshInfo, UINT sceneIndex, UINT nodeIndex, UINT primitiveIndex);

	inline unsigned char* GetBufferData(UINT index)
	{
		return m_model.buffers[index].data.data();
	}

private:

	inline tinygltf::Node& GetNode(UINT sceneIndex, UINT nodeIndex)
	{

		return m_model.nodes[
			m_model.scenes[sceneIndex].nodes[nodeIndex]
		];
	}

	inline tinygltf::Mesh& GetMesh(UINT sceneIndex, UINT nodeIndex)
	{
		return m_model.meshes[GetNode(sceneIndex, nodeIndex).mesh];
	}

	inline tinygltf::Primitive& GetPrimitive(UINT sceneIndex, UINT nodeIndex, UINT primitiveIndex)
	{
		return GetMesh(sceneIndex, nodeIndex).primitives[primitiveIndex];
	}

	inline tinygltf::Accessor& GetAccessor(UINT index)
	{
		return m_model.accessors[index];
	}

	inline tinygltf::BufferView& GetBufferView(UINT index)
	{
		return m_model.bufferViews[index];
	}

	std::vector<std::string> m_supportedAttributes;
	std::string m_modelPath;

	tinygltf::Model    m_model;
	tinygltf::TinyGLTF m_loader;

	DxGltfStatus              m_loadStatus;
};


