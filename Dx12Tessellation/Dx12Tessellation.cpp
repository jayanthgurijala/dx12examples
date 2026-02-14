/*
* Copyright (C) 2026 by Jayanth Gurijala
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/
// Dx12Tessellation.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "Dx12Tessellation.h"
#include "ExampleEntryPoint.h"
#include <d3dx12.h>
#include "dxhelper.h"
#include <imgui.h>

Dx12Tessellation::Dx12Tessellation(UINT width, UINT height) :
	m_tesstriTessLevel(2),
	Dx12SampleBase(width, height)
{
}


HRESULT Dx12Tessellation::OnInit()
{
	auto pDevice = GetDevice();

	CD3DX12_STATIC_SAMPLER_DESC staticSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR);
	dxhelper::DxCreateRootSignature(
		pDevice,
		&m_pRootSignature,
		{
			0_rcbv,
			"srv_1_0,uav_0_0,cbv_0_0"_dt,
			"1_1"_rc
		},
		{ staticSampler });

	ComPtr<ID3DBlob> vertexShader = GetCompiledShaderBlob("TessFactor_VS.cso");
	ComPtr<ID3DBlob> hullShader   = GetCompiledShaderBlob("TessFactor_HS.cso");
	ComPtr<ID3DBlob> domainShader = GetCompiledShaderBlob("TessFactor_DS.cso");
	ComPtr<ID3DBlob> pixelShader  = GetCompiledShaderBlob("TessFactor_PS.cso");

	assert(vertexShader != nullptr && hullShader != nullptr && domainShader != nullptr && pixelShader != nullptr);

	//m_modelPipelineState
	D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineStateDesc = {};
	pipelineStateDesc.InputLayout = {m_meshState.inputElementDesc.data(), static_cast<UINT>(m_meshState.inputElementDesc.size())};
	pipelineStateDesc.pRootSignature = m_pRootSignature.Get();
	
	pipelineStateDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
	pipelineStateDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
	pipelineStateDesc.HS = CD3DX12_SHADER_BYTECODE(hullShader.Get());
	pipelineStateDesc.DS = CD3DX12_SHADER_BYTECODE(domainShader.Get());

	pipelineStateDesc.RasterizerState = dxhelper::GetRasterizerState(D3D12_CULL_MODE_BACK, D3D12_FILL_MODE_WIREFRAME);
	pipelineStateDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	pipelineStateDesc.DepthStencilState = dxhelper::GetDepthStencilState();

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
	SetFrameInfo(nullptr, 0);

	ImGui::Text("Tessellation");
	ImGui::SameLine();

	if (ImGui::Button("-"))
		m_tesstriTessLevel -= 0.1f;

	ImGui::SameLine();

	ImGui::SetNextItemWidth(80);
	ImGui::InputFloat("##tess", &m_tesstriTessLevel, 0, 0);

	ImGui::SameLine();

	if (ImGui::Button("+"))
		m_tesstriTessLevel += 0.1f;

	// Clamp value
	m_tesstriTessLevel = (m_tesstriTessLevel > 20.0f) ? 20.0f : m_tesstriTessLevel;
	m_tesstriTessLevel = m_tesstriTessLevel;

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
	pCmdList->SetPipelineState(m_modelPipelineState.Get());
	pCmdList->SetGraphicsRootSignature(m_pRootSignature.Get());
	ID3D12DescriptorHeap* descHeaps[] = { GetSrvDescriptorHeap() };
	pCmdList->SetDescriptorHeaps(_countof(descHeaps), descHeaps);


	pCmdList->SetGraphicsRootConstantBufferView(0, GetCameraBuffer());
	pCmdList->SetGraphicsRootDescriptorTable(1, GetAppSrvGpuHandle(0));

	UINT bits = *reinterpret_cast<UINT*>(&m_tesstriTessLevel);
	pCmdList->SetGraphicsRoot32BitConstant(2, bits, 0);

	RenderModel(pCmdList);

	return S_OK;
}

DX_ENTRY_POINT(Dx12Tessellation);
