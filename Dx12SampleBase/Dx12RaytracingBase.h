#pragma once
#include "Dx12SampleBase.h"

struct RayPayload
{
	float color[4];
	UINT currentRecursionDepth;
};

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
	VOID CreateCollectionStateObject(ComPtr<ID3DBlob>& shaderBlob, ComPtr<ID3D12StateObject>& stateObject);
	VOID CompileShaderBlobs();
	VOID CreateRayTracingStateObject();
	VOID BuildShaderTables();
	VOID CreateUAVOutput();
	VOID BuildBlasAndTlas();


	ComPtr<ID3D12Device5>              m_dxrDevice;
	ComPtr<ID3D12GraphicsCommandList4> m_dxrCommandList;

	ComPtr<ID3DBlob> m_blobChsAhsMiss;

	ComPtr<ID3DBlob> m_blobRayGenSimple;
	ComPtr<ID3DBlob> m_blobRayGenBadSimple;
	ComPtr<ID3DBlob> m_blobRayGenInvertSimple;

	ComPtr<ID3D12StateObject> m_rtpso;

	ComPtr<ID3D12StateObject> m_rayGenSimpleSo;
	ComPtr<ID3D12StateObject> m_rayGenBadSo;
	ComPtr<ID3D12StateObject> m_rayGenInvertSo;

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

	inline CD3DX12_SHADER_BYTECODE GetShaderByteCodeFromBlob(ComPtr<ID3DBlob>& shaderBlob)
	{
		return CD3DX12_SHADER_BYTECODE(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize());
	}

	inline void AddShaderConfigSubObject(CD3DX12_STATE_OBJECT_DESC& stateObjectDesc)
	{
		auto shaderConfigSubObject = stateObjectDesc.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
		const UINT payloadSize = sizeof(RayPayload); //ray payload
		const UINT attributeSize = sizeof(FLOAT) * 2; //bary centrics
		shaderConfigSubObject->Config(payloadSize, attributeSize);
	}

	inline void AddPipelineConfigSubObject(CD3DX12_STATE_OBJECT_DESC& stateObjectDesc)
	{
		auto pipelineConfigSubObject = stateObjectDesc.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
		const UINT maxRecursionDepth = MaxRecursionDepth();
		pipelineConfigSubObject->Config(maxRecursionDepth);
	}

	inline void AddGlobalRootSignatureSubObject(CD3DX12_STATE_OBJECT_DESC& stateObjectDesc)
	{
		auto globalRootSigSubObject = stateObjectDesc.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
		globalRootSigSubObject->SetRootSignature(m_rootSignature.Get());
	}

	VOID SerializeBlasTlas(D3D12_GPU_VIRTUAL_ADDRESS blasGpuVa, const char* fileName, const char* resourceName);
	UINT DeSerializeBlasTlas(ComPtr<ID3D12Resource>&                      pResource,
						     D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE accelType,
							 UINT                                         numGpuVas,
						     D3D12_GPU_VIRTUAL_ADDRESS*                   pBlasGpuVa,
						     const char*                                  fileName,
						     const char*                                  resourceName);
	
};

