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
	CD3DX12_STATIC_SAMPLER_DESC staticSampler(0, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR);

	const UINT numDescRanges = 1;
    std::vector<CD3DX12_DESCRIPTOR_RANGE> descRanges(numDescRanges);

    const UINT numSrvsNeededForApp = NumSRVsInScene();
	descRanges[0] = dxhelper::GetSRVDescRange(numSrvsNeededForApp);

	auto rootCbv          = dxhelper::GetRootCbv();
	auto descTable        = dxhelper::GetRootDescTable(descRanges);
    auto materialsRootCbv = dxhelper::GetRootCbv(1);


	dxhelper::DxCreateRootSignature(
		GetDevice(),
		&m_pRootSignature,
		{
			rootCbv,
			descTable,
			materialsRootCbv
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

	FLOAT clearColor[4] = { 0.5f, 0.5f, 0.5f, 1.0f };

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

	UINT primIdxInScene       = 0;
	const UINT numSrvsPerPrim = NumSRVsPerPrimitive();
	const UINT numNodes = NumNodesInScene(0);

	for (UINT nodeIdx = 0; nodeIdx < numNodes; nodeIdx++)
	{
		const UINT numPrims =  NumPrimitivesInNodeMesh(0, nodeIdx);
		for (UINT primIdx = 0; primIdx < numPrims; primIdx++)
		{
			auto& curPrimitive = GetPrimitiveInfo(0, nodeIdx, primIdx);
			pCmdList->SetPipelineState(curPrimitive.pipelineState.Get());
			pCmdList->SetGraphicsRootConstantBufferView(0, GetNodeInfo(0, nodeIdx).gpuCameraData);
			pCmdList->SetGraphicsRootDescriptorTable(1, GetAppSrvGpuHandle(primIdxInScene * numSrvsPerPrim));
			pCmdList->SetGraphicsRootConstantBufferView(2, curPrimitive.materialTextures.meterialCb);
			RenderModel(pCmdList, nodeIdx, 0);
			primIdxInScene++;
		}
	}

	SetFrameInfo(nullptr, 0);

	return S_OK;
}

DX_ENTRY_POINT(Dx12HelloWorld);




