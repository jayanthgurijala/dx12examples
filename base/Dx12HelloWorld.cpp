#include "Dx12HelloWorld.h"
#include <d3dx12.h>
#include <iostream>



Dx12HelloWorld::Dx12HelloWorld(UINT width, UINT height) :
	m_vertexBufferSizeInBytes(0),
	m_vertexStrideInBytes(0),
	Dx12SampleBase(width, height)
{
	m_fileReader = FileReader();
}

HRESULT Dx12HelloWorld::PreRun()
{
	HRESULT result = S_OK;

	result = CreatePipelineState();
	result = CreateAndLoadVertexBuffer();
	result = CreateAppResources();
	return result;
}


HRESULT Dx12HelloWorld::PostRun()
{
	return S_OK;
}

HRESULT Dx12HelloWorld::CreatePipelineState()
{
	HRESULT       result  = S_OK;
	ID3D12Device* pDevice = GetDevice();

	ComPtr<ID3DBlob>            vertexShader;
	ComPtr<ID3DBlob>            pixelShader;

	//Create Root Signature
	{
		D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
		rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
		rootSignatureDesc.pParameters = nullptr;
		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;

		///@todo Error handling
		D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);

		pDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignarure));
	}


	//Create Shaders
	{
		///@todo compileFlags = 0 for release
		UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;

		std::wstring compiledVertexShaderPath = m_fileReader.GetFullAssetFilePath(L"VertexShader.cso");
		std::wstring compiledPixelShaderPath  = m_fileReader.GetFullAssetFilePath(L"PixelShader.cso");

		vertexShader = m_fileReader.LoadShaderBlobFromAssets(compiledVertexShaderPath);
		pixelShader = m_fileReader.LoadShaderBlobFromAssets(compiledPixelShaderPath);
	}


	D3D12_INPUT_ELEMENT_DESC inputElementDesc[1];
	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = {};
	{
		
		inputElementDesc[0].SemanticName         = "POSITION";
		inputElementDesc[0].SemanticIndex        = 0;
		inputElementDesc[0].Format               = DXGI_FORMAT_R32G32B32_FLOAT; //x,y,z
		inputElementDesc[0].InputSlot            = 0;
		inputElementDesc[0].AlignedByteOffset    = 0;
		inputElementDesc[0].InputSlotClass       = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		inputElementDesc[0].InstanceDataStepRate = 0;

		inputLayoutDesc.NumElements             = 1;
		inputLayoutDesc.pInputElementDescs      = inputElementDesc;
	}

	//Create pipeline state
	/*
	* 1. Input Layout
	* 2. Root Signature
	* 3. Shaders
	* 4. Rasterizer State
	* 5. Blend State
	* 6. DepthStencil State
	* 7. Sample Mask
	* 8. Primitive Topology
	* 9. RTVs and DSVs
	* 10. MSAA
	* 11. Flags, Cached PSOs
	*/
	{
		D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineStateDesc = {};
		pipelineStateDesc.InputLayout                        = inputLayoutDesc;
		pipelineStateDesc.pRootSignature                     = m_rootSignarure.Get();
		pipelineStateDesc.VS                                 = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
		pipelineStateDesc.PS                                 = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
		pipelineStateDesc.RasterizerState                    = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		pipelineStateDesc.BlendState                         = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		pipelineStateDesc.DepthStencilState                  = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

		///@todo write some test cases for this
		pipelineStateDesc.SampleMask                         = UINT_MAX;
		pipelineStateDesc.PrimitiveTopologyType              = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		pipelineStateDesc.NumRenderTargets                   = 1;
		pipelineStateDesc.RTVFormats[0]						 = GetBackBufferFormat();

		///@todo experiment with flags
		pipelineStateDesc.Flags                              = D3D12_PIPELINE_STATE_FLAG_NONE;
		
		pipelineStateDesc.SampleDesc.Count                   = 1;

		pDevice->CreateGraphicsPipelineState(&pipelineStateDesc, IID_PPV_ARGS(&m_pipelineState));
	}

	return result;
}

HRESULT Dx12HelloWorld::CreateAndLoadVertexBuffer()
{
	HRESULT result = S_OK;
	ComPtr<ID3D12Resource> stagingBuffer;
	D3D12_RESOURCE_STATES      vbState   = D3D12_RESOURCE_STATE_COPY_DEST;

	SimpleVertex triangleVertices[] =
	{
		{ { 0.0f, 0.25f * m_aspectRatio, 0.0f }, { 1.0f, 0.0f, 0.0f}, {0.0f,0.0f} },
		{ { 0.25f, -0.25f * m_aspectRatio, 0.0f }, { 0.0f, 1.0f, 0.0f}, {0.0f,0.0f} },
		{ { -0.25f, -0.25f * m_aspectRatio, 0.0f }, { 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f} }
	};

	const UINT vertexBufferSize = sizeof(triangleVertices);
	m_vertexBufferSizeInBytes   = vertexBufferSize;
	m_vertexStrideInBytes       = sizeof(SimpleVertex);

	m_vertexBuffer = CreateBufferWithData(triangleVertices, vertexBufferSize);

	return result;
}



HRESULT Dx12HelloWorld::CreateAppResources()
{
	HRESULT    result   = S_OK;
	const UINT numRTVs  = NumRTVsNeededForApp();
	result              = CreateRenderTargetResourceAndSRVs(numRTVs);
	result              = CreateRenderTargetViews(numRTVs, FALSE);

	return result;
}

HRESULT Dx12HelloWorld::RenderFrame()
{
	ID3D12GraphicsCommandList*  pCmdList      = GetCmdList();
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle     = GetRenderTargetView(0, FALSE);
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
	
	vertexBufferView.BufferLocation           = m_vertexBuffer.Get()->GetGPUVirtualAddress();
	vertexBufferView.SizeInBytes              = m_vertexBufferSizeInBytes;
	vertexBufferView.StrideInBytes            = m_vertexStrideInBytes;
	
	FLOAT clearColor[4] = { 0.02f, 0.02f, 0.05f, 1.0f};
	
	pCmdList->OMSetRenderTargets(1,
		                         &rtvHandle,              //D3D12_CPU_DESCRIPTOR_HANDLE
		                         FALSE,                   //RTsSingleHandleToDescriptorRange
		                         nullptr);                //D3D12_CPU_DESCRIPTOR_HANDLE
	
	pCmdList->ClearRenderTargetView(rtvHandle,
		                            clearColor,
		                            0,
		                            nullptr);
	
	pCmdList->SetGraphicsRootSignature(m_rootSignarure.Get());
	pCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pCmdList->SetPipelineState(m_pipelineState.Get());
	pCmdList->IASetVertexBuffers(0, 1, &vertexBufferView);
	pCmdList->DrawInstanced(3, 1, 0, 0);

	RenderRtvContentsOnScreen(0);

	return S_OK;
}
