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
	m_finalDraw = std::make_unique<DxGfxDrawRenderObject>(0, 1, 0, TRUE, 0);
}


VOID Dx12Tessellation::OnInit()
{
	auto pDevice = GetDevice();

	CD3DX12_STATIC_SAMPLER_DESC staticSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR);

	const UINT numDescRanges = 1;
	std::vector<CD3DX12_DESCRIPTOR_RANGE> descRanges(numDescRanges);

	const UINT numSrvsNeededForApp = NumSRVsInAllModelAssets();
	descRanges[0] = dxhelper::GetSRVDescRange(numSrvsNeededForApp);

	auto viewProj         = dxhelper::GetRootCbv(0);
	auto perInstance      = dxhelper::GetRootCbv(1);
	auto descTable        = dxhelper::GetRootDescTable(descRanges);
	auto materialsRootCbv = dxhelper::GetRootCbv(0, 3);
	auto rootConstant     = dxhelper::GetRootConstants(1, 0, 2);


	dxhelper::DxCreateRootSignature(
		pDevice,
		&m_pRootSignature,
		{
			viewProj,
			perInstance,
			descTable,
			materialsRootCbv,
			rootConstant,

		},
		{ staticSampler });

	//auto& curPrimitive = GetPrimitiveInfo(0, 0);
	//
	//std::vector<D3D12_INPUT_ELEMENT_DESC> inputElementsDesc;
	//const UINT numAttributes = CreateInputElementDesc(curPrimitive.vertexBufferInfo, inputElementsDesc);

	CreatePerPrimGfxPipelineState();
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

	RenderGfxDrawInit(m_pRootSignature.Get(), 0, D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);
	m_finalDraw->RenderInitViewProjRtvDsv();

	ID3D12GraphicsCommandList* pCmdList = GetCmdList();
	UINT bits = *reinterpret_cast<UINT*>(&m_tesstriTessLevel);
	pCmdList->SetGraphicsRoot32BitConstant(4, bits, 0);

	m_finalDraw->Render();

	AddFrameInfo(0, DxDescriptorTypeRtvSrv);
}

DX_ENTRY_POINT(Dx12Tessellation);
