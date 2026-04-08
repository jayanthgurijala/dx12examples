/*
* Copyright (C) 2026 by Jayanth Gurijala
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/
#pragma once

#include "resource.h"
#include "Dx12SampleBase.h"

using namespace Microsoft::WRL;

class Dx12Tessellation : public Dx12SampleBase
{
public:
	Dx12Tessellation(UINT width, UINT height);
	virtual VOID RenderFrameGfxDraw() override;

protected:
	virtual inline UINT NumRTVsNeededForApp() override { return 1; }
	virtual inline UINT NumDSVsNeededForApp() override { return 1; }
	virtual inline UINT NumSRVsPerPrimNeededForApp() override { return 0; }
	virtual inline const std::vector<std::string> GltfFileName() override { return { "deer.gltf" }; }

	virtual VOID OnInit() override;

	virtual inline UINT NumRootConstantsForApp() override { return 1; }
	virtual inline ID3D12RootSignature* GetRootSignature() override { return m_pRootSignature.Get(); }
	virtual inline VOID RenderFrame() override { RenderFrameGfxDraw(); }

	virtual inline std::string GetHullShaderName() override
	{
		return std::string("TessPassthrough_HS.cso");
	}

	virtual inline std::string GetDomainShaderName() override
	{
		return std::string("TessFactor3_DS.cso");
	}

private:
	ComPtr<ID3D12RootSignature> m_pRootSignature;
	FLOAT m_tesstriTessLevel;

};

