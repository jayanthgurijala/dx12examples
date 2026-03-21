/*
* Copyright (C) 2026 by Jayanth Gurijala
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/
#pragma once

#include "resource.h"
#include "Dx12SampleBase.h"

using namespace Microsoft::WRL;

class Dx12RayTracedForest : public Dx12SampleBase
{
public:
	Dx12RayTracedForest(UINT width, UINT height);
	virtual VOID RenderFrame() override;

	virtual HRESULT OnInit() override;

protected:
	virtual inline UINT NumRTVsNeededForApp()         override { return 1; }

	virtual inline UINT NumSRVsPerPrimNeededForApp()         override 
	{
        return NumSrvsForRaytracing(); //uv buffer srv and index buffer srv
	} 
	
	virtual inline UINT NumDSVsNeededForApp()         override { return 1; }
	virtual inline UINT NumUAVsNeededForApp()         override { return 1; }
	virtual inline UINT NumRootSrvDescriptorsForApp() override { return 1; }
	virtual inline const std::vector<std::string> GltfFileName() override { return { "terrain_gridlines.gltf", "deer.gltf", "oaktree.gltf" }; }
	virtual VOID LoadSceneDescription(std::vector<DxSceneElementInstance>& sceneDescription) override;

private:
	VOID BuildBlasAndTlas();
	VOID CreateRtPSO();
	VOID BuildShaderTables();
	VOID CreateUAVOutput();

	VOID InitChineseDragon(DxSceneElementInstance& sceneElement, UINT localIdx = 0);
	VOID InitTerrain(DxSceneElementInstance& sceneElement, UINT localIdx = 0);
	VOID InitAnimalsDeer(DxSceneElementInstance& sceneElement, UINT localIdx = 0);
	VOID InitOakTrees(DxSceneElementInstance& sceneElement, UINT localIdx = 0);

	inline UINT NumSrvsForRaytracing() { return 3; }

	ComPtr<ID3D12Device5>              m_dxrDevice;
	ComPtr<ID3D12GraphicsCommandList4> m_dxrCommandList;


	ComPtr<ID3D12StateObject> m_rtpso;

	ComPtr<ID3D12Resource> m_shaderBindingTable;

	D3D12_GPU_VIRTUAL_ADDRESS_RANGE m_rayGenBaseAddress;
	D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE m_hitTableBaseAddress;
	D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE m_missTableBaseAddress;

	ComPtr<ID3D12Resource>      m_uavOutputResource;
	ComPtr<ID3D12RootSignature> m_rootSignature;
	ComPtr<ID3D12RootSignature> m_localRootSignature;

	UINT m_rootCbvIndex;
	UINT m_descTableIndex;

	//@note e.g consider loading (1) Deer (2) OakTree (3) Terrain
	//      1) Deer    - has one primitive in one blas
	//      2) OakTree - has two primitives in one blas
	//      3) Terrain - has one primitive in one blas
	DxSceneBlasDesc m_sceneBlas;
	DxSceneTlasDesc m_sceneTlas;
};


