// Dx12HelloWorld.cpp : Defines the entry point for the application.
//

#include "Dx12HelloWorld.h"
#include "ExampleEntryPoint.h"
#include "framework.h"
#include <d3dx12.h>

Dx12HelloWorld::Dx12HelloWorld(UINT width, UINT height) :
	Dx12SampleBase(width, height)
{
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
	RenderModel(pCmdList);

	RenderRtvContentsOnScreen(0);

	return S_OK;
}

DX_ENTRY_POINT(Dx12HelloWorld);



