// Dx12HelloWorld.cpp : Defines the entry point for the application.
//

#include "Dx12HelloWorld.h"
#include "ExampleEntryPoint.h"
#include "framework.h"
#include <d3dx12.h>
#include "dxhelper.h"

Dx12HelloWorld::Dx12HelloWorld(UINT width, UINT height) :
	Dx12SampleBase(width, height)
{
}

HRESULT Dx12HelloWorld::CreatePipelineStateFromModel()
{
	CD3DX12_STATIC_SAMPLER_DESC staticSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR);
	dxhelper::DxCreateRootSignature(
		GetDevice(),
		&m_pRootSignature,
		{
			0_rcbv,
			"srv_1_0,uav_0_0,cbv_0_0"_dt,
		},
		{ staticSampler });
	Dx12SampleBase::CreatePipelineStateFromModel();
	return S_OK;
}

HRESULT Dx12HelloWorld::RenderFrame()
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

	pCmdList->SetPipelineState(m_modelPipelineState.Get());
	pCmdList->SetGraphicsRootSignature(m_pRootSignature.Get());
	ID3D12DescriptorHeap* descHeaps[] = { GetSrvDescriptorHeap() };
	pCmdList->SetDescriptorHeaps(_countof(descHeaps), descHeaps);

	pCmdList->SetGraphicsRootConstantBufferView(0, GetCameraBuffer());
	pCmdList->SetGraphicsRootDescriptorTable(1, GetAppSrvGpuHandle(0));

	RenderModel(pCmdList);

	SetFrameInfo(nullptr, 0);

	return S_OK;
}

DX_ENTRY_POINT(Dx12HelloWorld);



