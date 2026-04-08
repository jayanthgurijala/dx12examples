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


VOID Dx12Tessellation::OnInit()
{
	auto pDevice = GetDevice();

	CD3DX12_STATIC_SAMPLER_DESC staticSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR);

	const UINT numDescRanges = 1;
	std::vector<CD3DX12_DESCRIPTOR_RANGE> descRanges(numDescRanges);

	const UINT numSrvsNeededForApp = NumSRVsInScene(0);
	descRanges[0] = dxhelper::GetSRVDescRange(numSrvsNeededForApp);

	auto viewProj = dxhelper::GetRootCbv(0);
	auto perInstance = dxhelper::GetRootCbv(0,3);
	auto descTable = dxhelper::GetRootDescTable(descRanges);
	auto rootConstant = dxhelper::GetRootConstants(1, 2);

	dxhelper::DxCreateRootSignature(
		pDevice,
		&m_pRootSignature,
		{
			viewProj,
			perInstance,
			descTable,
			rootConstant
		},
		{ staticSampler });

	auto& curPrimitive = GetPrimitiveInfo(0, 0, 0);

	std::vector<D3D12_INPUT_ELEMENT_DESC> inputElementsDesc;
	const UINT numAttributes = CreateInputElementDesc(curPrimitive.vertexBufferInfo, inputElementsDesc);



	char vertexShaderName[64];
	char hullShaderName[64];
	char domainShaderName[64];
	char pixelShaderName[64];
	snprintf(vertexShaderName, 64, "Simple3_VS.cso");
	snprintf(hullShaderName, 64, "TessPassthrough_HS.cso");
	snprintf(domainShaderName, 64, "TessFactor3_DS.cso");
	snprintf(pixelShaderName, 64, "SimplePS.cso", numAttributes);

	ComPtr<ID3DBlob> vertexShader = GetCompiledShaderBlob(vertexShaderName);
	ComPtr<ID3DBlob> hullShader   = GetCompiledShaderBlob(hullShaderName);
	ComPtr<ID3DBlob> domainShader = GetCompiledShaderBlob(domainShaderName);
	ComPtr<ID3DBlob> pixelShader  = GetCompiledShaderBlob(pixelShaderName);

	assert(vertexShader != nullptr && hullShader != nullptr && domainShader != nullptr && pixelShader != nullptr);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineStateDesc = {};
	pipelineStateDesc.InputLayout = { inputElementsDesc.data() , numAttributes };
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
	pipelineStateDesc.DSVFormat = GetDepthStencilDsvFormat();

	///@todo experiment with flags
	pipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

	pipelineStateDesc.SampleDesc.Count = 1;

	pDevice->CreateGraphicsPipelineState(&pipelineStateDesc, IID_PPV_ARGS(&m_modelPipelineState));
}

VOID Dx12Tessellation::RenderFrameGfxDraw()
{
	ImGui::Text("Tessellation");
	ImGui::SameLine();

	if (ImGui::Button("-##tessminus"))
		m_tesstriTessLevel -= 0.1f;

	ImGui::SameLine();

	ImGui::SetNextItemWidth(80);
	ImGui::InputFloat("##tess", &m_tesstriTessLevel, 0, 0);

	ImGui::SameLine();

	if (ImGui::Button("+##tessplus"))
		m_tesstriTessLevel += 0.1f;

	// Clamp value
	m_tesstriTessLevel = (m_tesstriTessLevel > 20.0f) ? 20.0f : m_tesstriTessLevel;
	m_tesstriTessLevel = m_tesstriTessLevel;

	ID3D12GraphicsCommandList* pCmdList = GetCmdList();
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = GetRtvCpuHandle(0);
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = GetDsvCpuHandle(0);

	pCmdList->OMSetRenderTargets(1,
		&rtvHandle,
		FALSE,                   //RTsSingleHandleToDescriptorRange
		&dsvHandle);

	pCmdList->ClearRenderTargetView(rtvHandle,
		RenderTargetClearColor().data(),
		0,
		nullptr);

	pCmdList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	pCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);
	pCmdList->SetPipelineState(m_modelPipelineState.Get());
	pCmdList->SetGraphicsRootSignature(m_pRootSignature.Get());
	ID3D12DescriptorHeap* descHeaps[] = { GetSrvDescriptorHeap() };
	pCmdList->SetDescriptorHeaps(_countof(descHeaps), descHeaps);


	pCmdList->SetGraphicsRootConstantBufferView(0, GetViewProjGpuVa());
	pCmdList->SetGraphicsRootConstantBufferView(1, GetPerInstanceDataGpuVa(0));
	pCmdList->SetGraphicsRootDescriptorTable(2, GetPerPrimSrvGpuHandle(0));

	UINT bits = *reinterpret_cast<UINT*>(&m_tesstriTessLevel);
	pCmdList->SetGraphicsRoot32BitConstant(3, bits, 0);

	RenderModel(pCmdList, 0, 0, 0);
	AddFrameInfo(0, DxDescriptorTypeRtvSrv);
}

DX_ENTRY_POINT(Dx12Tessellation);
