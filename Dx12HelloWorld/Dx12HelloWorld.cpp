/*
* Copyright (C) 2026 by Jayanth Gurijala
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/
// Dx12HelloWorld.cpp : Defines the entry point for the application.
//

#include "Dx12HelloWorld.h"
#include "ExampleEntryPoint.h"
#include <d3dx12.h>
#include "dxhelper.h"
#include <imgui.h>

Dx12HelloWorld::Dx12HelloWorld(UINT width, UINT height) :
	Dx12SampleBase(width, height)
{
}

HRESULT Dx12HelloWorld::OnInit()
{
	CD3DX12_STATIC_SAMPLER_DESC staticSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR);

	dxhelper::DxCreateRootSignature(
		GetDevice(),
		&m_pRootSignature,
		{
			0_rcbv,
			"srv_5_0,uav_0_0,cbv_0_0"_dt,
			1_rcbv
		},
		{ staticSampler });

	CreateVSPSPipelineStateFromModel();

	return S_OK;
}

HRESULT Dx12HelloWorld::RenderFrame()
{
	ImGui::Text("Hello World");

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
	pCmdList->SetGraphicsRootSignature(m_pRootSignature.Get());
	ID3D12DescriptorHeap* descHeaps[] = { GetSrvDescriptorHeap() };
	pCmdList->SetDescriptorHeaps(_countof(descHeaps), descHeaps);

	
	pCmdList->SetGraphicsRootDescriptorTable(1, GetAppSrvGpuHandle(0));

	const UINT numNodes = NumNodesInScene();
	for (UINT nodeIdx = 0; nodeIdx < numNodes; nodeIdx++)
	{
		const UINT numPrims = NumPrimitivesInNodeMesh(nodeIdx);
		for (UINT primIdx = 0; primIdx < numPrims; primIdx++)
		{
			auto& curPrimitive = GetPrimitiveInfo(nodeIdx, primIdx);
			pCmdList->SetPipelineState(curPrimitive.pipelineState.Get());
			pCmdList->SetGraphicsRootConstantBufferView(0, GetNodeInfo(nodeIdx).gpuCameraData);
			pCmdList->SetGraphicsRootConstantBufferView(2, curPrimitive.materialTextures.meterialCb);
			RenderModel(pCmdList, nodeIdx, 0);
		}
	}

	SetFrameInfo(nullptr, 0);

	return S_OK;
}

DX_ENTRY_POINT(Dx12HelloWorld);




