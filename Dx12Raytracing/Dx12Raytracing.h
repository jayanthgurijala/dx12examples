/*
* Copyright (C) 2026 by Jayanth Gurijala
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/
#pragma once

#include "resource.h"
#include "Dx12RaytracingBase.h"

using namespace Microsoft::WRL;

class Dx12Raytracing : public Dx12RaytracingBase
{
public:
	Dx12Raytracing(UINT width, UINT height);
	virtual VOID RenderFrame() override;

	virtual VOID OnInit() override;

protected:
	virtual inline UINT NumRTVsNeededForApp()         override { return 0; }

	virtual inline UINT NumSRVsPerPrimNeededForApp()         override 
	{
        return NumSrvsForRaytracing(); //uv buffer srv and index buffer srv
	} 
	
	virtual inline UINT NumDSVsNeededForApp()         override { return 0; }
	virtual inline UINT NumUAVsNeededForApp()         override { return 1; }
	virtual inline UINT NumRootSrvDescriptorsForApp() override { return 1; }
	virtual inline const std::vector<std::string> GltfFileName() override { return { "chinesedragon.gltf" }; }

private:

	inline UINT NumSrvsForRaytracing() { return 4; }

};


