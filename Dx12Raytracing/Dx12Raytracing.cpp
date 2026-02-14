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

static std::wstring s_hitgroup_1 = L"Hitgroup_1";


//@note size must match Ray payload
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

	const D3D12_INDEX_BUFFER_VIEW indexBufferView   = GetModelIndexBufferView(0);
	const D3D12_VERTEX_BUFFER_VIEW vertexBufferView = GetModelPositionVertexBufferView();
	const DxDrawPrimitive         drawInfo          = GetDrawInfo(0);

	D3D12_RAYTRACING_GEOMETRY_DESC geomDesc = {};
	geomDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
	geomDesc.Triangles.IndexBuffer = drawInfo.isIndexedDraw ? indexBufferView.BufferLocation : 0;
	geomDesc.Triangles.IndexCount = drawInfo.isIndexedDraw ? drawInfo.numIndices : 0;
	geomDesc.Triangles.IndexFormat = drawInfo.isIndexedDraw ? indexBufferView.Format : DXGI_FORMAT_UNKNOWN;

	geomDesc.Triangles.VertexBuffer = { vertexBufferView.BufferLocation, vertexBufferView.StrideInBytes };
	geomDesc.Triangles.VertexCount  = drawInfo.numVertices;
	geomDesc.Triangles.VertexFormat = GetVertexBufferFormat(0);

	//Object space -> new object space
	geomDesc.Triangles.Transform3x4 = 0;
	geomDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS bottomLevelInputs = {};
	bottomLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	bottomLevelInputs.NumDescs = 1;
	bottomLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	bottomLevelInputs.pGeometryDescs = &geomDesc;
	bottomLevelInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

	// Get required sizes for an acceleration structure.
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO bottomLevelPrebuildInfo = {};
	m_dxrDevice->GetRaytracingAccelerationStructurePrebuildInfo(&bottomLevelInputs, &bottomLevelPrebuildInfo);
	
	dxhelper::AllocateBufferResource(m_dxrDevice.Get(), bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes, &m_blasResultBuffer, L"BlasResult", D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, resultState);
	dxhelper::AllocateBufferResource(m_dxrDevice.Get(), bottomLevelPrebuildInfo.ScratchDataSizeInBytes, &m_blasScratchBuffer, L"BlasScratch", D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, scratchState);

	PrintUtils::PrintBufferAddressRange(m_blasResultBuffer.Get());
	PrintUtils::PrintBufferAddressRange(m_blasScratchBuffer.Get());

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC bottomLevelBuildDesc = {};
	bottomLevelBuildDesc.Inputs = bottomLevelInputs;
	bottomLevelBuildDesc.DestAccelerationStructureData = m_blasResultBuffer->GetGPUVirtualAddress();
	bottomLevelBuildDesc.ScratchAccelerationStructureData = m_blasScratchBuffer->GetGPUVirtualAddress();
	m_dxrCommandList->BuildRaytracingAccelerationStructure(&bottomLevelBuildDesc, 0, nullptr);

	ExecuteBuildAccelerationStructures();

	DxCamera* pCamera = GetCamera();
	const XMFLOAT4X4 pData = pCamera->GetDxrModelTransposeMatrix(0);
	D3D12_RAYTRACING_INSTANCE_DESC instanceDesc = {};
	memcpy(instanceDesc.Transform, &pData, sizeof(instanceDesc.Transform));
	//instanceDesc.Transform[0][0] = instanceDesc.Transform[1][1] = instanceDesc.Transform[2][2] = 1;
	instanceDesc.InstanceMask = 1;
	instanceDesc.AccelerationStructure = m_blasResultBuffer->GetGPUVirtualAddress();

	m_instanceDescBuffer = CreateBufferWithData(&instanceDesc, sizeof(instanceDesc), L"RayTracing_InstanceDesc", D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COMMON, TRUE);

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS topLevelInputs = {};
	topLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	topLevelInputs.NumDescs = 1;
	topLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	topLevelInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
	topLevelInputs.InstanceDescs = m_instanceDescBuffer->GetGPUVirtualAddress();

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO topLevelPrebuildInfo = {};
	m_dxrDevice->GetRaytracingAccelerationStructurePrebuildInfo(&topLevelInputs, &topLevelPrebuildInfo);
	dxhelper::AllocateBufferResource(m_dxrDevice.Get(), topLevelPrebuildInfo.ResultDataMaxSizeInBytes, &m_tlasResultBuffer, L"TlasResult", D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, resultState);
	dxhelper::AllocateBufferResource(m_dxrDevice.Get(), topLevelPrebuildInfo.ScratchDataSizeInBytes, &m_tlasScratchBuffer, L"TlasScratch", D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, scratchState);

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC topLevelBuildDesc = {};
	topLevelBuildDesc.Inputs = topLevelInputs;
	topLevelBuildDesc.DestAccelerationStructureData = m_tlasResultBuffer->GetGPUVirtualAddress();
	topLevelBuildDesc.ScratchAccelerationStructureData = m_tlasScratchBuffer->GetGPUVirtualAddress();
	m_dxrCommandList->BuildRaytracingAccelerationStructure(&topLevelBuildDesc, 0, nullptr);

	ExecuteBuildAccelerationStructures();
}

VOID Dx12Raytracing::CreateRtPSO()
{
	CD3DX12_STATIC_SAMPLER_DESC staticSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR);

	dxhelper::DxCreateRootSignature
	(
		m_dxrDevice.Get(),

		//camera buffer, texture stv, output uav, AS, vertex buffer, index buffer
		&m_rootSignature,
		{
			0_rcbv,
			"srv_3_0,uav_1_0,cbv_0_0"_dt,
			3_rsrv
		},
		{staticSampler}
	);
	//@todo Simplify and use CD3DX12
	ComPtr<ID3DBlob> compiledShaders = GetCompiledShaderBlob("RaytraceSimpleCHS.cso");

	D3D12_EXPORT_DESC exports[] = {
		{ L"MyRaygenShader", nullptr, D3D12_EXPORT_FLAG_NONE },
		{ L"MyMissShader", nullptr, D3D12_EXPORT_FLAG_NONE },
		{ L"MyClosestHitShader", nullptr, D3D12_EXPORT_FLAG_NONE }
	};

	D3D12_DXIL_LIBRARY_DESC dxilLibDesc = {};
	dxilLibDesc.DXILLibrary.BytecodeLength = compiledShaders->GetBufferSize();
	dxilLibDesc.DXILLibrary.pShaderBytecode = compiledShaders->GetBufferPointer();
	dxilLibDesc.NumExports = _countof(exports);
	dxilLibDesc.pExports = exports;

	D3D12_STATE_SUBOBJECT dxilLibSubobject = {};
	dxilLibSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
	dxilLibSubobject.pDesc = &dxilLibDesc;


	D3D12_HIT_GROUP_DESC hitGroupDesc = {};
	hitGroupDesc.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
	hitGroupDesc.HitGroupExport = s_hitgroup_1.c_str();
	hitGroupDesc.ClosestHitShaderImport = exports[2].Name;

	
	D3D12_STATE_SUBOBJECT hitGroup_1SubObject = {};
	hitGroup_1SubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
	hitGroup_1SubObject.pDesc = &hitGroupDesc;


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

	D3D12_STATE_SUBOBJECT subobjects[] = {
				dxilLibSubobject,
				hitGroup_1SubObject,
				shaderConfigSubObject,
				globalRootSigSubObject,
				pipelineConfigSubobject
	};

	D3D12_STATE_OBJECT_DESC stateObjectDesc = {};
	stateObjectDesc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
	stateObjectDesc.NumSubobjects = _countof(subobjects);
	stateObjectDesc.pSubobjects = subobjects;

	m_dxrDevice->CreateStateObject(
				&stateObjectDesc,
				IID_PPV_ARGS(&m_rtpso));
}

VOID Dx12Raytracing::BuildShaderTables()
{
	ComPtr<ID3D12StateObjectProperties> props;
	m_rtpso->QueryInterface(IID_PPV_ARGS(&props));

	void* raygenShaderID = props->GetShaderIdentifier(L"MyRaygenShader");
	void* missShaderID   = props->GetShaderIdentifier(L"MyMissShader");
	void* hitgroupID     = props->GetShaderIdentifier(s_hitgroup_1.c_str());

	assert(raygenShaderID != nullptr && missShaderID != nullptr && hitgroupID != nullptr);

	const UINT shaderRecordSize = D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT; // 32 bytes
	const UINT sectionAlignment = D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;  //64 bytes alignment

	UINT64 rayGenTableSize   = shaderRecordSize;
	UINT64 rayGenAlignedSize = dxhelper::DxAlign(rayGenTableSize, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);

	UINT64 hitGroupTableSize   = shaderRecordSize;
	UINT64 hitGroupAlignedSize = dxhelper::DxAlign(hitGroupTableSize, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);

	UINT64 missTableSize        = shaderRecordSize;
	UINT64 missTableAlignedSize = dxhelper::DxAlign(missTableSize, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);


	const UINT64 sbtTableTotalSize = rayGenAlignedSize + hitGroupAlignedSize + missTableAlignedSize;
	BYTE* sbtDataStart = static_cast<BYTE*>(malloc(sbtTableTotalSize));
	BYTE* sbtDataWritePtr = sbtDataStart;
	memcpy(sbtDataWritePtr, raygenShaderID, rayGenTableSize);
	sbtDataWritePtr += rayGenAlignedSize;

	memcpy(sbtDataWritePtr, hitgroupID, hitGroupTableSize);
	sbtDataWritePtr += hitGroupAlignedSize;

	memcpy(sbtDataWritePtr, missShaderID, missTableSize);

	m_shaderBindingTable = CreateBufferWithData(sbtDataStart, sbtTableTotalSize, L"ShaderBindingTable");

	m_rayGenBaseAddress.StartAddress = m_shaderBindingTable->GetGPUVirtualAddress();
	m_rayGenBaseAddress.SizeInBytes = shaderRecordSize;

	m_hitTableBaseAddress.StartAddress = m_rayGenBaseAddress.StartAddress + rayGenAlignedSize;
	m_hitTableBaseAddress.SizeInBytes = shaderRecordSize;
	m_hitTableBaseAddress.StrideInBytes = shaderRecordSize;

	m_missTableBaseAddress.StartAddress = m_hitTableBaseAddress.StartAddress + hitGroupAlignedSize;
	m_missTableBaseAddress.SizeInBytes = shaderRecordSize;
	m_missTableBaseAddress.StrideInBytes = shaderRecordSize;

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
	CreateAppUavDescriptorAtIndex(0, m_uavOutputResource.Get());
}

HRESULT Dx12Raytracing::RenderFrame()
{
	static BOOL vbIbTransitioned = FALSE;
	auto indexBufferRes = GetModelIndexBufferResource();
	auto indexBufferView = GetModelIndexBufferView(0);
	CreateAppBufferSrvDescriptorAtIndex(1, indexBufferRes, indexBufferView.SizeInBytes / 4, 4);

	auto uvVbBufferRes  = GetModelMainTextureUVBufferResource();
	auto uvVbView       = GetModelMainTextureUVBufferView();


	CreateAppBufferSrvDescriptorAtIndex(2, uvVbBufferRes, uvVbView.SizeInBytes / 4, 4);

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
	m_dxrCommandList->SetComputeRootShaderResourceView(2, m_tlasResultBuffer->GetGPUVirtualAddress());

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
