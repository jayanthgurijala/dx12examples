#pragma once

#include <d3d12.h>
#include <d3dx12.h>
#include "DxPrintUtils.h"

using namespace Microsoft::WRL;

inline CD3DX12_ROOT_PARAMETER operator ""_rcbv(unsigned long long shaderRegister)
{
	auto rootCbv = CD3DX12_ROOT_PARAMETER();
	rootCbv.InitAsConstantBufferView(shaderRegister);
	return rootCbv;
}

inline CD3DX12_ROOT_PARAMETER operator ""_rsrv(unsigned long long shaderRegister)
{
	auto rootSrv = CD3DX12_ROOT_PARAMETER();
	rootSrv.InitAsShaderResourceView(shaderRegister);
	return rootSrv;
}

inline CD3DX12_ROOT_PARAMETER operator ""_uav(unsigned long long shaderRegister)
{
	auto rootUav = CD3DX12_ROOT_PARAMETER();
	rootUav.InitAsUnorderedAccessView(shaderRegister);
	return rootUav;
}

inline CD3DX12_ROOT_PARAMETER operator ""_rc(const char* str, size_t)
{
	UINT num32bitValues, shaderRegister;
	int numRead = sscanf_s(str, "%u_%u", &num32bitValues, &shaderRegister);
	auto rootConstant = CD3DX12_ROOT_PARAMETER();
	rootConstant.InitAsConstants(num32bitValues, shaderRegister);
	return rootConstant;
}

///@note syntax is "srv_count_base,uav_count_base,cbv_count,base"
inline CD3DX12_ROOT_PARAMETER operator ""_dt(const char* str, size_t)
{
	CD3DX12_ROOT_PARAMETER descriptorTable = {};

	UINT srvCount, srvBase, uavCount, uavBase, cbvCount, cbvBase;
	int numRead = sscanf_s(str, "srv_%u_%u,uav_%u_%u,cbv_%u_%u", &srvCount, &srvBase, &uavCount, &uavBase, &cbvCount, &cbvBase);
	auto rootDt = CD3DX12_ROOT_PARAMETER();
	UINT numRanges = 0;
	numRanges += (srvCount > 0) ? 1 : 0;
	numRanges += (uavCount > 0) ? 1 : 0;
	numRanges += (cbvCount > 0) ? 1 : 0;

	///@todo bad! Fix!
	static std::vector<CD3DX12_DESCRIPTOR_RANGE> descTableRanges;

	descTableRanges.resize(numRanges);

	UINT rangeIndex = 0;
	if (srvCount > 0)
	{
		descTableRanges[rangeIndex].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, srvCount, 0);
		rangeIndex++;
	}

	if (uavCount > 0)
	{
		descTableRanges[rangeIndex].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, uavCount, 0);
		rangeIndex++;
	}

	if (cbvCount > 0)
	{
		descTableRanges[rangeIndex].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, cbvCount, 0);
		rangeIndex++;
	}

	descriptorTable.InitAsDescriptorTable(numRanges, descTableRanges.data(), D3D12_SHADER_VISIBILITY_ALL);
	return descriptorTable;
}

namespace dxhelper
{
	inline D3D12_INPUT_LAYOUT_DESC GetInputLayoutDesc_Layout1()
	{
		D3D12_INPUT_LAYOUT_DESC layout;
		// Define the vertex input layout.
		static const D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
		};

		layout.NumElements = _countof(inputElementDescs);
		layout.pInputElementDescs = inputElementDescs;
		return layout;
	}

	inline void CreateRootSignature_Temp(ID3D12Device *pDevice, CD3DX12_ROOT_SIGNATURE_DESC& rootSignatureDesc, ID3D12RootSignature** ppRootSignature)
	{
		ComPtr<ID3DBlob> rootSigBlob;
		ComPtr<ID3DBlob> errorBlob;
		D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &rootSigBlob, &errorBlob);
		if (errorBlob != NULL)
		{
			OutputDebugStringA((char*)errorBlob->GetBufferPointer());
		}
		pDevice->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), IID_PPV_ARGS(ppRootSignature));
	}

	inline ComPtr<ID3D12RootSignature> GetSrvDescTableRootSignature(ID3D12Device* pDevice, UINT numSRVs)
	{
		ComPtr<ID3D12RootSignature> rootSignature;
		auto descriptorTable = CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
		CD3DX12_ROOT_PARAMETER rootParameters[1];
		rootParameters[0].InitAsDescriptorTable(1,
			&descriptorTable,
			D3D12_SHADER_VISIBILITY_PIXEL);
		CD3DX12_STATIC_SAMPLER_DESC staticSampler(0, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR);
		auto rootSignatureDesc = CD3DX12_ROOT_SIGNATURE_DESC(1, rootParameters, 1, &staticSampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
		CreateRootSignature_Temp(pDevice, rootSignatureDesc, &rootSignature);
		return rootSignature;
	}

	inline CD3DX12_RASTERIZER_DESC GetRasterizerState(D3D12_CULL_MODE cullMode = D3D12_CULL_MODE_BACK, D3D12_FILL_MODE fillMode = D3D12_FILL_MODE_SOLID)
	{
		CD3DX12_RASTERIZER_DESC rast(D3D12_DEFAULT);
		rast.CullMode = cullMode;
		rast.FillMode = fillMode;

		return rast;
	}

	inline CD3DX12_DEPTH_STENCIL_DESC GetDepthStencilState(BOOL enableDepth = TRUE, BOOL enableStencil = FALSE)
	{
		CD3DX12_DEPTH_STENCIL_DESC depthStencilState(D3D12_DEFAULT);
		depthStencilState.DepthEnable = enableDepth;
		depthStencilState.StencilEnable = enableStencil;
		return depthStencilState;
	}

	inline void AllocateBufferResource(ID3D12Device* pDevice,
		                               UINT64 bufferSizeInBytes,
		                               ID3D12Resource** ppResource,
		                               const wchar_t* resourceName = nullptr,
									   D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE,
		                               D3D12_RESOURCE_STATES initialResourceState = D3D12_RESOURCE_STATE_COMMON,
									   BOOL isUploadHeap = FALSE)
	{
		D3D12_HEAP_TYPE heapType = (isUploadHeap == FALSE) ? D3D12_HEAP_TYPE_DEFAULT : D3D12_HEAP_TYPE_GPU_UPLOAD;
		auto heapProps = CD3DX12_HEAP_PROPERTIES(heapType);
		auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSizeInBytes, flags);

		pDevice->CreateCommittedResource(&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			initialResourceState,
			nullptr,
			IID_PPV_ARGS(ppResource));
		if (resourceName)
		{
			(*ppResource)->SetName(resourceName);
		}
	}

	inline void AllocateTexture2DResource(ID3D12Device* pDevice,
		                                  ID3D12Resource** ppResource,
		                                  UINT64 width,
		                                  UINT height,
		                                  DXGI_FORMAT format,
										  const wchar_t* resourceName = nullptr,
									      D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON,
										  D3D12_CLEAR_VALUE* optimizedClearValue = nullptr,
		                                  D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE,
										  D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT)
	{
		auto heapProps = CD3DX12_HEAP_PROPERTIES(heapType);
		auto resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(format, width, height, 1, 1, 1, 0, flags);

		pDevice->CreateCommittedResource(&heapProps,
										 D3D12_HEAP_FLAG_NONE,
										 &resourceDesc,
			                             initialState,
										 optimizedClearValue,
										 IID_PPV_ARGS(ppResource));
		if (resourceName)
		{
			(*ppResource)->SetName(resourceName);
		}
	}

	inline UINT64 DxAlign(UINT64 value, UINT alignment)
	{
		return (value + (alignment - 1)) & ~(alignment - 1);
	}

	inline VOID DxCreateRootSignature(
		ID3D12Device* pDevice,
		ID3D12RootSignature** ppRootSignature,
		const std::vector<CD3DX12_ROOT_PARAMETER>& rootParameters,
		const std::vector< CD3DX12_STATIC_SAMPLER_DESC>& staticSampleDesc)
	{
		CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(
			rootParameters.size(), 
			rootParameters.data(),
			staticSampleDesc.size(),
			staticSampleDesc.data(),
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		ComPtr<ID3DBlob> error;
		ComPtr<ID3DBlob> signature;

		D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
		if (error != nullptr)
		{
			const char* errorMsg =
				reinterpret_cast<const char*>(error->GetBufferPointer());
			PrintUtils::PrintString(errorMsg);
			assert(1);
		}

		pDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(ppRootSignature));
	}
}
