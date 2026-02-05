// Dx12HelloWorld.cpp : Defines the entry point for the application.
//

#include <d3d12.h>
#include "Dx12Raytracing.h"
#include "ExampleEntryPoint.h"
#include "framework.h"
#include <d3dx12.h>
#include "dxhelper.hpp"
#include "DxPrintUtils.h"

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

	BuildBlasAndTlas();
	CreateRtPSO();
	BuildShaderTables();

	return S_OK;
}


VOID Dx12Raytracing::BuildBlasAndTlas()
{
	StartBuildingAccelerationStructures();

	const D3D12_RESOURCE_STATES resultState  = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;

	//@note should this be in UAV? Sample apps use UAV but there is DebugLayer warning
	const D3D12_RESOURCE_STATES scratchState = D3D12_RESOURCE_STATE_COMMON;

	const D3D12_INDEX_BUFFER_VIEW indexBufferView   = GetModelIndexBufferView(0);
	const D3D12_VERTEX_BUFFER_VIEW vertexBufferView = GetModelVertexBufferView(0);
	const DxDrawPrimitive         drawInfo          = GetDrawInfo(0);

	D3D12_RAYTRACING_GEOMETRY_DESC geomDesc = {};
	geomDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
	geomDesc.Triangles.IndexBuffer = drawInfo.isIndexedDraw ? indexBufferView.BufferLocation : 0;
	geomDesc.Triangles.IndexCount  = drawInfo.isIndexedDraw ? drawInfo.numIndices : 0;
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
	
	dxhelper::AllocateUAVBuffers(m_dxrDevice.Get(), bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes, &m_blasResultBuffer, L"BlasResult", resultState);
	dxhelper::AllocateUAVBuffers(m_dxrDevice.Get(), bottomLevelPrebuildInfo.ScratchDataSizeInBytes, &m_blasScratchBuffer, L"BlasScratch", scratchState);

	PrintUtils::PrintBufferAddressRange(m_blasResultBuffer.Get());
	PrintUtils::PrintBufferAddressRange(m_blasScratchBuffer.Get());

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC bottomLevelBuildDesc = {};
	bottomLevelBuildDesc.Inputs = bottomLevelInputs;
	bottomLevelBuildDesc.DestAccelerationStructureData = m_blasResultBuffer->GetGPUVirtualAddress();
	bottomLevelBuildDesc.ScratchAccelerationStructureData = m_blasScratchBuffer->GetGPUVirtualAddress();
	m_dxrCommandList->BuildRaytracingAccelerationStructure(&bottomLevelBuildDesc, 0, nullptr);

	D3D12_RAYTRACING_INSTANCE_DESC instanceDesc = {};
	instanceDesc.Transform[0][0] = instanceDesc.Transform[1][1] = instanceDesc.Transform[2][2] = 1;
	instanceDesc.InstanceMask = 1;
	instanceDesc.AccelerationStructure = m_blasResultBuffer->GetGPUVirtualAddress();
	dxhelper::AllocateUAVBuffers(m_dxrDevice.Get(), sizeof(instanceDesc), &m_instanceDescBuffer);


	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS topLevelInputs = {};
	topLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	topLevelInputs.NumDescs = 1;
	topLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	topLevelInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
	topLevelInputs.InstanceDescs = m_instanceDescBuffer->GetGPUVirtualAddress();

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO topLevelPrebuildInfo = {};
	m_dxrDevice->GetRaytracingAccelerationStructurePrebuildInfo(&topLevelInputs, &topLevelPrebuildInfo);
	dxhelper::AllocateUAVBuffers(m_dxrDevice.Get(), topLevelPrebuildInfo.ResultDataMaxSizeInBytes, &m_tlasResultBuffer, L"TlasResult", resultState);
	dxhelper::AllocateUAVBuffers(m_dxrDevice.Get(), topLevelPrebuildInfo.ScratchDataSizeInBytes, &m_tlasScratchBuffer, L"TlasScratch", scratchState);

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC topLevelBuildDesc = {};
	topLevelBuildDesc.Inputs = topLevelInputs;
	topLevelBuildDesc.DestAccelerationStructureData = m_tlasResultBuffer->GetGPUVirtualAddress();
	topLevelBuildDesc.ScratchAccelerationStructureData = m_tlasScratchBuffer->GetGPUVirtualAddress();
	m_dxrCommandList->BuildRaytracingAccelerationStructure(&topLevelBuildDesc, 0, nullptr);

	ExecuteBuildAccelerationStructures();
}

VOID Dx12Raytracing::CreateRtPSO()
{
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
	hitGroupDesc.ClosestHitShaderImport = exports[2].ExportToRename;

	
	D3D12_STATE_SUBOBJECT hitGroup_1SubObject = {};
	hitGroup_1SubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
	hitGroup_1SubObject.pDesc = &hitGroupDesc;


	D3D12_RAYTRACING_SHADER_CONFIG shaderConfig = {};
	shaderConfig.MaxAttributeSizeInBytes = D3D12_RAYTRACING_MAX_ATTRIBUTE_SIZE_IN_BYTES;
	shaderConfig.MaxPayloadSizeInBytes = sizeof(RayPayload);

	D3D12_STATE_SUBOBJECT shaderConfigSubObject;
	shaderConfigSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
	shaderConfigSubObject.pDesc = &shaderConfig;

	m_globalRootSignature = CreateRTUAVOutAndASGlobalRootSig();
	D3D12_GLOBAL_ROOT_SIGNATURE globalRootSigDesc = {};
	globalRootSigDesc.pGlobalRootSignature = m_globalRootSignature.Get();

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
	
	BYTE* sbtData = static_cast<BYTE*>(malloc(shaderRecordSize * 3));
	memcpy(sbtData, raygenShaderID, shaderRecordSize);
	memcpy(sbtData + shaderRecordSize, hitgroupID, shaderRecordSize);
	memcpy(sbtData + 2 * shaderRecordSize, missShaderID, shaderRecordSize);

	m_shaderBindingTable = CreateBufferWithData(sbtData, shaderRecordSize * 3, FALSE, L"ShaderBindingTable");

	m_rayGenBaseAddress.StartAddress = m_shaderBindingTable->GetGPUVirtualAddress();
	m_rayGenBaseAddress.SizeInBytes = shaderRecordSize;

	m_hitTableBaseAddress.StartAddress = m_rayGenBaseAddress.StartAddress + shaderRecordSize;
	m_hitTableBaseAddress.SizeInBytes = shaderRecordSize;
	m_hitTableBaseAddress.StrideInBytes = shaderRecordSize;

	m_missTableBaseAddress.StartAddress = m_hitTableBaseAddress.StartAddress + shaderRecordSize;
	m_missTableBaseAddress.SizeInBytes = shaderRecordSize;
	m_missTableBaseAddress.StrideInBytes = shaderRecordSize;

	free(sbtData);
}

HRESULT Dx12Raytracing::RenderFrame()
{
	D3D12_DISPATCH_RAYS_DESC dispatchRaysDesc = {};
	dispatchRaysDesc.Width = GetWidth();
	dispatchRaysDesc.Height = GetHeight();
	dispatchRaysDesc.Depth = 1;
	dispatchRaysDesc.RayGenerationShaderRecord = m_rayGenBaseAddress;
	dispatchRaysDesc.HitGroupTable = m_hitTableBaseAddress;
	dispatchRaysDesc.MissShaderTable = m_missTableBaseAddress;

	m_dxrCommandList->SetPipelineState1(m_rtpso.Get());

	m_dxrCommandList->DispatchRays(&dispatchRaysDesc);
	return S_OK;
}

/*HRESULT Dx12Raytracing::RenderFrame()
{
	ID3D12GraphicsCommandList* pCmdList = GetCmdList();
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = GetRenderTargetView(0, FALSE);
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = GetDsvCpuHeapHandle(0);

	FLOAT clearColor[4] = { 0.7f, 0.7f, 1.0f, 1.0f };

	pCmdList->OMSetRenderTargets(1,
		&rtvHandle,
		FALSE,                   //RTsSingleHandleToDescriptorRange
		&dsvHandle);

	pCmdList->ClearRenderTargetView(rtvHandle,
		clearColor,
		0,
		nullptr);

	pCmdList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	pCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	RenderModel(pCmdList);

	RenderRtvContentsOnScreen(0);

	return S_OK;
}*/


DX_ENTRY_POINT(Dx12Raytracing);