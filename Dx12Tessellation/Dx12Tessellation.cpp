// Dx12Tessellation.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "Dx12Tessellation.h"
#include "ExampleEntryPoint.h"
#include <d3dx12.h>
#include "DxPipelineInitializers.hpp"

Dx12Tessellation::Dx12Tessellation(UINT width, UINT height) :
	m_tesstriTessLevel(1),
	Dx12SampleBase(width, height)
{
}


HRESULT Dx12Tessellation::CreatePipelineStateFromModel()
{
	auto pDevice = GetDevice();

	ComPtr<ID3DBlob> vertexShader = GetCompiledShaderBlob("TessPassthrough_VS.cso");
	ComPtr<ID3DBlob> hullShader = GetCompiledShaderBlob("TessPassthrough_HS.cso");
	ComPtr<ID3DBlob> domainShader = GetCompiledShaderBlob("TessPassthrough_DS.cso");
	ComPtr<ID3DBlob> pixelShader = GetCompiledShaderBlob("TessPassthrough_PS.cso");

	assert(vertexShader != nullptr && hullShader != nullptr && domainShader != nullptr && pixelShader != nullptr);

	//m_modelPipelineState
	D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineStateDesc = {};
	pipelineStateDesc.InputLayout = {m_meshState.inputElementDesc.data(), static_cast<UINT>(m_meshState.inputElementDesc.size())};
	pipelineStateDesc.pRootSignature = m_modelRootSignature.Get();
	
	pipelineStateDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
	pipelineStateDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
	pipelineStateDesc.HS = CD3DX12_SHADER_BYTECODE(hullShader.Get());
	pipelineStateDesc.DS = CD3DX12_SHADER_BYTECODE(domainShader.Get());

	pipelineStateDesc.RasterizerState = dxinit::GetRasterizerState();
	pipelineStateDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	pipelineStateDesc.DepthStencilState = dxinit::GetDepthStencilState();

	///@todo write some test cases for this
	pipelineStateDesc.SampleMask = UINT_MAX;
	pipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
	pipelineStateDesc.NumRenderTargets = 1;
	pipelineStateDesc.RTVFormats[0] = GetBackBufferFormat();
	pipelineStateDesc.DSVFormat = GetDepthStencilFormat();

	///@todo experiment with flags
	pipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

	pipelineStateDesc.SampleDesc.Count = 1;

	pDevice->CreateGraphicsPipelineState(&pipelineStateDesc, IID_PPV_ARGS(&m_modelPipelineState));

	return S_OK;
}

HRESULT Dx12Tessellation::RenderFrame()
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

	pCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);
	RenderModel(pCmdList);

	RenderRtvContentsOnScreen(0);

	return S_OK;
}

DX_ENTRY_POINT(Dx12Tessellation);