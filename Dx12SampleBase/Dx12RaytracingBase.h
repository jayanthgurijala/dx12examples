#pragma once
#include "Dx12SampleBase.h"


class Dx12RaytracingBase :
	public Dx12SampleBase
{
public:

	Dx12RaytracingBase(UINT width, UINT height);
	virtual VOID OnInit() override;

protected:

	virtual inline UINT MaxRecursionDepth() { return 2; }
	virtual inline std::string SerializedFilePrefix() { return "common"; }

	virtual inline BOOL SerializeBlas() { return FALSE; }
	virtual inline BOOL SerializeTlas() { return FALSE; }

	virtual inline BOOL DeSerializeBlas() { return FALSE; }
	virtual inline BOOL DeSerializeTlas() { return FALSE; }

	VOID CreateGlobalRootSignature();
	VOID CreateLocalRootSignature();
	VOID CreatePerPrimSrvs();
	VOID CreateRayTracingStateObject();
	VOID BuildShaderTables();
	VOID CreateUAVOutput();
	VOID BuildBlasAndTlas();


	ComPtr<ID3D12Device5>              m_dxrDevice;
	ComPtr<ID3D12GraphicsCommandList4> m_dxrCommandList;


	ComPtr<ID3D12StateObject> m_rtpso;

	ComPtr<ID3D12Resource> m_shaderBindingTable;

	D3D12_GPU_VIRTUAL_ADDRESS_RANGE m_rayGenBaseAddress;
	D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE m_hitTableBaseAddress;
	D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE m_missTableBaseAddress;

	ComPtr<ID3D12Resource>      m_uavOutputResource;
	ComPtr<ID3D12RootSignature> m_rootSignature;
	ComPtr<ID3D12RootSignature> m_localRootSignature;

	//@note e.g consider loading (1) Deer (2) OakTree (3) Terrain
	//      1) Deer    - has one primitive in one blas
	//      2) OakTree - has two primitives in one blas
	//      3) Terrain - has one primitive in one blas
	DxSceneBlasDesc m_sceneBlas;
	DxSceneTlasDesc m_sceneTlas;
private:
	inline std::string GetBlasSerializedFileName(UINT index)
	{
		return SerializedFilePrefix() + "_blas_" + std::to_string(index) + ".bin";
	}

	inline std::string GetTlasSerializedFileName(UINT index)
	{
		return SerializedFilePrefix() + "_tlas_" + std::to_string(index) + ".bin";
	}

	VOID SerializeBlasTlas(D3D12_GPU_VIRTUAL_ADDRESS blasGpuVa, const char* fileName, const char* resourceName);
	UINT DeSerializeBlasTlas(ComPtr<ID3D12Resource>&                      pResource,
						     D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE accelType,
							 UINT                                         numGpuVas,
						     D3D12_GPU_VIRTUAL_ADDRESS*                   pBlasGpuVa,
						     const char*                                  fileName,
						     const char*                                  resourceName);
	
};

