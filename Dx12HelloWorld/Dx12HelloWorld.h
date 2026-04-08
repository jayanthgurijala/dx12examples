/*
* Copyright (C) 2026 by Jayanth Gurijala
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/
#pragma once

#include "resource.h"
#include "Dx12SampleBase.h"

using namespace Microsoft::WRL;

class Dx12HelloWorld : public Dx12SampleBase
{
public:
	Dx12HelloWorld(UINT width, UINT height);
	virtual VOID RenderFrameGfxDraw() override;

protected:
	virtual inline UINT NumRTVsNeededForApp() override { return 1; }
	virtual inline UINT NumSRVsPerPrimNeededForApp() override { return 0; }
	virtual inline UINT NumDSVsNeededForApp() override { return 1; }
	virtual inline const std::vector<std::string> GltfFileName() override { return { "lantern.gltf" }; }
	virtual inline ID3D12RootSignature* GetRootSignature() override { return m_pRootSignature.Get(); }
	virtual VOID OnInit() override;
	virtual inline VOID RenderFrame() override { RenderFrameGfxDraw(); }

private:
	ComPtr<ID3D12RootSignature> m_pRootSignature;

};


