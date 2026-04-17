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
#include "DxTransformHelper.h"

Dx12HelloForest::Dx12HelloForest(UINT width, UINT height) :
	Dx12SampleBase(width, height)
{
	m_finalDraw = std::make_unique<DxGfxDrawRenderObject>(0, 1, 0, TRUE, 0);
	m_shadowPass = std::make_unique<DxGfxDrawRenderObject>(0, 1, 1, TRUE, 1);

	m_numRTVs = m_finalDraw->NumRTVs() + m_shadowPass->NumRTVs();
	m_numDSVs = m_finalDraw->NumDSVs() + m_shadowPass->NumDSVs();
}

VOID Dx12HelloForest::OnInit()
{
	CD3DX12_STATIC_SAMPLER_DESC staticSampler(0, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR);

	const UINT numDescRanges = 1;
    std::vector<CD3DX12_DESCRIPTOR_RANGE> descRanges(numDescRanges);

    const UINT numSrvsNeededForApp = NumSRVsInAllModelAssets();
	descRanges[0] = dxhelper::GetSRVDescRange(numSrvsNeededForApp);

	auto sceneData        = dxhelper::GetRootCbv(0);
	auto worldData        = dxhelper::GetRootCbv(1);
	auto descTable        = dxhelper::GetRootDescTable(descRanges);
    auto materialsRootCbv = dxhelper::GetRootCbv(0,3);


	dxhelper::DxCreateRootSignature(
		GetDevice(),
		&m_pRootSignature,
		{
			sceneData,
			worldData,
			descTable,
			materialsRootCbv
		},
		{ staticSampler });

	CreatePerPrimGfxPipelineState();

}

VOID Dx12HelloForest::RenderFrameGfxDraw()
{
	ImGui::Text("Hello Forest");

	RenderGfxDrawInit(m_pRootSignature.Get(), 0);

	m_shadowPass->RenderInitViewProjRtvDsv();
	m_shadowPass->Render();

	m_finalDraw->RenderInitViewProjRtvDsv();
	m_finalDraw->Render();

	AddFrameInfo(0, DxDescriptorTypeRtvSrv);
}


/*
* 
* Order of SceneElements  Terrain, Deer, OakTree (foliage transparency last)
* 
*/

VOID Dx12HelloForest::LoadSceneDescription(std::vector<DxSceneElementInstance>& sceneDescription)
{

	const UINT numSceneElements = NumModelAssetsLoaded();
	sceneDescription.resize(numSceneElements);

	UINT terrainIdx = 0;
	UINT deerIdx    = 0;
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
		else if (idx == 2)
		{
			//InitChineseDragon(currentSceneElement, 0);
		} 
		else
		{
			InitOakTrees(currentSceneElement, oakTreeIdx);
			oakTreeIdx++;
		}
	}
}

VOID Dx12HelloForest::InitChineseDragon(DxSceneElementInstance& sceneElement, UINT localIdx)
{
	sceneElement.sceneElementIdx = 2;

	sceneElement.addToExtents = FALSE;
	sceneElement.numInstances = 1;
	sceneElement.trsMatrix.resize(sceneElement.numInstances);

	auto& trsMatrix = sceneElement.trsMatrix[0];

	DxTransformHelper::SetTranslationIdentityValues(trsMatrix);
	DxTransformHelper::SetRotationIdentityDegrees(trsMatrix);
	DxTransformHelper::SetScaleValues(trsMatrix, 0.3f, 0.3f, 0.3f);
}

VOID Dx12HelloForest::InitTerrain(DxSceneElementInstance& sceneElement, UINT localIdx)
{
	sceneElement.sceneElementIdx = 0;

	sceneElement.numInstances = 1;
	sceneElement.addToExtents = FALSE;

	sceneElement.trsMatrix.resize(sceneElement.numInstances);
	auto& trsMatrix = sceneElement.trsMatrix[0];

	DxTransformHelper::SetTranslationValues(trsMatrix, 0.0f, -0.04f, 0.0f);
	DxTransformHelper::SetRotationIdentityDegrees(trsMatrix);
	DxTransformHelper::SetScaleIdentityValues(trsMatrix);

}

VOID Dx12HelloForest::InitAnimalsDeer(DxSceneElementInstance& sceneElement, UINT localIdx)
{
	sceneElement.sceneElementIdx = 1;
	sceneElement.addToExtents = TRUE;
	sceneElement.numInstances = 1;
	sceneElement.trsMatrix.resize(sceneElement.numInstances);

	auto& trsMatrix = sceneElement.trsMatrix[0];

	DxTransformHelper::TranslationIdentity(trsMatrix);
	DxTransformHelper::SetRotationIdentityDegrees(trsMatrix);
	DxTransformHelper::SetScaleIdentityValues(trsMatrix);
}

VOID Dx12HelloForest::InitOakTrees(DxSceneElementInstance& sceneElement, UINT localIdx)
{
	sceneElement.sceneElementIdx = 3;
	sceneElement.numInstances = 1;
	sceneElement.addToExtents = TRUE;
	sceneElement.trsMatrix.resize(sceneElement.numInstances);

	auto& trsMatrix = sceneElement.trsMatrix[0];

	DxTransformHelper::SetTranslationValues(trsMatrix, 0.8f + localIdx * 0.8f, 0.0f, 0.8f);
	DxTransformHelper::SetRotationIdentityDegrees(trsMatrix);
	DxTransformHelper::SetScaleValues(trsMatrix, 3.0f, 3.0f, 3.0f);
}

DX_ENTRY_POINT(Dx12HelloForest);




