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

	CreateVSPSPipelineStateFromModel();

	return S_OK;
}

HRESULT Dx12HelloWorld::RenderFrameGfxDraw()
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

	const UINT numSrvsPerPrim = NumSRVsPerPrimitive();


	ProcessSceneInstancesNodesPrims([this, pCmdList, numSrvsPerPrim](UINT sceneEleIdx, UINT instanceIdx, UINT nodeIdx, UINT primIdx, UINT flatInstanceNodeIdx, UINT sceneElementIdx)
		{
			auto& curPrimitive = GetPrimitiveInfo(sceneElementIdx, nodeIdx, primIdx);
			auto& sceneLoadElement = SceneElementInstance(sceneEleIdx);
			pCmdList->SetPipelineState(curPrimitive.pipelineState.Get());
			pCmdList->SetGraphicsRootConstantBufferView(0, GetViewProjLightsGpuVa());
			pCmdList->SetGraphicsRootConstantBufferView(1, GetPerInstanceDataGpuVa(flatInstanceNodeIdx));
			pCmdList->SetGraphicsRootDescriptorTable(2, GetAppSrvGpuHandle(curPrimitive.materialTextures.descriptorHeapOffset));
			pCmdList->SetGraphicsRootConstantBufferView(3, curPrimitive.materialTextures.meterialCb);
			RenderModel(pCmdList, sceneLoadElement.sceneElementIdx, nodeIdx, primIdx);
		});

	SetFrameInfo(nullptr, 0);

	return S_OK;
}

DX_ENTRY_POINT(Dx12HelloWorld);




