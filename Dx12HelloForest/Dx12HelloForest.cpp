/*
* Copyright (C) 2026 by Jayanth Gurijala
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/
// Dx12HelloWorld.cpp : Defines the entry point for the application.
//

#include "Dx12HelloForest.h"
#include "ExampleEntryPoint.h"
#include <d3dx12.h>
#include "dxhelper.h"
#include <imgui.h>

Dx12HelloForest::Dx12HelloForest(UINT width, UINT height) :
	Dx12SampleBase(width, height)
{
}

HRESULT Dx12HelloForest::OnInit()
{
	CD3DX12_STATIC_SAMPLER_DESC staticSampler(0, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR);

	const UINT numDescRanges = 1;
    std::vector<CD3DX12_DESCRIPTOR_RANGE> descRanges(numDescRanges);

    const UINT numSrvsNeededForApp = NumSRVsInScene(0);
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

HRESULT Dx12HelloForest::RenderFrame()
{
	ImGui::Text("Hello World");

	ID3D12GraphicsCommandList* pCmdList = GetCmdList();
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = GetRenderTargetView(0, FALSE);
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = GetDsvCpuHeapHandle(0);

	pCmdList->OMSetRenderTargets(1,
		&rtvHandle,
		FALSE,                   //RTsSingleHandleToDescriptorRange
		&dsvHandle);

	pCmdList->ClearRenderTargetView(rtvHandle,
		RenderTargetClearColor().data(),
		0,
		nullptr);

	pCmdList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	pCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pCmdList->SetGraphicsRootSignature(m_pRootSignature.Get());
	ID3D12DescriptorHeap* descHeaps[] = { GetSrvDescriptorHeap() };
	pCmdList->SetDescriptorHeaps(_countof(descHeaps), descHeaps);

	UINT primIdxInScene       = 0;

	const UINT numSrvsPerPrim = NumSRVsPerPrimitive();

	const UINT numSceneElements = NumSceneElementsLoaded();

	for (UINT sceneIdx = 0; sceneIdx < numSceneElements; sceneIdx++)
	{
		const UINT numNodes = NumNodesInScene(sceneIdx);
		for (UINT nodeIdx = 0; nodeIdx < numNodes; nodeIdx++)
		{
			const UINT numPrims = NumPrimitivesInNodeMesh(sceneIdx, nodeIdx);
			for (UINT primIdx = 0; primIdx < numPrims; primIdx++)
			{
				auto& curPrimitive = GetPrimitiveInfo(sceneIdx, nodeIdx, primIdx);
				pCmdList->SetPipelineState(curPrimitive.pipelineState.Get());
				pCmdList->SetGraphicsRootConstantBufferView(0, GetNodeInfo(sceneIdx, nodeIdx).gpuCameraData);
				pCmdList->SetGraphicsRootDescriptorTable(1, GetAppSrvGpuHandle(primIdxInScene * numSrvsPerPrim));
				pCmdList->SetGraphicsRootConstantBufferView(2, curPrimitive.materialTextures.meterialCb);
				RenderModel(pCmdList, sceneIdx, nodeIdx, primIdx);
				primIdxInScene++;
			}
		}
	}

	SetFrameInfo(nullptr, 0);

	return S_OK;
}

DX_ENTRY_POINT(Dx12HelloForest);




