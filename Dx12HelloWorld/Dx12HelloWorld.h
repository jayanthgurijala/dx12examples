#pragma once

#include "resource.h"
#include "Dx12SampleBase.h"

using namespace Microsoft::WRL;

class Dx12HelloWorld : public Dx12SampleBase
{
public:
	Dx12HelloWorld(UINT width, UINT height);
	virtual HRESULT RenderFrame() override;

protected:
	virtual inline UINT NumRTVsNeededForApp() override { return 1; }
	virtual inline UINT NumSRVsNeededForApp() override { return 1; }
	virtual inline UINT NumDSVsNeededForApp() override { return 1; }
	virtual inline const std::string GltfFileName() override { return "deer.gltf"; }

private:

};

