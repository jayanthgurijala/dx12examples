/*
* Copyright (C) 2026 by Jayanth Gurijala
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/
#pragma once

#include "resource.h"
#include "Dx12SampleBase.h"

using namespace Microsoft::WRL;

class Dx12HelloForest : public Dx12SampleBase
{
public:
	Dx12HelloForest(UINT width, UINT height);
	virtual HRESULT RenderFrame() override;

protected:
	virtual inline UINT NumRTVsNeededForApp() override { return 1; }
	virtual inline UINT NumSRVsPerPrimNeededForApp() override { return 0; }
	virtual inline UINT NumDSVsNeededForApp() override { return 1; }
	virtual inline const std::vector<std::string> GltfFileName() override { return {"terrain_gridlines.gltf", "deer.gltf", "chinesedragon.gltf", "oaktree.gltf" }; }
	virtual inline ID3D12RootSignature* GetRootSignature() override { return m_pRootSignature.Get(); }
	virtual HRESULT OnInit() override;
	virtual VOID LoadSceneDescription(std::vector<DxSceneElementInstance>& sceneDescription) override;
	virtual inline std::array<FLOAT, 4> RenderTargetClearColor() { return{ 0.2f, 0.2f, 0.3f, 1.0f }; }

private:
	ComPtr<ID3D12RootSignature> m_pRootSignature;

	VOID InitChineseDragon(DxSceneElementInstance& sceneElement, UINT localIdx = 0);
	VOID InitTerrain(DxSceneElementInstance& sceneElement, UINT localIdx = 0);
	VOID InitAnimalsDeer(DxSceneElementInstance& sceneElement, UINT localIdx = 0);
	VOID InitOakTrees(DxSceneElementInstance& sceneElement, UINT localIdx = 0);

};


