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
	static BOOL vbIbTransitioned = FALSE;

    //@todo optimization - transition resources once in init and keep them in the required state for raytracing instead of transitioning every frame.
	// This is just to keep the sample code simple and focused on raytracing.
	auto uvVbBufferRes = GetModelUvVertexBufferResource(0, 0, 0, 0);
	auto indexBufferRes = GetModelIndexBufferResource(0, 0, 0);

	ImGui::Text("Ray Tracing");

	AddFrameInfo(0, DxDescriptorTypeUavSrv);

	//if (vbIbTransitioned == FALSE)
	{
		CD3DX12_RESOURCE_BARRIER resourceBarrier[3];
		resourceBarrier[0] = CD3DX12_RESOURCE_BARRIER::Transition(indexBufferRes,
			                                                        D3D12_RESOURCE_STATE_INDEX_BUFFER,
			                                                        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		resourceBarrier[1] = CD3DX12_RESOURCE_BARRIER::Transition(uvVbBufferRes,
																	D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
																	D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		resourceBarrier[2] = CD3DX12_RESOURCE_BARRIER::UAV(m_uavOutputResource.Get());

		vbIbTransitioned = TRUE;
		m_dxrCommandList->ResourceBarrier(3, resourceBarrier);
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
	m_dxrCommandList->SetComputeRootConstantBufferView(0, GetViewProjLightsGpuVa(0)); //GetUavGpuHandle
	m_dxrCommandList->SetComputeRootConstantBufferView(1, GetPerInstanceDataGpuVa(0));
	m_dxrCommandList->SetComputeRootDescriptorTable(2, GetUavGpuHandle(0));
	m_dxrCommandList->SetComputeRootShaderResourceView(3, m_sceneTlas.sceneTlas.resultBuffer->GetGPUVirtualAddress());


	m_dxrCommandList->DispatchRays(&dispatchRaysDesc);

	{
		CD3DX12_RESOURCE_BARRIER resourceBarrier[3];
		resourceBarrier[0] = CD3DX12_RESOURCE_BARRIER::Transition(indexBufferRes,
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_INDEX_BUFFER
			);
		resourceBarrier[1] = CD3DX12_RESOURCE_BARRIER::Transition(uvVbBufferRes,
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
			);
		resourceBarrier[2] = CD3DX12_RESOURCE_BARRIER::UAV(m_uavOutputResource.Get());

		vbIbTransitioned = TRUE;
		m_dxrCommandList->ResourceBarrier(3, resourceBarrier);
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
	const UINT numSceneElements = NumSceneElementsLoaded();
	sceneDescription.resize(numSceneElements);

	UINT terrainIdx = 0;
	UINT deerIdx = 0;
	UINT oakTreeIdx = 0;

	for (UINT idx = 0; idx < numSceneElements; idx++)
	{
		auto& currentSceneElement = sceneDescription[idx];
		if (idx == 0)
		{
			InitTerrain(currentSceneElement, terrainIdx);
			terrainIdx++;
		}
		else if (idx == 1)
		{
			InitAnimalsDeer(currentSceneElement, deerIdx);
			deerIdx++;
		}
		else
		{
			InitOakTrees(currentSceneElement, oakTreeIdx);
			oakTreeIdx++;
		}
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

	trsMatrix.translation[0] = 0.0f;
	trsMatrix.translation[1] = 0.0f;
	trsMatrix.translation[2] = 0.0f;

	trsMatrix.rotationInDegrees[0] = 0.0f;
	trsMatrix.rotationInDegrees[1] = 0.0f;
	trsMatrix.rotationInDegrees[2] = 0.0f;

	trsMatrix.scale[0] = 1.0f;
	trsMatrix.scale[1] = 1.0f;
	trsMatrix.scale[2] = 1.0f;
}

VOID Dx12RayTracedForest::InitOakTrees(DxSceneElementInstance& sceneElement, UINT localIdx)
{
	sceneElement.sceneElementIdx = 2;
	sceneElement.numInstances = 1;
	sceneElement.addToExtents = TRUE;
	sceneElement.trsMatrix.resize(sceneElement.numInstances);

	auto& trsMatrix = sceneElement.trsMatrix[0];
	trsMatrix.translation[0] = 0.8f + localIdx * 0.8f; //looking from rear of Deer moving right
	trsMatrix.translation[1] = 0.0f; //moving forward towards Deer nose
	trsMatrix.translation[2] = 0.8; //moving down

	trsMatrix.rotationInDegrees[0] = 0.0f;
	trsMatrix.rotationInDegrees[1] = 0.0f;
	trsMatrix.rotationInDegrees[2] = 0.0f;

	trsMatrix.scale[0] = 3.0f;
	trsMatrix.scale[1] = 3.0f;
	trsMatrix.scale[2] = 3.0f;
}

DX_ENTRY_POINT(Dx12RayTracedForest);
