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

VOID Dx12HelloWorld::OnInit()
{
	CD3DX12_STATIC_SAMPLER_DESC staticSampler(0, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR);

	const UINT numDescRanges = 1;
    std::vector<CD3DX12_DESCRIPTOR_RANGE> descRanges(numDescRanges);

    const UINT numSrvsNeededForApp = NumSRVsInAllModelAssets();
	descRanges[0] = dxhelper::GetSRVDescRange(numSrvsNeededForApp);

	auto viewProj         = dxhelper::GetRootCbv(0);
	auto perInstance      = dxhelper::GetRootCbv(1);
	auto descTable        = dxhelper::GetRootDescTable(descRanges);
    auto materialsRootCbv = dxhelper::GetRootCbv(0, 3);


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

	CreatePerPrimGfxPipelineState();

}

VOID Dx12HelloWorld::RenderFrameGfxDraw()
{
	ImGui::Text("Hello World");

	ID3D12GraphicsCommandList* pCmdList = GetCmdList();
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = GetRtvCpuHandle(0);
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = GetDsvCpuHandle(0);

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


	ForEachModelAssetInstancesInScene([this, pCmdList, numSrvsPerPrim](UINT modelAssetIdx, UINT primIdx, UINT instanceIdx, UINT flatInstanceIdx)
		{
			auto& curPrimitive = GetPrimitiveInfo(modelAssetIdx, primIdx);
			pCmdList->SetPipelineState(curPrimitive.pipelineState.Get());
			pCmdList->SetGraphicsRootConstantBufferView(0, GetViewProjGpuVa());
			pCmdList->SetGraphicsRootConstantBufferView(1, GetPerInstanceDataGpuVa(flatInstanceIdx));
			pCmdList->SetGraphicsRootDescriptorTable(2, GetPerPrimSrvGpuHandle(curPrimitive.primLinearIdxInModelAssets));
			pCmdList->SetGraphicsRootConstantBufferView(3, curPrimitive.materialTextures.meterialCb);
			RenderModel(pCmdList, modelAssetIdx, primIdx);
		});

	AddFrameInfo(0, DxDescriptorTypeRtvSrv);

}

DX_ENTRY_POINT(Dx12HelloWorld);




