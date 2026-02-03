#pragma once

#include "resource.h"
#include "Dx12SampleBase.h"

using namespace Microsoft::WRL;

class Dx12Tessellation : public Dx12SampleBase
{
public:
	Dx12Tessellation(UINT width, UINT height);
	virtual HRESULT RenderFrame() override;

protected:
	virtual inline UINT NumRTVsNeededForApp() override { return 1; }
	virtual inline UINT NumSRVsNeededForApp() override { return 1; }
	virtual inline UINT NumDSVsNeededForApp() override { return 1; }
	virtual inline const std::string GltfFileName() override { return "deer.gltf"; }

	virtual HRESULT CreatePipelineStateFromModel() override;

	virtual inline UINT NumRootConstantsForApp() override { return 1; }
	virtual inline VOID AppSetRootConstantForModel(ID3D12GraphicsCommandList* pCmdList, UINT rootParamIdx) override {
		UINT bits = *reinterpret_cast<UINT*>(&m_tesstriTessLevel);
		pCmdList->SetGraphicsRoot32BitConstant(rootParamIdx, bits, 0);
	}

private:
	FLOAT m_tesstriTessLevel;

};
