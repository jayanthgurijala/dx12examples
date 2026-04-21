/*
* Copyright (C) 2026 by Jayanth Gurijala
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/
// Dx12HelloWorld.cpp : Defines the entry point for the application.
//

#include <d3d12.h>
#include "Dx12RayTracedForest.h"
#include "ExampleEntryPoint.h"
#include "framework.h"
#include <d3dx12.h>
#include "dxhelper.h"
#include "DxPrintUtils.h"
#include <imgui.h>
#include "DxTransformHelper.h"

using namespace DirectX;


Dx12RayTracedForest::Dx12RayTracedForest(UINT width, UINT height) :
	Dx12RaytracingBase(width, height)
{
}

VOID Dx12RayTracedForest::OnInit()
{
	Dx12RaytracingBase::OnInit();
}


VOID Dx12RayTracedForest::RenderFrame()
{

	ImGui::Text("Sponza RayTraced");

	AddFrameInfo(0, DxDescriptorTypeUavSrv);

	///@todo optimize this
	{
		CD3DX12_RESOURCE_BARRIER resourceBarrier[1];
		resourceBarrier[0] = CD3DX12_RESOURCE_BARRIER::UAV(m_uavOutputResource.Get());
		m_dxrCommandList->ResourceBarrier(1, resourceBarrier);
	}


	D3D12_DISPATCH_RAYS_DESC dispatchRaysDesc = {};
	dispatchRaysDesc.Width = GetWidth();
	dispatchRaysDesc.Height = GetHeight();
	dispatchRaysDesc.Depth = 1;
	dispatchRaysDesc.RayGenerationShaderRecord = m_rayGenBaseAddress;
	dispatchRaysDesc.HitGroupTable = m_hitTableBaseAddress;
	dispatchRaysDesc.MissShaderTable = m_missTableBaseAddress;

	ID3D12DescriptorHeap* descriptorHeaps[] = { GetSrvDescriptorHeap() };
	m_dxrCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
	m_dxrCommandList->SetPipelineState1(m_rtpso.Get());
	m_dxrCommandList->SetComputeRootSignature(m_rootSignature.Get());

	//Root args required - UAV in the descriptor heap and Accelaration structure
	m_dxrCommandList->SetComputeRootConstantBufferView(0, GetViewProjGpuVa()); //GetUavGpuHandle
	m_dxrCommandList->SetComputeRootConstantBufferView(1, GetPerInstanceDataGpuVa(0));
	m_dxrCommandList->SetComputeRootDescriptorTable(2, GetUavGpuHandle(0));
	m_dxrCommandList->SetComputeRootShaderResourceView(3, m_sceneTlas.sceneTlas.resultBuffer->GetGPUVirtualAddress());


	m_dxrCommandList->DispatchRays(&dispatchRaysDesc);

	{
		CD3DX12_RESOURCE_BARRIER resourceBarrier[1];
		resourceBarrier[0] = CD3DX12_RESOURCE_BARRIER::UAV(m_uavOutputResource.Get());
		m_dxrCommandList->ResourceBarrier(1, resourceBarrier);
	}
}

/*
*
* Order of SceneElements  Terrain, Deer, OakTree (foliage transparency last)
*
*/

VOID Dx12RayTracedForest::LoadSceneDescription(std::vector<DxSceneElementInstance>& sceneDescription)
{
	const UINT numSceneElementsLoaded = NumModelAssetsLoaded();
	sceneDescription.resize(numSceneElementsLoaded);

	auto& sceneElement = sceneDescription[0];

	sceneElement.sceneElementIdx = 0;

	sceneElement.addToExtents = FALSE;
	sceneElement.numInstances = 1;
	sceneElement.trsMatrix.resize(sceneElement.numInstances);

	auto& trsMatrix = sceneElement.trsMatrix[0];

	DxTransformHelper::SetTranslationIdentityValues(trsMatrix);
	DxTransformHelper::SetRotationIdentityDegrees(trsMatrix);
	DxTransformHelper::SetScaleValues(trsMatrix, 1.0f, 1.0f, 1.0f);
}




DX_ENTRY_POINT(Dx12RayTracedForest);
