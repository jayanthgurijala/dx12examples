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

using namespace DirectX;

//somewhat a grid
static const FLOAT OakTreePositions[][3] =
{
	{ -4.0f, -4.0, 0.8f },
    {  0.0f, -4.0, 0.8f },
	{ 4.0f, -4.0, 0.8f },
	{ 4.0f, 0.0, 0.8f },
	{ -4.0f, 4.0, 0.8f },
	{ -4.0f, 0.0, 0.8f },
    { 4.0f, 4.0, 0.8f },
	{ 0.0f, 4.0, 0.8f },


	{ -2.0f, -2.0, 0.8f },
	{  0.0f, -2.0, 0.8f },
	{ 2.0f, -2.0, 0.8f },
	{ 2.0f, 0.0, 0.8f },
	{ -2.0f, 2.0, 0.8f },
	{ -2.0f, 0.0, 0.8f },
	{ 2.0f, 2.0, 0.8f },
	{ 0.0f, 2.0, 0.8f },

};


//absolute positions
static const FLOAT DeerPositions[][3] =
{
	{ 0.0f, 0.0f, 0.0f },
	{ 4.0f, 0.0f, 3.0f },
	{ -4.0f, 0.0f, 3.0f },
	{ 4.0f, 0.0f, -3.0f },
	{ -4.0f, 0.0f, -3.0f },
    { 8.0f, 0.0f, 3.0f },
    {  -8.0f, 0.0f, 3.0f },
	{ 8.0f, 0.0f, -3.0f },
	{ -8.0f, 0.0f, -3.0f },
};


static const FLOAT DeerRotations[][3] =
{
	{ 0.0f, 0.0f, 0.0f },
	{ 0.0f, 90.0f, 0.0f },
	{ 0.0f, 45.0f, 0.0f },
	{ 0.0f, 30.0f, 0.0f },
	{ 0.0f, 10.0f, 0.0f },
	{ 0.0f, 80.0f, 0.0f },
	{ 0.0f, 0.0f, 0.0f },
	{ 0.0f, 90.0f, 0.0f },
	{ 0.0f, 45.0f, 0.0f },
	{ 0.0f, 30.0f, 0.0f },
	{ 0.0f, 10.0f, 0.0f },
	{ 0.0f, 80.0f, 0.0f },

};


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

	ImGui::Text("Ray Tracing");

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


	const UINT numSceneElementsLoaded = NumSceneElementsLoaded();

    const UINT numTerrainElements = 1;
    const UINT numDeerElements    = _countof(DeerPositions);
    const UINT numOakTreeElements = _countof(OakTreePositions);

    const UINT numSceneElements = numDeerElements + numOakTreeElements + numTerrainElements;
	sceneDescription.resize(numSceneElements);

	UINT currentDescIdx = 0;

	{
		auto& currentSceneElement = sceneDescription[currentDescIdx];
		InitTerrain(currentSceneElement, 0);
		currentDescIdx++;
	}

    for (UINT deerIdx = 0; deerIdx < numDeerElements; deerIdx++, currentDescIdx++)
	{
		auto& currentSceneElement = sceneDescription[currentDescIdx];
		InitAnimalsDeer(currentSceneElement, deerIdx);
	}

    for (UINT oakTreeIdx = 0; oakTreeIdx < numOakTreeElements; oakTreeIdx++, currentDescIdx++)
	{
		auto& currentSceneElement = sceneDescription[currentDescIdx];
		InitOakTrees(currentSceneElement, oakTreeIdx);
	}
}


VOID Dx12RayTracedForest::InitTerrain(DxSceneElementInstance& sceneElement, UINT localIdx)
{
	sceneElement.sceneElementIdx = 0;

	sceneElement.numInstances = 1;
	sceneElement.addToExtents = FALSE;

	sceneElement.trsMatrix.resize(sceneElement.numInstances);
	auto& trsMatrix = sceneElement.trsMatrix[0];

	trsMatrix.translation[0] = 0.0f;
	trsMatrix.translation[1] = -.04f;
	trsMatrix.translation[2] = 0.0f;

	if (localIdx == 0)
	{
		trsMatrix.rotationInDegrees[0] = 0.0f;
	}
	else
	{
		trsMatrix.rotationInDegrees[0] = 90.0f;
	}
	trsMatrix.rotationInDegrees[1] = 0.0f;
	trsMatrix.rotationInDegrees[2] = 0.0f;

	trsMatrix.scale[0] = 1.0f;
	trsMatrix.scale[1] = 1.0f;
	trsMatrix.scale[2] = 1.0f;
}

VOID Dx12RayTracedForest::InitAnimalsDeer(DxSceneElementInstance& sceneElement, UINT localIdx)
{
	sceneElement.sceneElementIdx = 1;
	sceneElement.addToExtents = TRUE;
	sceneElement.numInstances = 1;
	sceneElement.trsMatrix.resize(sceneElement.numInstances);

	auto& trsMatrix = sceneElement.trsMatrix[0];

	trsMatrix.translation[0] = DeerPositions[localIdx][0];
	trsMatrix.translation[1] = DeerPositions[localIdx][1];
	trsMatrix.translation[2] = DeerPositions[localIdx][2];

    trsMatrix.rotationInDegrees[0] = DeerRotations[localIdx][0];
    trsMatrix.rotationInDegrees[1] = DeerRotations[localIdx][1];
    trsMatrix.rotationInDegrees[2] = DeerPositions[localIdx][2];

	trsMatrix.scale[0] = 1.0f;
	trsMatrix.scale[1] = 1.0f;
	trsMatrix.scale[2] = 1.0f;
}

VOID Dx12RayTracedForest::InitOakTrees(DxSceneElementInstance& sceneElement, UINT localIdx)
{
	sceneElement.sceneElementIdx = 2;
	sceneElement.numInstances = 1;
	sceneElement.addToExtents = FALSE;
	sceneElement.trsMatrix.resize(sceneElement.numInstances);

	auto& trsMatrix = sceneElement.trsMatrix[0];


    trsMatrix.translation[0] = OakTreePositions[localIdx][0];
    trsMatrix.translation[1] = OakTreePositions[localIdx][1];
    trsMatrix.translation[2] = OakTreePositions[localIdx][2];

	trsMatrix.rotationInDegrees[0] = 0.0f;
	trsMatrix.rotationInDegrees[1] = 0.0f;
	trsMatrix.rotationInDegrees[2] = 0.0f;

	trsMatrix.scale[0] = 3.0f;
	trsMatrix.scale[1] = 3.0f;
	trsMatrix.scale[2] = 3.0f;
}

DX_ENTRY_POINT(Dx12RayTracedForest);
