/*
* Copyright (C) 2026 by Jayanth Gurijala
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/
// Dx12HelloWorld.cpp : Defines the entry point for the application.
//

#include "Dx12HelloMesh.h"
#include "ExampleEntryPoint.h"
#include <d3dx12.h>
#include "dxhelper.h"
#include <imgui.h>

Dx12HelloMesh::Dx12HelloMesh(UINT width, UINT height) :
	Dx12SampleBase(width, height)
{
}

VOID Dx12HelloMesh::OnInit()
{
	Dx12SampleBase::OnInit();

	ID3D12Device* pDevice = GetDevice();
	assert(pDevice != nullptr);

	ID3D12GraphicsCommandList* pCmdList = GetCmdList();
	assert(pCmdList != nullptr);

	pDevice->QueryInterface(IID_PPV_ARGS(&m_meshDevice));
	assert(m_meshDevice != nullptr);

	pCmdList->QueryInterface(IID_PPV_ARGS(&m_meshCommandList));
	assert(m_meshCommandList != nullptr);

	CreateMeshPSO();
	CreatePerPrimSRVs();
}

VOID Dx12HelloMesh::CreateMeshPSO()
{
	D3DX12_MESH_SHADER_PIPELINE_STATE_DESC psoDesc = {};

	CD3DX12_STATIC_SAMPLER_DESC staticSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR);

	const UINT numDescRanges = 1;
	std::vector<CD3DX12_DESCRIPTOR_RANGE> descRanges(numDescRanges);

	const UINT numSrvsNeededForApp = NumSRVsInScene(0);
	descRanges[0] = dxhelper::GetSRVDescRange(numSrvsNeededForApp);

	auto viewProj         = dxhelper::GetRootCbv(0);
	auto perInstance      = dxhelper::GetRootCbv(1);
	auto descTable        = dxhelper::GetRootDescTable(descRanges);
	auto materialsRootCbv = dxhelper::GetRootCbv(2);


	dxhelper::DxCreateRootSignature(
		GetDevice(),
		&m_pRootSignature,
		{
			viewProj,
			perInstance,
			descTable,
			materialsRootCbv
		},
		{ staticSampler });

	ComPtr<ID3DBlob> meshShader = GetCompiledShaderBlob("HelloMesh_MS.cso");
	ComPtr<ID3DBlob> pixelShader = GetCompiledShaderBlob("HelloMesh_PS.cso");

	psoDesc.pRootSignature = GetRootSignature();
	psoDesc.AS = { nullptr, 0 }; // optional
	psoDesc.MS = CD3DX12_SHADER_BYTECODE(meshShader.Get());

	psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

	psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc.Count = 1;

	auto psoStream = CD3DX12_PIPELINE_MESH_STATE_STREAM(psoDesc);

	D3D12_PIPELINE_STATE_STREAM_DESC streamDesc;
	streamDesc.pPipelineStateSubobjectStream = &psoStream;
	streamDesc.SizeInBytes = sizeof(psoStream);

	m_meshDevice->CreatePipelineState(&streamDesc, IID_PPV_ARGS(&m_meshPipelineState));
}

VOID Dx12HelloMesh::CreatePerPrimSRVs()
{
	const UINT numSRVsForMaterials = AppSrvOffsetForPrim();
	const auto& curPrimInfo = GetPrimitiveInfo(0, 0, 0);
	
	CreateSrvBufferForPrimitive(curPrimInfo, GltfVertexAttribMax, numSRVsForMaterials + 0, TRUE);
	CreateSrvBufferForPrimitive(curPrimInfo, GltfVertexAttribPosition, numSRVsForMaterials + 1, FALSE);
	CreateSrvBufferForPrimitive(curPrimInfo, GltfVertexAttribTexcoord0, numSRVsForMaterials + 2, FALSE);
}

VOID Dx12HelloMesh::RenderFrame()
{
	ImGui::Text("Hello Mesh Shaders!");

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = GetRtvCpuHandle(0); // GetRenderTargetView(0, FALSE);
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = GetDsvCpuHandle(0);

	assert(rtvHandle.ptr != 0);
	assert(dsvHandle.ptr != 0);

	FLOAT clearColor[4] = { 0.5f, 0.5f, 0.5f, 1.0f };

	m_meshCommandList->OMSetRenderTargets(1,
									      &rtvHandle,
									      FALSE,                   //RTsSingleHandleToDescriptorRange
									      &dsvHandle);

	m_meshCommandList->ClearRenderTargetView(rtvHandle,
											clearColor,
											0,
											nullptr);

	m_meshCommandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	m_meshCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	m_meshCommandList->SetPipelineState(m_meshPipelineState.Get());
	m_meshCommandList->SetGraphicsRootSignature(m_pRootSignature.Get());
	ID3D12DescriptorHeap* descHeaps[] = { GetSrvDescriptorHeap() };
	m_meshCommandList->SetDescriptorHeaps(_countof(descHeaps), descHeaps);

	m_meshCommandList->SetGraphicsRootConstantBufferView(0, GetViewProjGpuVa());
	m_meshCommandList->SetGraphicsRootConstantBufferView(1, GetPerInstanceDataGpuVa(0));
	m_meshCommandList->SetGraphicsRootDescriptorTable(2, GetPerPrimSrvGpuHandle(0));

	m_meshCommandList->DispatchMesh(64, 1, 1);

	AddFrameInfo(0, DxDescriptorTypeRtvSrv);
}

DX_ENTRY_POINT(Dx12HelloMesh);




