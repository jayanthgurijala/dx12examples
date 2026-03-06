/*
* Copyright (C) 2026 by Jayanth Gurijala
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/
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
	virtual inline UINT NumRTVsNeededForApp()         override { return 1; }

	virtual inline UINT NumSRVsPerPrimNeededForApp()         override 
	{
        return NumSrvsForRaytracing(); //uv buffer srv and index buffer srv
	} 
	
	virtual inline UINT NumDSVsNeededForApp()         override { return 1; }
	virtual inline UINT NumUAVsNeededForApp()         override { return 1; }
	virtual inline UINT NumRootSrvDescriptorsForApp() override { return 1; }
	virtual inline const std::string GltfFileName() override { return "oaktree.gltf"; }

private:
	VOID BuildBlasAndTlas();
	VOID CreateRtPSO();
	VOID BuildShaderTables();
	VOID CreateUAVOutput();

	inline UINT NumSrvsForRaytracing() { return 3; }

	ComPtr<ID3D12Device5>              m_dxrDevice;
	ComPtr<ID3D12GraphicsCommandList4> m_dxrCommandList;

	std::vector<ComPtr<ID3D12Resource>> m_blasResultBuffer;
	std::vector<ComPtr<ID3D12Resource>> m_blasScratchBuffer; //@todo remove this

	ComPtr<ID3D12Resource> m_instanceDescBuffer;
	ComPtr<ID3D12Resource> m_tlasScratchBuffer;
	ComPtr<ID3D12Resource> m_tlasResultBuffer;

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
	UINT m_tlasSrvRootParamIndex;
};


