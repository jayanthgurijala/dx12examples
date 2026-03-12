/*
* Copyright (C) 2026 by Jayanth Gurijala
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/
// Dx12HelloWorld.cpp : Defines the entry point for the application.
//

#include "Dx12HelloForest.h"
#include "ExampleEntryPoint.h"
#include <d3dx12.h>
#include "dxhelper.h"
#include <imgui.h>

Dx12HelloForest::Dx12HelloForest(UINT width, UINT height) :
	Dx12SampleBase(width, height)
{
}

HRESULT Dx12HelloForest::OnInit()
{
	CD3DX12_STATIC_SAMPLER_DESC staticSampler(0, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR);

	const UINT numDescRanges = 1;
    std::vector<CD3DX12_DESCRIPTOR_RANGE> descRanges(numDescRanges);

    const UINT numSrvsNeededForApp = NumSRVsInScene(0);
	descRanges[0] = dxhelper::GetSRVDescRange(numSrvsNeededForApp);

	auto rootCbv          = dxhelper::GetRootCbv();
	auto descTable        = dxhelper::GetRootDescTable(descRanges);
    auto materialsRootCbv = dxhelper::GetRootCbv(1);


	dxhelper::DxCreateRootSignature(
		GetDevice(),
		&m_pRootSignature,
		{
			rootCbv,
			descTable,
			materialsRootCbv
		},
		{ staticSampler });

	CreateVSPSPipelineStateFromModel();

	return S_OK;
}

HRESULT Dx12HelloForest::RenderFrame()
{
	ImGui::Text("Hello World");

	ID3D12GraphicsCommandList* pCmdList = GetCmdList();
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = GetRenderTargetView(0, FALSE);
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = GetDsvCpuHeapHandle(0);

	pCmdList->OMSetRenderTargets(1,
		&rtvHandle,
		FALSE,                   //RTsSingleHandleToDescriptorRange
		&dsvHandle);

	pCmdList->ClearRenderTargetView(rtvHandle,
		RenderTargetClearColor().data(),
		0,
		nullptr);

	pCmdList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	pCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pCmdList->SetGraphicsRootSignature(m_pRootSignature.Get());
	ID3D12DescriptorHeap* descHeaps[] = { GetSrvDescriptorHeap() };
	pCmdList->SetDescriptorHeaps(_countof(descHeaps), descHeaps);


	const UINT numSrvsPerPrim         = NumSRVsPerPrimitive();
	const UINT numElementsInSceneLoad = NumElementsInSceneLoad();

	ProcessSceneInstancesNodesPrims([this, pCmdList, numSrvsPerPrim](UINT sceneEleIdx, UINT instanceIdx, UINT nodeIdx, UINT primIdx, UINT flatInstanceNodeIdx, UINT primIdxInObject, UINT sceneElementIdx)
	{
			auto& curPrimitive = GetPrimitiveInfo(sceneElementIdx, nodeIdx, primIdx);
			auto& sceneLoadElement = SceneElementInstance(sceneEleIdx);
			pCmdList->SetPipelineState(curPrimitive.pipelineState.Get());
			pCmdList->SetGraphicsRootConstantBufferView(0, sceneLoadElement.instanceCameraGpuVa[flatInstanceNodeIdx]);
			pCmdList->SetGraphicsRootDescriptorTable(1, GetAppSrvGpuHandle(primIdxInObject * numSrvsPerPrim));
			pCmdList->SetGraphicsRootConstantBufferView(2, curPrimitive.materialTextures.meterialCb);
			RenderModel(pCmdList, sceneLoadElement.sceneElementIdx, nodeIdx, primIdx);
	});


	SetFrameInfo(nullptr, 0);

	return S_OK;
}


/*
* 
* Order of SceneElements  Terrain, Deer, OakTree (foliage transparency last)
* 
*/

VOID Dx12HelloForest::LoadSceneDescription(std::vector<DxSceneElementInstance>& sceneDescription)
{
	const UINT numSceneElementsLoaded = NumSceneElementsLoaded();
	const UINT numSceneElements = NumSceneElementsLoaded() + 1;
	sceneDescription.resize(numSceneElements);

	UINT terrainIdx = 0;
	UINT deerIdx    = 0;
	UINT oakTreeIdx = 0;

	for (UINT idx = 0; idx < numSceneElements; idx++)
	{
		auto& currentSceneElement = sceneDescription[idx];
		currentSceneElement.sceneElementIdx = (idx < numSceneElementsLoaded) ? idx : numSceneElementsLoaded - 1 ;

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
		else if (idx == 2 || idx == 3)
		{
			InitOakTrees(currentSceneElement, oakTreeIdx);
			oakTreeIdx++;
		}
	}
}

VOID Dx12HelloForest::InitTerrain(DxSceneElementInstance& sceneElement, UINT localIdx)
{
	sceneElement.numInstances = 1;
	sceneElement.addToExtents = FALSE;

	sceneElement.trsMatrix.resize(sceneElement.numInstances);
	auto& trsMatrix = sceneElement.trsMatrix[0];

	trsMatrix.translation[0] = 0.0f;
	trsMatrix.translation[1] = -.04f;
	trsMatrix.translation[2] = 0.0f;

	trsMatrix.rotationInDegrees[0] = 0.0f;
	trsMatrix.rotationInDegrees[1] = 0.0f;
	trsMatrix.rotationInDegrees[2] = 0.0f;

	trsMatrix.scale[0] = 1.0f;
	trsMatrix.scale[1] = 1.0f;
	trsMatrix.scale[2] = 1.0f;
}

VOID Dx12HelloForest::InitAnimalsDeer(DxSceneElementInstance& sceneElement, UINT localIdx)
{
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

VOID Dx12HelloForest::InitOakTrees(DxSceneElementInstance& sceneElement, UINT localIdx)
{
	sceneElement.numInstances = 1;
	sceneElement.addToExtents = FALSE;
	

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

DX_ENTRY_POINT(Dx12HelloForest);




