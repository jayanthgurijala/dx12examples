#pragma once

#include "resource.h"
#include "Dx12SampleBase.h"

using namespace Microsoft::WRL;

class Dx12Raytracing : public Dx12SampleBase
{
public:
	Dx12Raytracing(UINT width, UINT height);
	virtual HRESULT RenderFrame() override;

	virtual HRESULT OnInit() override;

protected:
	virtual inline UINT NumRTVsNeededForApp() override { return 1; }
	virtual inline UINT NumSRVsNeededForApp() override { return 1; }
	virtual inline UINT NumDSVsNeededForApp() override { return 1; }
	virtual inline const std::string GltfFileName() override { return "deer.gltf"; }

private:
	VOID BuildBlasAndTlas();
	VOID CreateRtPSO();
	VOID BuildShaderTables();

	ComPtr<ID3D12Device5>              m_dxrDevice;
	ComPtr<ID3D12GraphicsCommandList4> m_dxrCommandList;

	ComPtr<ID3D12Resource> m_blasResultBuffer;
	ComPtr<ID3D12Resource> m_blasScratchBuffer;

	ComPtr<ID3D12Resource> m_instanceDescBuffer;
	ComPtr<ID3D12Resource> m_tlasScratchBuffer;
	ComPtr<ID3D12Resource> m_tlasResultBuffer;

	ComPtr<ID3D12RootSignature> m_globalRootSignature;

	ComPtr<ID3D12StateObject> m_rtpso;

	ComPtr<ID3D12Resource> m_shaderBindingTable;

	D3D12_GPU_VIRTUAL_ADDRESS_RANGE m_rayGenBaseAddress;
	D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE m_hitTableBaseAddress;
	D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE m_missTableBaseAddress;
};

