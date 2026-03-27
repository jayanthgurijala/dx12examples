/*
* Copyright (C) 2026 by Jayanth Gurijala
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/
#pragma once

#include "resource.h"
#include "Dx12RaytracingBase.h"

using namespace Microsoft::WRL;

class Dx12RayTracedForest : public Dx12RaytracingBase
{
public:
	Dx12RayTracedForest(UINT width, UINT height);
	virtual VOID RenderFrame() override;

	virtual VOID OnInit() override;

protected:
	virtual inline UINT NumRTVsNeededForApp()         override { return 1; }

	virtual inline UINT NumSRVsPerPrimNeededForApp()         override 
	{
        return NumSrvsForRaytracing();
	} 
	
	virtual inline UINT NumDSVsNeededForApp()         override { return 1; }
	virtual inline UINT NumUAVsNeededForApp()         override { return 1; }
	virtual inline UINT NumRootSrvDescriptorsForApp() override { return 1; }
	virtual inline const std::vector<std::string> GltfFileName() override { return { "terrain_gridlines.gltf", "deer.gltf", "oaktree.gltf" }; }
	virtual VOID LoadSceneDescription(std::vector<DxSceneElementInstance>& sceneDescription) override;

private:
	VOID InitTerrain(DxSceneElementInstance& sceneElement, UINT localIdx = 0);
	VOID InitAnimalsDeer(DxSceneElementInstance& sceneElement, UINT localIdx = 0);
	VOID InitOakTrees(DxSceneElementInstance& sceneElement, UINT localIdx = 0);

	/// Position, Normal, Index and UV buffers
	inline UINT NumSrvsForRaytracing() { return 4; }

};


