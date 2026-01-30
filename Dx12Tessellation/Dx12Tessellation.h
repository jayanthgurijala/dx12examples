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

private:

};
