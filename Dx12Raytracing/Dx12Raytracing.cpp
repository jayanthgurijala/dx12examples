// Dx12HelloWorld.cpp : Defines the entry point for the application.
//

#include "Dx12Raytracing.h"
#include "ExampleEntryPoint.h"
#include "framework.h"
#include <d3dx12.h>
#include "dxhelper.hpp"
#include "DxPrintUtils.h"

Dx12Raytracing::Dx12Raytracing(UINT width, UINT height) :
	Dx12SampleBase(width, height)
{
}



HRESULT Dx12Raytracing::RenderFrame()
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

	return S_OK;
}


VOID Dx12Raytracing::BuildBlasAndTlas()
{
	StartBuildingAccelerationStructures();

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
	
	dxhelper::AllocateUAVBuffers(m_dxrDevice.Get(), bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes, &m_blasResultBuffer, L"BlasResult");
	dxhelper::AllocateUAVBuffers(m_dxrDevice.Get(), bottomLevelPrebuildInfo.ScratchDataSizeInBytes, &m_blasScratchBuffer, L"BlasScratch");

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
	dxhelper::AllocateUAVBuffers(m_dxrDevice.Get(), topLevelPrebuildInfo.ResultDataMaxSizeInBytes, &m_tlasResultBuffer);
	dxhelper::AllocateUAVBuffers(m_dxrDevice.Get(), topLevelPrebuildInfo.ScratchDataSizeInBytes, &m_tlasScratchBuffer);

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC topLevelBuildDesc = {};
	topLevelBuildDesc.Inputs = topLevelInputs;
	topLevelBuildDesc.DestAccelerationStructureData = m_tlasResultBuffer->GetGPUVirtualAddress();
	topLevelBuildDesc.ScratchAccelerationStructureData = m_tlasScratchBuffer->GetGPUVirtualAddress();
	m_dxrCommandList->BuildRaytracingAccelerationStructure(&topLevelBuildDesc, 0, nullptr);

	ExecuteBuildAccelerationStructures();
}


DX_ENTRY_POINT(Dx12Raytracing);