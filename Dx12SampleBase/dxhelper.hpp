#pragma once

#include <d3d12.h>
#include <d3dx12.h>

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

	inline ComPtr<ID3D12RootSignature> GetSrvDescTableRootSignature(ID3D12Device* pDevice, UINT numSRVs)
	{
		ComPtr<ID3D12RootSignature> rootSignature;

		ComPtr<ID3DBlob> rootSigBlob;
		ComPtr<ID3DBlob> errorBlob;

		auto descriptorTable = CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

		CD3DX12_ROOT_PARAMETER rootParameters[1];
		rootParameters[0].InitAsDescriptorTable(1,
			&descriptorTable,
			D3D12_SHADER_VISIBILITY_PIXEL);

		CD3DX12_STATIC_SAMPLER_DESC staticSampler(0, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR);
		auto rootSignatureDesc = CD3DX12_ROOT_SIGNATURE_DESC(1, rootParameters, 1, &staticSampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &rootSigBlob, &errorBlob);

		if (errorBlob != NULL)
		{
			OutputDebugStringA((char*)errorBlob->GetBufferPointer());
		}

		pDevice->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
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

	inline void AllocateUAVBuffers(ID3D12Device* pDevice, UINT64 bufferSize, ID3D12Resource **ppResource, const wchar_t* resourceName = nullptr, D3D12_RESOURCE_STATES initialResourceState = D3D12_RESOURCE_STATE_COMMON)
	{
		auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
		pDevice->CreateCommittedResource(&uploadHeapProperties,
			                             D3D12_HEAP_FLAG_NONE,
			                             &bufferDesc,
			                             initialResourceState,
			                             nullptr,
									     IID_PPV_ARGS(ppResource));

		if (resourceName)
		{
			(*ppResource)->SetName(resourceName);
		}
	}

	inline void AllocateBufferResource(ID3D12Device* pDevice, UINT64 bufferSizeInBytes, ID3D12Resource** ppResource, BOOL isUploadHeap, const wchar_t* resourceName = nullptr, D3D12_RESOURCE_STATES initialResourceState = D3D12_RESOURCE_STATE_COMMON)
	{
		D3D12_HEAP_TYPE heapType = (isUploadHeap == FALSE) ? D3D12_HEAP_TYPE_DEFAULT : D3D12_HEAP_TYPE_GPU_UPLOAD;

		auto heapProps = CD3DX12_HEAP_PROPERTIES(heapType);
		auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSizeInBytes);

		pDevice->CreateCommittedResource(&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			IID_PPV_ARGS(ppResource));
		if (resourceName)
		{
			(*ppResource)->SetName(resourceName);
		}
	}
}
