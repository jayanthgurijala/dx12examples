#pragma once

#include "resource.h"
#include "Dx12SampleBase.h"

using namespace Microsoft::WRL;

class Dx12HelloMesh : public Dx12SampleBase
{
public:
	Dx12HelloMesh(UINT width, UINT height);
	virtual HRESULT RenderFrame() override;

protected:
	virtual inline UINT NumRTVsNeededForApp() override { return 1; }
	virtual inline UINT NumSRVsNeededForApp() override { return 3; }
	virtual inline UINT NumDSVsNeededForApp() override { return 1; }
	virtual inline const std::string GltfFileName() override { return "deer.gltf"; }
	virtual inline ID3D12RootSignature* GetRootSignature() override { return m_pRootSignature.Get(); }
	virtual HRESULT CreatePipelineStateFromModel() override;
	virtual HRESULT OnInit() override;


private:

	VOID CreateMeshPSO();

	ComPtr<ID3D12RootSignature> m_pRootSignature;
	ComPtr<ID3D12Device6>              m_meshDevice;
	ComPtr<ID3D12GraphicsCommandList6> m_meshCommandList;
	ComPtr<ID3D12PipelineState> m_meshPipelineState;

};

