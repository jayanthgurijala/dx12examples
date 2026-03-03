/*
* Copyright (C) 2026 by Jayanth Gurijala
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/
// Dx12HelloWorld.cpp : Defines the entry point for the application.
//

#include <d3d12.h>
#include "Dx12Raytracing.h"
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

Dx12Raytracing::Dx12Raytracing(UINT width, UINT height) :
	Dx12SampleBase(width, height)
{
}

HRESULT Dx12Raytracing::OnInit()
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


VOID Dx12Raytracing::BuildBlasAndTlas()
{
	StartBuildingAccelerationStructures();

	const D3D12_RESOURCE_STATES resultState  = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;

	//@note should this be in UAV? Sample apps use UAV but there is DebugLayer warning
	const D3D12_RESOURCE_STATES scratchState = D3D12_RESOURCE_STATE_COMMON;

	const UINT numNodesInScene = NumNodesInScene();
	m_blasResultBuffer.clear();
	m_blasResultBuffer.resize(numNodesInScene);
	m_blasScratchBuffer.clear();
	m_blasScratchBuffer.resize(numNodesInScene);

	for (UINT nodeIdx = 0; nodeIdx < numNodesInScene; nodeIdx++)
	{
		const UINT numPrimsInScene = NumPrimitivesInNodeMesh(nodeIdx);
		std::vector< D3D12_RAYTRACING_GEOMETRY_DESC> geomDescs(numPrimsInScene);
		for (UINT primIdx = 0; primIdx < numPrimsInScene; primIdx++)
		{
			BOOL isTransparent = IsPrimitiveTransparent(nodeIdx, primIdx);
			const D3D12_INDEX_BUFFER_VIEW indexBufferView   = GetModelIndexBufferView(nodeIdx, primIdx);
			const D3D12_VERTEX_BUFFER_VIEW vertexBufferView = GetModelPositionVertexBufferView(nodeIdx, primIdx);
			const DxDrawPrimitive         drawInfo = GetDrawInfo(nodeIdx, primIdx);

			D3D12_RAYTRACING_GEOMETRY_DESC& geomDesc = geomDescs[primIdx];
			geomDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
			geomDesc.Triangles.IndexBuffer = drawInfo.isIndexedDraw ? indexBufferView.BufferLocation : 0;
			geomDesc.Triangles.IndexCount = drawInfo.isIndexedDraw ? drawInfo.numIndices : 0;
			geomDesc.Triangles.IndexFormat = drawInfo.isIndexedDraw ? indexBufferView.Format : DXGI_FORMAT_UNKNOWN;

			geomDesc.Triangles.VertexBuffer = { vertexBufferView.BufferLocation, vertexBufferView.StrideInBytes };
			geomDesc.Triangles.VertexCount = drawInfo.numVertices;
			geomDesc.Triangles.VertexFormat = GetVertexPositionBufferFormat(nodeIdx, primIdx);

			//Object space -> new object space
			geomDesc.Triangles.Transform3x4 = 0;
			geomDesc.Flags = ((isTransparent == FALSE) ? D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE : D3D12_RAYTRACING_GEOMETRY_FLAG_NONE);
		}

		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS bottomLevelInputs = {};
		bottomLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
		bottomLevelInputs.NumDescs = geomDescs.size();
		bottomLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
		bottomLevelInputs.pGeometryDescs = geomDescs.data();
		bottomLevelInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

		// Get required sizes for an acceleration structure.
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO bottomLevelPrebuildInfo = {};
		m_dxrDevice->GetRaytracingAccelerationStructurePrebuildInfo(&bottomLevelInputs, &bottomLevelPrebuildInfo);


		dxhelper::AllocateBufferResource(m_dxrDevice.Get(), bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes, &m_blasResultBuffer[nodeIdx], "BlasResult", D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, resultState);
		dxhelper::AllocateBufferResource(m_dxrDevice.Get(), bottomLevelPrebuildInfo.ScratchDataSizeInBytes, &m_blasScratchBuffer[nodeIdx], "BlasScratch", D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, scratchState);
	
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC bottomLevelBuildDesc = {};
		bottomLevelBuildDesc.Inputs                            = bottomLevelInputs;
		bottomLevelBuildDesc.DestAccelerationStructureData    = m_blasResultBuffer[nodeIdx]->GetGPUVirtualAddress();
		bottomLevelBuildDesc.ScratchAccelerationStructureData = m_blasScratchBuffer[nodeIdx]->GetGPUVirtualAddress();
		m_dxrCommandList->BuildRaytracingAccelerationStructure(&bottomLevelBuildDesc, 0, nullptr);

		ExecuteBuildAccelerationStructures();
	
	}


	DxCamera* pCamera = GetCamera();
	std::vector< D3D12_RAYTRACING_INSTANCE_DESC> instanceDescs(numNodesInScene);
	for (UINT nodeIdx = 0; nodeIdx < numNodesInScene; nodeIdx++)
	{
		const XMFLOAT4X4 pData = pCamera->GetDxrModelTransposeMatrix(nodeIdx);
		D3D12_RAYTRACING_INSTANCE_DESC& instanceDesc = instanceDescs[nodeIdx];
		memcpy(instanceDesc.Transform, &pData, sizeof(instanceDesc.Transform));
		instanceDesc.InstanceMask = 1;
		instanceDesc.InstanceContributionToHitGroupIndex = nodeIdx;
		instanceDesc.AccelerationStructure = m_blasResultBuffer[nodeIdx]->GetGPUVirtualAddress();
	}

	m_instanceDescBuffer = CreateBufferWithData(instanceDescs.data(),
		                                        sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * numNodesInScene,
		                                        "RayTracing_InstanceDesc",
		                                        D3D12_RESOURCE_FLAG_NONE,
		                                        D3D12_RESOURCE_STATE_COMMON,
		                                        TRUE);

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS topLevelInputs = {};
	topLevelInputs.Type          = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	topLevelInputs.NumDescs      = instanceDescs.size();
	topLevelInputs.DescsLayout   = D3D12_ELEMENTS_LAYOUT_ARRAY;
	topLevelInputs.Flags         = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
	topLevelInputs.InstanceDescs = m_instanceDescBuffer->GetGPUVirtualAddress();

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO topLevelPrebuildInfo = {};
	m_dxrDevice->GetRaytracingAccelerationStructurePrebuildInfo(&topLevelInputs, &topLevelPrebuildInfo);
	dxhelper::AllocateBufferResource(m_dxrDevice.Get(), topLevelPrebuildInfo.ResultDataMaxSizeInBytes, &m_tlasResultBuffer, "TlasResult", D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, resultState);
	dxhelper::AllocateBufferResource(m_dxrDevice.Get(), topLevelPrebuildInfo.ScratchDataSizeInBytes, &m_tlasScratchBuffer, "TlasScratch", D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, scratchState);

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC topLevelBuildDesc = {};
	topLevelBuildDesc.Inputs = topLevelInputs;
	topLevelBuildDesc.DestAccelerationStructureData = m_tlasResultBuffer->GetGPUVirtualAddress();
	topLevelBuildDesc.ScratchAccelerationStructureData = m_tlasScratchBuffer->GetGPUVirtualAddress();
	m_dxrCommandList->BuildRaytracingAccelerationStructure(&topLevelBuildDesc, 0, nullptr);

	ExecuteBuildAccelerationStructures();
}


VOID Dx12Raytracing::CreateRtPSO()
{
	//@todo gltf description has sampler info for each texture
	CD3DX12_STATIC_SAMPLER_DESC staticSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR);

    const UINT numSrvsForModelMaterials  = NumSRVsInScene();
    const UINT numExtraSrvsForRaytracing = NumSrvsForRaytracing(); //uv buffer srv and index buffer srv
    const UINT numUavsInScene            = NumUAVsNeededForApp();
	const UINT numCbvsInScene            = 0;

    ///@note One range for model meterial SRVs(space = 0), one range for vertex buffer SRV and index buffer srv (space = 1), and one for UAV
	const UINT numDescTableRanges = 3;
    std::vector<CD3DX12_DESCRIPTOR_RANGE> descTableRanges(numDescTableRanges);
    descTableRanges[0] = dxhelper::GetSRVDescRange(numSrvsForModelMaterials, 0, 0); //model material SRVs
    descTableRanges[1] = dxhelper::GetSRVDescRange(numExtraSrvsForRaytracing, 0, 1); //vertex buffer SRV and index buffer srv
    descTableRanges[2] = dxhelper::GetUAVDescRange(numUavsInScene, 0, 0); //output UAV

	auto rootDescriptorTable = dxhelper::GetRootDescTable(descTableRanges);
    auto rootSrv = dxhelper::GetRootSrv(0, 2); //tlas srv at register space 2
    auto rootCbv = dxhelper::GetRootCbv(); //camera buffer at register space 0


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
			rootCbv,
			rootDescriptorTable,
			rootSrv
		},
		{staticSampler}
	);

	{
		const UINT numDescTableRanges = 1;
		std::vector<CD3DX12_DESCRIPTOR_RANGE> descTableRanges(numDescTableRanges);
		descTableRanges[0]       = dxhelper::GetSRVDescRange(NumSRVsPerPrimitive(), 0, 3);
		auto rootDescriptorTable = dxhelper::GetRootDescTable(descTableRanges);
		auto rootCbv             = dxhelper::GetRootCbv(0, 3);
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

	///create the extra two SRV descriptors for raytracing
	{
		auto uvVbBufferRes = GetModelUvVertexBufferResource();
		auto uvVbView = GetModelUvBufferView();

		const UINT uvVbElementSizeInBytes = 4;
		const UINT uvVbNumElements = uvVbView.SizeInBytes / 4;
		CreateAppBufferSrvDescriptorAtIndex(numSrvsForModelMaterials + 0, uvVbBufferRes, uvVbNumElements, uvVbElementSizeInBytes);

		auto indexBufferRes = GetModelIndexBufferResource();
		auto indexBufferView = GetModelIndexBufferView(0);

		const UINT ibElementSizeInBytes = 4;
		const UINT ibNumElements = indexBufferView.SizeInBytes / ibElementSizeInBytes;
		CreateAppBufferSrvDescriptorAtIndex(numSrvsForModelMaterials + 1, indexBufferRes, ibNumElements, ibElementSizeInBytes);
	}

    m_rootCbvIndex = 0;
    m_descTableIndex = 1;
	m_tlasSrvRootParamIndex = 2;

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

VOID Dx12Raytracing::BuildShaderTables()
{
	UINT numNodesInScene = NumNodesInScene();
	UINT totalPrimIdx = 0;
	for (UINT nodeIdx = 0; nodeIdx < numNodesInScene; nodeIdx++)
	{
		UINT numPrimsInNodeMesh = NumPrimitivesInNodeMesh(nodeIdx);
		for (UINT primIdx = 0; primIdx < numPrimsInNodeMesh; primIdx++)
		{
			totalPrimIdx++;
		}
	}

	const UINT totalPrimsInScene = totalPrimIdx;

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

	UINT numHitGroups = _countof(s_hitGroups);

	const UINT64 sbtTableTotalSize = rayGenAlignedSize + hitGroupTotalSizeInBytes + missTableAlignedSize;
	BYTE* const sbtDataStart       = static_cast<BYTE*>(malloc(sbtTableTotalSize));
	BYTE* sbtDataWritePtr          = sbtDataStart;

	{
		memcpy(sbtDataWritePtr, raygenShaderID, rayGenTableSize);
		sbtDataWritePtr += rayGenAlignedSize;
	}

	{
		BYTE* const pHitGroupStartWritePtr = sbtDataWritePtr;
		UINT numNodesInScene = NumNodesInScene();
		UINT totalPrimIdx = 0;
		const UINT numSrvsPerPrim = NumSRVsPerPrimitive();
		for (UINT nodeIdx = 0; nodeIdx < numNodesInScene; nodeIdx++)
		{
			UINT numPrimsInNodeMesh = NumPrimitivesInNodeMesh(nodeIdx);
			for (UINT primIdx = 0; primIdx < numPrimsInNodeMesh; primIdx++)
			{
				auto& curPrimitive    = GetPrimitiveInfo(nodeIdx, primIdx);
				BOOL needsAnyHit      = IsPrimitiveTransparent(nodeIdx, primIdx);
				auto gpuVAMaterialsCB = curPrimitive.materialTextures.meterialCb;
				auto gpuVAMatTex      = GetAppSrvGpuHandle(totalPrimIdx * numSrvsPerPrim);
				void* curHitGroupID   = (needsAnyHit == TRUE) ? hitgroupMaskID : hitgroupOpaqueID;

				memcpy(sbtDataWritePtr, curHitGroupID, hitGroupTableSize);
				sbtDataWritePtr += shaderRecordSize;
				memcpy(sbtDataWritePtr, &gpuVAMaterialsCB, sizeof(gpuVAMaterialsCB));
				sbtDataWritePtr += sizeof(gpuVAMaterialsCB);
				memcpy(sbtDataWritePtr, &gpuVAMatTex, sizeof(gpuVAMatTex));
				sbtDataWritePtr += sizeof(gpuVAMatTex);

				assert(pHitGroupStartWritePtr + totalPrimIdx * hitGroupAlignedSize + hitGroupTableSize == sbtDataWritePtr);
				totalPrimIdx++;
				sbtDataWritePtr = pHitGroupStartWritePtr + hitGroupAlignedSize * totalPrimIdx;
			}
		}
	}

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

VOID Dx12Raytracing::CreateUAVOutput()
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

HRESULT Dx12Raytracing::RenderFrame()
{
	static BOOL vbIbTransitioned = FALSE;

    //@todo optimization - transition resources once in init and keep them in the required state for raytracing instead of transitioning every frame.
	// This is just to keep the sample code simple and focused on raytracing.
	auto uvVbBufferRes = GetModelUvVertexBufferResource();
	auto indexBufferRes = GetModelIndexBufferResource();

	ImGui::Text("Ray Tracing");

	SetFrameInfo(m_uavOutputResource.Get(), UINT_MAX, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

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
	m_dxrCommandList->SetComputeRootConstantBufferView(0, GetCameraBuffer());
	m_dxrCommandList->SetComputeRootDescriptorTable(1, GetAppSrvGpuHandle(0));
	m_dxrCommandList->SetComputeRootShaderResourceView(m_tlasSrvRootParamIndex, m_tlasResultBuffer->GetGPUVirtualAddress());


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

	return S_OK;
}

DX_ENTRY_POINT(Dx12Raytracing);
