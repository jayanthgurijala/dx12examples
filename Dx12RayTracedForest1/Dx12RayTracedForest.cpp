/*
* Copyright (C) 2026 by Jayanth Gurijala
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/
// Dx12HelloWorld.cpp : Defines the entry point for the application.
//

#include <d3d12.h>
#include "Dx12RayTracedForest.h"
#include "ExampleEntryPoint.h"
#include "framework.h"
#include <d3dx12.h>
#include "dxhelper.h"
#include "DxPrintUtils.h"
#include <imgui.h>

using namespace DirectX;

static LPCWSTR s_hitGroups[] =
{
	L"HitGroup_opaque",
	L"HitGroup_mask"
};


struct RayPayload
{
	float color[4];
};

Dx12RayTracedForest::Dx12RayTracedForest(UINT width, UINT height) :
	Dx12SampleBase(width, height)
{
}

HRESULT Dx12RayTracedForest::OnInit()
{
	Dx12SampleBase::OnInit();

	ID3D12Device* pDevice = GetDevice();
	assert(pDevice != nullptr);

	ID3D12GraphicsCommandList* pCmdList = GetCmdList();
	assert(pCmdList != nullptr);

	pDevice->QueryInterface(IID_PPV_ARGS(&m_dxrDevice));
	assert(m_dxrDevice != nullptr);

	pCmdList->QueryInterface(IID_PPV_ARGS(&m_dxrCommandList));
	assert(m_dxrCommandList != nullptr);

	CreateRtPSO();
	BuildBlasAndTlas();
	BuildShaderTables();
	CreateUAVOutput();

	return S_OK;
}


VOID Dx12RayTracedForest::BuildBlasAndTlas()
{
	StartBuildingAccelerationStructures();

	const D3D12_RESOURCE_STATES resultState  = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;

	//@note should this be in UAV? Sample apps use UAV but there is DebugLayer warning
	const D3D12_RESOURCE_STATES scratchState = D3D12_RESOURCE_STATE_COMMON;

	const UINT numSceneElements = NumSceneElementsLoaded();
	for (UINT sceneElemIdx = 0; sceneElemIdx < numSceneElements; sceneElemIdx++)
	{
		//------> for each node need a Tlas but Tlas needs to be built from scene description i.e. LoadScene() info
		const UINT numNodesInScene = NumNodesInScene(sceneElemIdx);
		for (UINT nodeIdx = 0; nodeIdx < numNodesInScene; nodeIdx++)
		{
			//------> for each prim need a GeomDesc, all Geom Desc go into one Blas
			const UINT numPrimsInNodeMesh = NumPrimitivesInNodeMesh(sceneElemIdx, nodeIdx);
			m_sceneBlas.sceneElementsBlas.emplace_back();
			DxASDesc& blasForNode = m_sceneBlas.sceneElementsBlas.back();
			std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> geomDescs(numPrimsInNodeMesh);
			for (UINT primIdx = 0; primIdx < numPrimsInNodeMesh; primIdx++)
			{
				auto& geomDesc = geomDescs[primIdx];
				const D3D12_INDEX_BUFFER_VIEW indexBufferView = GetModelIndexBufferView(sceneElemIdx, nodeIdx, primIdx);
				const D3D12_VERTEX_BUFFER_VIEW vertexBufferView = GetModelPositionVertexBufferView(sceneElemIdx, nodeIdx, primIdx);
				const DxDrawPrimitive         drawInfo = GetDrawInfo(sceneElemIdx, nodeIdx, primIdx);
				const BOOL isPrimTransparent = IsPrimitiveTransparent(sceneElemIdx, nodeIdx, primIdx);
				geomDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
				geomDesc.Triangles.IndexBuffer = drawInfo.isIndexedDraw ? indexBufferView.BufferLocation : 0;
				geomDesc.Triangles.IndexCount = drawInfo.isIndexedDraw ? drawInfo.numIndices : 0;
				geomDesc.Triangles.IndexFormat = drawInfo.isIndexedDraw ? indexBufferView.Format : DXGI_FORMAT_UNKNOWN;

				geomDesc.Triangles.VertexBuffer = { vertexBufferView.BufferLocation, vertexBufferView.StrideInBytes };
				geomDesc.Triangles.VertexCount = drawInfo.numVertices;
				geomDesc.Triangles.VertexFormat = GetVertexPositionBufferFormat(sceneElemIdx, nodeIdx, primIdx);

				//Object space -> new object space
				geomDesc.Triangles.Transform3x4 = 0;
				geomDesc.Flags = ((isPrimTransparent == FALSE) ? D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE : D3D12_RAYTRACING_GEOMETRY_FLAG_NONE);
			}
			D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS bottomLevelInputs = {};
			bottomLevelInputs.Type           = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
			bottomLevelInputs.NumDescs       = geomDescs.size();
			bottomLevelInputs.DescsLayout    = D3D12_ELEMENTS_LAYOUT_ARRAY;
			bottomLevelInputs.pGeometryDescs = geomDescs.data();
			bottomLevelInputs.Flags          = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
			// Get required sizes for an acceleration structure.
			D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO bottomLevelPrebuildInfo = {};
			m_dxrDevice->GetRaytracingAccelerationStructurePrebuildInfo(&bottomLevelInputs, &bottomLevelPrebuildInfo);
			dxhelper::AllocateBufferResource(m_dxrDevice.Get(), bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes, &blasForNode.resultBuffer, "BlasResult", D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, resultState);
			dxhelper::AllocateBufferResource(m_dxrDevice.Get(), bottomLevelPrebuildInfo.ScratchDataSizeInBytes, &blasForNode.scratchBuffer, "BlasScratch", D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, scratchState);

			D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC bottomLevelBuildDesc = {};
			bottomLevelBuildDesc.Inputs                           = bottomLevelInputs;
			bottomLevelBuildDesc.DestAccelerationStructureData    = blasForNode.resultBuffer->GetGPUVirtualAddress();
			bottomLevelBuildDesc.ScratchAccelerationStructureData = blasForNode.scratchBuffer->GetGPUVirtualAddress();
			m_dxrCommandList->BuildRaytracingAccelerationStructure(&bottomLevelBuildDesc, 0, nullptr);

			ExecuteBuildAccelerationStructures();
		}
	}


	DxCamera* pCamera = GetCamera();
	const UINT numNodeTransforms = pCamera->NumModelTransforms();
	std::vector< D3D12_RAYTRACING_INSTANCE_DESC> instanceDescs(numNodeTransforms);
	const UINT numElementsInSceneLoad = NumElementsInSceneLoad();
	UINT transformsLinearIdx = 0;
	UINT sceneElementNodeLineadIdx = 0;
	for (UINT idx = 0; idx < numElementsInSceneLoad; idx++)
	{
		const UINT numNodesInScene = NumNodesInScene(idx);
		auto& info = SceneElementInstance(idx);
		const UINT sceneElementIdx = info.sceneElementIdx;
		//one node can have 2 BLAS e.g. oaktree
		for (UINT nodeIdx = 0; nodeIdx < numNodesInScene; nodeIdx++)
		{
			auto& blasDesc = m_sceneBlas.sceneElementsBlas[sceneElementNodeLineadIdx];
			const UINT numInstances = info.numInstances;
			for (UINT instanceIdx = 0; instanceIdx < numInstances; instanceIdx++)
			{
				D3D12_RAYTRACING_INSTANCE_DESC& instanceDesc = instanceDescs[transformsLinearIdx];
				const XMFLOAT4X4 pData = pCamera->GetDxrModelTransposeMatrix(transformsLinearIdx);
				memcpy(instanceDesc.Transform, &pData, sizeof(instanceDesc.Transform));
				instanceDesc.InstanceMask = 1;
				instanceDesc.InstanceContributionToHitGroupIndex = sceneElementNodeLineadIdx;
				instanceDesc.AccelerationStructure = blasDesc.resultBuffer->GetGPUVirtualAddress();
				transformsLinearIdx++;
			}
			sceneElementNodeLineadIdx++;
		}
	}

	//@note Important we use a fence and block on CPU, so this will not get deallocated till TLAS build is complete.
	 ComPtr<ID3D12Resource> instanceDesc = CreateBufferWithData(instanceDescs.data(),
		                                                        instanceDescs.size() * sizeof(D3D12_RAYTRACING_INSTANCE_DESC),
		                                                        "RayTracing_InstanceDesc",
		                                                        D3D12_RESOURCE_FLAG_NONE,
		                                                        D3D12_RESOURCE_STATE_COMMON,
		                                                        TRUE);
	

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS topLevelInputs = {};
	topLevelInputs.Type          = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	topLevelInputs.NumDescs      = instanceDescs.size();
	topLevelInputs.DescsLayout   = D3D12_ELEMENTS_LAYOUT_ARRAY;
	topLevelInputs.Flags         = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
	topLevelInputs.InstanceDescs = instanceDesc->GetGPUVirtualAddress();

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO topLevelPrebuildInfo = {};
	m_dxrDevice->GetRaytracingAccelerationStructurePrebuildInfo(&topLevelInputs, &topLevelPrebuildInfo);
	dxhelper::AllocateBufferResource(m_dxrDevice.Get(), topLevelPrebuildInfo.ResultDataMaxSizeInBytes, &m_sceneTlas.sceneTlas.resultBuffer, "TlasResult", D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, resultState);
	dxhelper::AllocateBufferResource(m_dxrDevice.Get(), topLevelPrebuildInfo.ScratchDataSizeInBytes, &m_sceneTlas.sceneTlas.scratchBuffer, "TlasScratch", D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, scratchState);

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC topLevelBuildDesc = {};
	topLevelBuildDesc.Inputs = topLevelInputs;
	topLevelBuildDesc.DestAccelerationStructureData = m_sceneTlas.sceneTlas.resultBuffer->GetGPUVirtualAddress();
	topLevelBuildDesc.ScratchAccelerationStructureData = m_sceneTlas.sceneTlas.scratchBuffer->GetGPUVirtualAddress();
	m_dxrCommandList->BuildRaytracingAccelerationStructure(&topLevelBuildDesc, 0, nullptr);

	//@note IMPORTANT, depending on CPU wait in this call, else instance descs buffer will get deallocated
	ExecuteBuildAccelerationStructures();
}


VOID Dx12RayTracedForest::CreateRtPSO()
{
	//@todo gltf description has sampler info for each texture
	CD3DX12_STATIC_SAMPLER_DESC staticSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR);

    const UINT numUavsInScene            = NumUAVsNeededForApp();

    ///@note One range for model meterial SRVs(space = 0), one range for vertex buffer SRV and index buffer srv (space = 1), and one for UAV
	const UINT numDescTableRanges = 1;
    std::vector<CD3DX12_DESCRIPTOR_RANGE> descTableRanges(numDescTableRanges);
    descTableRanges[0] = dxhelper::GetUAVDescRange(numUavsInScene, 0, 0); //output UAV

	auto rootDescriptorTable = dxhelper::GetRootDescTable(descTableRanges);
    auto rootSrv             = dxhelper::GetRootSrv(0, 2); //tlas srv at register space 2
    auto viewProj            = dxhelper::GetRootCbv(0); //camera buffer at register space 0
    auto perInstance         = dxhelper::GetRootCbv(1); //camera buffer at register space 0


	///@note In Raytracing, root signature needs to be divided into local and global root signature.
	// Camera Data, TLAS, UAV Output and sampler (for now) - part of global root sig
	// Prim Textures and Material Data - move to local root signature
	// Local root sig? - Root CBV with material info and DescTable with "5" textures 
	dxhelper::DxCreateRootSignature
	(
		m_dxrDevice.Get(),

		//camera buffer, texture stv, output uav, AS, vertex buffer, index buffer
		&m_rootSignature,
		{
			viewProj,
			perInstance,
			rootDescriptorTable,
			rootSrv
		},
		{staticSampler}
	);

	{
		const UINT numDescTableRanges = 1;
		std::vector<CD3DX12_DESCRIPTOR_RANGE> descTableRanges(numDescTableRanges);
		const UINT numSRVsPerPrim = NumSRVsPerPrimitive();
		const UINT registerSpace  = 3;
		descTableRanges[0]        = dxhelper::GetSRVDescRange(numSRVsPerPrim, 0, registerSpace);
		auto rootDescriptorTable  = dxhelper::GetRootDescTable(descTableRanges);
		auto rootCbv              = dxhelper::GetRootCbv(0, registerSpace);
		dxhelper::DxCreateRootSignature
		(
			m_dxrDevice.Get(),
			&m_localRootSignature,
			{
				rootCbv,
				rootDescriptorTable
			},
			{},
			D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE
		);
	}

	assert(m_localRootSignature != nullptr);

	const UINT numTotalSrvsPerPrim = NumSRVsPerPrimitive();
	const UINT appSrvOffset = AppSrvOffsetForPrim();
	///create the extra two SRV descriptors for raytracing
	ForEachSceneElementLoadedSceneNodePrim([this, appSrvOffset](UINT sceneIdx, UINT nodeIdx, UINT primIdx)
		{
			auto& curPrimitive = GetPrimitiveInfo(sceneIdx, nodeIdx, primIdx);
			auto uvVbBufferRes = GetModelUvVertexBufferResource(sceneIdx, nodeIdx, primIdx, 0);
			auto uvVbView = GetModelUvBufferView(sceneIdx, nodeIdx, primIdx, 0);

			const UINT uvVbElementSizeInBytes = 8;
			const UINT uvVbNumElements = uvVbView.SizeInBytes / 8;
			CreateAppBufferSrvDescriptorAtIndex(curPrimitive.primLinearIdxInSceneElements, appSrvOffset + 0, uvVbBufferRes, uvVbNumElements, uvVbElementSizeInBytes);

			auto posVbBufferRes = GetModelPositionVertexBufferResource(sceneIdx, nodeIdx, primIdx);
			auto posVbView = GetModelPositionVertexBufferView(sceneIdx, nodeIdx, primIdx);
			const UINT posVbElementSizeInBytes = 12;
			const UINT numPosElements = posVbView.SizeInBytes / 12;
			CreateAppBufferSrvDescriptorAtIndex(curPrimitive.primLinearIdxInSceneElements, appSrvOffset + 1, posVbBufferRes, numPosElements, posVbElementSizeInBytes);

			auto indexBufferRes = GetModelIndexBufferResource(sceneIdx, nodeIdx, primIdx);
			auto indexBufferView = GetModelIndexBufferView(sceneIdx, nodeIdx, primIdx);

			const UINT ibElementSizeInBytes = 4;
			const UINT ibNumElements = indexBufferView.SizeInBytes / ibElementSizeInBytes;
			CreateAppBufferSrvDescriptorAtIndex(curPrimitive.primLinearIdxInSceneElements, appSrvOffset + 2, indexBufferRes, ibNumElements, ibElementSizeInBytes);
		});


    m_rootCbvIndex          = 0;
    m_descTableIndex        = 1;

	//@todo Simplify and use CD3DX12
	ComPtr<ID3DBlob> compiledShaders = GetCompiledShaderBlob("RaytraceSimpleCHS.cso");

	D3D12_EXPORT_DESC exports[] = {
		{ L"MyRaygenShader", nullptr, D3D12_EXPORT_FLAG_NONE },
		{ L"MyMissShader", nullptr, D3D12_EXPORT_FLAG_NONE },
		{ L"CHSBaseColorTexturing", nullptr, D3D12_EXPORT_FLAG_NONE },
		{ L"CHSNormalMapping", nullptr, D3D12_EXPORT_FLAG_NONE },
		{ L"AHSAlphaCutOff", nullptr, D3D12_EXPORT_FLAG_NONE}
	};

	D3D12_DXIL_LIBRARY_DESC dxilLibDesc = {};
	dxilLibDesc.DXILLibrary.BytecodeLength = compiledShaders->GetBufferSize();
	dxilLibDesc.DXILLibrary.pShaderBytecode = compiledShaders->GetBufferPointer();
	dxilLibDesc.NumExports = _countof(exports);
	dxilLibDesc.pExports = exports;

	D3D12_STATE_SUBOBJECT dxilLibSubobject = {};
	dxilLibSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
	dxilLibSubobject.pDesc = &dxilLibDesc;

	D3D12_HIT_GROUP_DESC hitGroupOpaqueDesc = {};
	hitGroupOpaqueDesc.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
	hitGroupOpaqueDesc.HitGroupExport = s_hitGroups[0];
	hitGroupOpaqueDesc.ClosestHitShaderImport = exports[2].Name;
	D3D12_STATE_SUBOBJECT hitGroup_1SubObject = {};
	hitGroup_1SubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
	hitGroup_1SubObject.pDesc = &hitGroupOpaqueDesc;

	D3D12_HIT_GROUP_DESC hitGroupMaskDesc = {};
	hitGroupMaskDesc.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
	hitGroupMaskDesc.HitGroupExport = s_hitGroups[1];
	hitGroupMaskDesc.ClosestHitShaderImport = exports[2].Name;
	hitGroupMaskDesc.AnyHitShaderImport = exports[4].Name;
	D3D12_STATE_SUBOBJECT hitgroupMaskSubObj = {};
	hitgroupMaskSubObj.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
	hitgroupMaskSubObj.pDesc = &hitGroupMaskDesc;


	D3D12_RAYTRACING_SHADER_CONFIG shaderConfig = {};
	shaderConfig.MaxAttributeSizeInBytes = D3D12_RAYTRACING_MAX_ATTRIBUTE_SIZE_IN_BYTES;
	shaderConfig.MaxPayloadSizeInBytes = sizeof(RayPayload);

	D3D12_STATE_SUBOBJECT shaderConfigSubObject;
	shaderConfigSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
	shaderConfigSubObject.pDesc = &shaderConfig;

	D3D12_GLOBAL_ROOT_SIGNATURE globalRootSigDesc = {};
	globalRootSigDesc.pGlobalRootSignature = m_rootSignature.Get();
	D3D12_STATE_SUBOBJECT globalRootSigSubObject = {};
	globalRootSigSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
	globalRootSigSubObject.pDesc = &globalRootSigDesc;


	D3D12_RAYTRACING_PIPELINE_CONFIG pipelineConfig = {};
	pipelineConfig.MaxTraceRecursionDepth = 1;

	D3D12_STATE_SUBOBJECT pipelineConfigSubobject = {};
	pipelineConfigSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
	pipelineConfigSubobject.pDesc = &pipelineConfig;

	D3D12_LOCAL_ROOT_SIGNATURE localRootSigDesc = {};
	localRootSigDesc.pLocalRootSignature = m_localRootSignature.Get();
	D3D12_STATE_SUBOBJECT localRootSigSubObject = {};
	localRootSigSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
	localRootSigSubObject.pDesc = &localRootSigDesc;

	D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION localRootSigToHitGroup;
	localRootSigToHitGroup.NumExports = _countof(s_hitGroups);
	localRootSigToHitGroup.pExports = s_hitGroups;
	localRootSigToHitGroup.pSubobjectToAssociate = &localRootSigSubObject;

	D3D12_STATE_SUBOBJECT localSigAssociationSubObj;
	localSigAssociationSubObj.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
	localSigAssociationSubObj.pDesc = &localRootSigToHitGroup;

	D3D12_STATE_SUBOBJECT subobjects[] = {
				dxilLibSubobject,
				hitGroup_1SubObject,
				hitgroupMaskSubObj,
				localRootSigSubObject,
				localSigAssociationSubObj,
				shaderConfigSubObject,
				pipelineConfigSubobject,
				globalRootSigSubObject
	};

	//$%@#$@#$!@#!@#$@#$@#$ nonsense
	localRootSigToHitGroup.pSubobjectToAssociate = &subobjects[3];

	D3D12_STATE_OBJECT_DESC stateObjectDesc = {};
	stateObjectDesc.Type                    = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
	stateObjectDesc.NumSubobjects           = _countof(subobjects);
	stateObjectDesc.pSubobjects             = subobjects;

	m_dxrDevice->CreateStateObject(&stateObjectDesc,
								   IID_PPV_ARGS(&m_rtpso));

	assert(m_rtpso != nullptr);
}

VOID Dx12RayTracedForest::BuildShaderTables()
{


	/// -Scene Load Elements (1)Deer (2)OakTree (3)Terrain
	/// -Scene can be composed or X Deers, Y OakTrees and Z Terrains
	/// How many SBTs should hit groups have?
	/// To answer this question, need to understand how indexing works.
	/// Ray contributions in Trace Ray offset and allot blocks of hitgroups
	/// Instance Contribution = sceneloadElementIdx 
	///  so Deer = 0, OakTree = 1 and 2, Terrain = 3 (order of scene load elements but then constant)
	/// so even if we have X,Y,Z Deers, OakTrees and Terrains, we will ever need only 4 SBT entries, i.e. as many prims.
	
	std::vector<DxPrimitiveInfo*> primitiveInfo;
	UINT numSceneLoadElements = NumElementsInSceneLoad();
	for (UINT sceneIdx = 0; sceneIdx < numSceneLoadElements; sceneIdx++)
	{
		UINT numNodesInSceneElement = NumNodesInScene(sceneIdx);
		for (UINT nodeIdx = 0; nodeIdx < numNodesInSceneElement; nodeIdx++)
		{
			UINT numPrimsInNodeMesh = NumPrimitivesInNodeMesh(sceneIdx, nodeIdx);
			for (UINT primIdx = 0; primIdx < numPrimsInNodeMesh; primIdx++)
			{
				primitiveInfo.push_back(&GetPrimitiveInfo(sceneIdx, nodeIdx, primIdx));
			}
		}
	}

	const UINT totalPrimsInScene = primitiveInfo.size();

	///@note Every primitive has a material and needs an SBT.
	///      There are two hitgroups for now - opaque and transparent

	ComPtr<ID3D12StateObjectProperties> props;
	m_rtpso->QueryInterface(IID_PPV_ARGS(&props));

	void* raygenShaderID   = props->GetShaderIdentifier(L"MyRaygenShader");
	void* missShaderID     = props->GetShaderIdentifier(L"MyMissShader");
	void* hitgroupOpaqueID = props->GetShaderIdentifier(s_hitGroups[0]);
	void* hitgroupMaskID   = props->GetShaderIdentifier(s_hitGroups[1]);

	assert(raygenShaderID != nullptr && missShaderID != nullptr && hitgroupOpaqueID != nullptr && hitgroupMaskID != nullptr);

	const UINT shaderRecordSize = D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT; // 32 bytes
	const UINT sectionAlignment = D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;  //64 bytes alignment

	UINT64 rayGenTableSize   = shaderRecordSize;
	UINT64 rayGenAlignedSize = dxhelper::DxAlign(rayGenTableSize, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);

	UINT64 hitGroupTableSize   = shaderRecordSize + sizeof(D3D12_GPU_VIRTUAL_ADDRESS) + sizeof (D3D12_GPU_DESCRIPTOR_HANDLE);
	UINT64 hitGroupAlignedSize = dxhelper::DxAlign(hitGroupTableSize, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
	UINT64 hitGroupTotalSizeInBytes = hitGroupAlignedSize * totalPrimsInScene;

	UINT64 missTableSize        = shaderRecordSize;
	UINT64 missTableAlignedSize = dxhelper::DxAlign(missTableSize, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);

	const UINT64 sbtTableTotalSize = rayGenAlignedSize + hitGroupTotalSizeInBytes + missTableAlignedSize;
	BYTE* const sbtDataStart       = static_cast<BYTE*>(malloc(sbtTableTotalSize));
	BYTE* sbtDataWritePtr          = sbtDataStart;

	{
		memcpy(sbtDataWritePtr, raygenShaderID, rayGenTableSize);
		sbtDataWritePtr += rayGenAlignedSize;
	}

	assert((sbtDataWritePtr - sbtDataStart) == rayGenAlignedSize);
	BYTE* const pHitGroupStartWritePtr = sbtDataWritePtr;
	{
		

		for (UINT primIdx = 0; primIdx < totalPrimsInScene; primIdx++)
		{
			auto* curPrimitive     = primitiveInfo[primIdx];
			BOOL  needsAnyHit      = IsPrimitiveTransparent(*curPrimitive);
			auto  gpuVAMaterialsCB = curPrimitive->materialTextures.meterialCb;
			auto  gpuVAMatTex      = GetPerPrimSrvGpuHandle(curPrimitive->primLinearIdxInSceneElements);
			void* curHitGroupID    = (needsAnyHit == TRUE) ? hitgroupMaskID : hitgroupOpaqueID;
			sbtDataWritePtr = pHitGroupStartWritePtr + hitGroupAlignedSize * primIdx;
			memcpy(sbtDataWritePtr, curHitGroupID, hitGroupTableSize);
			sbtDataWritePtr += shaderRecordSize;
			assert(pHitGroupStartWritePtr + primIdx * hitGroupAlignedSize + shaderRecordSize == sbtDataWritePtr);
			memcpy(sbtDataWritePtr, &gpuVAMaterialsCB, sizeof(gpuVAMaterialsCB));
			sbtDataWritePtr += sizeof(gpuVAMaterialsCB);
			assert(pHitGroupStartWritePtr + primIdx * hitGroupAlignedSize + shaderRecordSize + sizeof(gpuVAMaterialsCB) == sbtDataWritePtr);
			memcpy(sbtDataWritePtr, &gpuVAMatTex, sizeof(gpuVAMatTex));
			sbtDataWritePtr += sizeof(gpuVAMatTex);

			assert(sbtDataWritePtr == pHitGroupStartWritePtr + primIdx * hitGroupAlignedSize + hitGroupTableSize);
		}
		
	}

	sbtDataWritePtr = pHitGroupStartWritePtr + hitGroupAlignedSize * totalPrimsInScene;

	assert(sbtDataWritePtr == sbtDataStart + rayGenAlignedSize + hitGroupAlignedSize * totalPrimsInScene);
	memcpy(sbtDataWritePtr, missShaderID, missTableSize);

	m_shaderBindingTable = CreateBufferWithData(sbtDataStart, sbtTableTotalSize, "ShaderBindingTable");

	m_rayGenBaseAddress.StartAddress = m_shaderBindingTable->GetGPUVirtualAddress();
	m_rayGenBaseAddress.SizeInBytes = rayGenTableSize;

	m_hitTableBaseAddress.StartAddress = m_rayGenBaseAddress.StartAddress + rayGenAlignedSize;
	m_hitTableBaseAddress.SizeInBytes  = hitGroupTotalSizeInBytes;
	m_hitTableBaseAddress.StrideInBytes = hitGroupAlignedSize;

	m_missTableBaseAddress.StartAddress = m_hitTableBaseAddress.StartAddress + hitGroupTotalSizeInBytes;
	m_missTableBaseAddress.SizeInBytes = missTableSize;
	m_missTableBaseAddress.StrideInBytes = missTableSize;

	free(sbtDataStart);
}

VOID Dx12RayTracedForest::CreateUAVOutput()
{
	dxhelper::AllocateTexture2DResource(m_dxrDevice.Get(),
		                                &m_uavOutputResource,
		                                GetWidth(),
		                                GetHeight(),
		                                GetBackBufferFormat(),
		                                L"RayTracingUAVOutput",
		                                D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		                                nullptr,
		                                D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	//@todo this is a bit confusing beacuse UAVs are created after all SRVs.
	// in this case after srvs for materials and ray tracing srvs.
	CreateAppUavDescriptorAtIndex(0, m_uavOutputResource.Get());
}

VOID Dx12RayTracedForest::RenderFrame()
{
	static BOOL vbIbTransitioned = FALSE;

    //@todo optimization - transition resources once in init and keep them in the required state for raytracing instead of transitioning every frame.
	// This is just to keep the sample code simple and focused on raytracing.
	auto uvVbBufferRes = GetModelUvVertexBufferResource(0, 0, 0, 0);
	auto indexBufferRes = GetModelIndexBufferResource(0, 0, 0);

	ImGui::Text("Ray Tracing");

	AddFrameInfo(0, DxDescriptorTypeUavSrv);

	//if (vbIbTransitioned == FALSE)
	{
		CD3DX12_RESOURCE_BARRIER resourceBarrier[3];
		resourceBarrier[0] = CD3DX12_RESOURCE_BARRIER::Transition(indexBufferRes,
			                                                        D3D12_RESOURCE_STATE_INDEX_BUFFER,
			                                                        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		resourceBarrier[1] = CD3DX12_RESOURCE_BARRIER::Transition(uvVbBufferRes,
																	D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
																	D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		resourceBarrier[2] = CD3DX12_RESOURCE_BARRIER::UAV(m_uavOutputResource.Get());

		vbIbTransitioned = TRUE;
		m_dxrCommandList->ResourceBarrier(3, resourceBarrier);
	}


	D3D12_DISPATCH_RAYS_DESC dispatchRaysDesc = {};
	dispatchRaysDesc.Width = GetWidth();
	dispatchRaysDesc.Height = GetHeight();
	dispatchRaysDesc.Depth = 1;
	dispatchRaysDesc.RayGenerationShaderRecord = m_rayGenBaseAddress;
	dispatchRaysDesc.HitGroupTable = m_hitTableBaseAddress;
	dispatchRaysDesc.MissShaderTable = m_missTableBaseAddress;

	ID3D12DescriptorHeap* descriptorHeaps[] = { GetSrvDescriptorHeap() };
	m_dxrCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
	m_dxrCommandList->SetPipelineState1(m_rtpso.Get());
	m_dxrCommandList->SetComputeRootSignature(m_rootSignature.Get());

	//Root args required - UAV in the descriptor heap and Accelaration structure
	m_dxrCommandList->SetComputeRootConstantBufferView(0, GetViewProjLightsGpuVa(0)); //GetUavGpuHandle
	m_dxrCommandList->SetComputeRootConstantBufferView(1, GetPerInstanceDataGpuVa(0));
	m_dxrCommandList->SetComputeRootDescriptorTable(2, GetUavGpuHandle(0));
	m_dxrCommandList->SetComputeRootShaderResourceView(3, m_sceneTlas.sceneTlas.resultBuffer->GetGPUVirtualAddress());


	m_dxrCommandList->DispatchRays(&dispatchRaysDesc);

	{
		CD3DX12_RESOURCE_BARRIER resourceBarrier[3];
		resourceBarrier[0] = CD3DX12_RESOURCE_BARRIER::Transition(indexBufferRes,
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_INDEX_BUFFER
			);
		resourceBarrier[1] = CD3DX12_RESOURCE_BARRIER::Transition(uvVbBufferRes,
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
			);
		resourceBarrier[2] = CD3DX12_RESOURCE_BARRIER::UAV(m_uavOutputResource.Get());

		vbIbTransitioned = TRUE;
		m_dxrCommandList->ResourceBarrier(3, resourceBarrier);
	}
}

/*
*
* Order of SceneElements  Terrain, Deer, OakTree (foliage transparency last)
*
*/

VOID Dx12RayTracedForest::LoadSceneDescription(std::vector<DxSceneElementInstance>& sceneDescription)
{


	const UINT numSceneElementsLoaded = NumSceneElementsLoaded();
	const UINT numSceneElements = NumSceneElementsLoaded();
	sceneDescription.resize(numSceneElements);

	UINT terrainIdx = 0;
	UINT deerIdx = 0;
	UINT oakTreeIdx = 0;

	for (UINT idx = 0; idx < numSceneElements; idx++)
	{
		auto& currentSceneElement = sceneDescription[idx];
		if (idx == 0)
		{
			InitTerrain(currentSceneElement, terrainIdx);
			terrainIdx++;
		}
		else if (idx == 1)
		{
			InitAnimalsDeer(currentSceneElement, deerIdx);
			deerIdx++;
		}
		else
		{
			InitOakTrees(currentSceneElement, oakTreeIdx);
			oakTreeIdx++;
		}
	}
}


VOID Dx12RayTracedForest::InitTerrain(DxSceneElementInstance& sceneElement, UINT localIdx)
{
	sceneElement.sceneElementIdx = 0;

	sceneElement.numInstances = 1;
	sceneElement.addToExtents = FALSE;

	sceneElement.trsMatrix.resize(sceneElement.numInstances);
	auto& trsMatrix = sceneElement.trsMatrix[0];

	trsMatrix.translation[0] = 0.0f;
	trsMatrix.translation[1] = -.04f;
	trsMatrix.translation[2] = 0.0f;

	if (localIdx == 0)
	{
		trsMatrix.rotationInDegrees[0] = 0.0f;
	}
	else
	{
		trsMatrix.rotationInDegrees[0] = 90.0f;
	}
	trsMatrix.rotationInDegrees[1] = 0.0f;
	trsMatrix.rotationInDegrees[2] = 0.0f;

	trsMatrix.scale[0] = 1.0f;
	trsMatrix.scale[1] = 1.0f;
	trsMatrix.scale[2] = 1.0f;
}

VOID Dx12RayTracedForest::InitAnimalsDeer(DxSceneElementInstance& sceneElement, UINT localIdx)
{
	sceneElement.sceneElementIdx = 1;
	sceneElement.addToExtents = TRUE;
	sceneElement.numInstances = 1;
	sceneElement.trsMatrix.resize(sceneElement.numInstances);

	auto& trsMatrix = sceneElement.trsMatrix[0];

	trsMatrix.translation[0] = 0.0f;
	trsMatrix.translation[1] = 0.0f;
	trsMatrix.translation[2] = 0.0f;

	trsMatrix.rotationInDegrees[0] = 0.0f;
	trsMatrix.rotationInDegrees[1] = 0.0f;
	trsMatrix.rotationInDegrees[2] = 0.0f;

	trsMatrix.scale[0] = 1.0f;
	trsMatrix.scale[1] = 1.0f;
	trsMatrix.scale[2] = 1.0f;
}

VOID Dx12RayTracedForest::InitOakTrees(DxSceneElementInstance& sceneElement, UINT localIdx)
{
	sceneElement.sceneElementIdx = 2;
	sceneElement.numInstances = 1;
	sceneElement.addToExtents = TRUE;
	sceneElement.trsMatrix.resize(sceneElement.numInstances);

	auto& trsMatrix = sceneElement.trsMatrix[0];
	trsMatrix.translation[0] = 0.8f + localIdx * 0.8f; //looking from rear of Deer moving right
	trsMatrix.translation[1] = 0.0f; //moving forward towards Deer nose
	trsMatrix.translation[2] = 0.8; //moving down

	trsMatrix.rotationInDegrees[0] = 0.0f;
	trsMatrix.rotationInDegrees[1] = 0.0f;
	trsMatrix.rotationInDegrees[2] = 0.0f;

	trsMatrix.scale[0] = 3.0f;
	trsMatrix.scale[1] = 3.0f;
	trsMatrix.scale[2] = 3.0f;
}

DX_ENTRY_POINT(Dx12RayTracedForest);
