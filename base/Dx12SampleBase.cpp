#include "Dx12SampleBase.h"
#include <d3dx12.h>
#include "tiny_gltf.h"


Dx12SampleBase::Dx12SampleBase(UINT width, UINT height) :
	m_width(width),
	m_height(height),
	m_simpleComposition({}),
	m_fenceValue(0),
	m_fenceEvent(0)
{
	m_aspectRatio = (static_cast<FLOAT>(width)) / height;
	m_assetReader = FileReader();
}


UINT Dx12SampleBase::GetSwapChainBufferCount()
{
	return 2;
}

DXGI_FORMAT Dx12SampleBase::GetBackBufferFormat()
{
	return DXGI_FORMAT_R8G8B8A8_UNORM;

}

HRESULT Dx12SampleBase::CreateDevice()
{
	HRESULT result = S_OK;
	ComPtr<ID3D12Debug> debugInterface;

	UINT dxgiFactoryFlags = 0;
	// Enable the debug layer (requires the Graphics Tools "optional feature").
	// NOTE: Enabling the debug layer after device creation will invalidate the active device.
	
	///@todo use an option and error handling with imgui

	result = D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface));
	if (result == S_OK)
	{
		debugInterface->EnableDebugLayer();
		dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
	}
	
	
	//Create dxgi factory to enumerate adapters, create device, swapchain etc.
	result = CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&m_dxgiFactory7));

	
	///@todo lets just enumerate the most performant GPU for now. need imgui for selection later.
	ComPtr<IDXGIAdapter1> dxgiAdapter1;
	DXGI_GPU_PREFERENCE gpuPreference = DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE;

	result = m_dxgiFactory7->EnumAdapterByGpuPreference(0, gpuPreference, IID_PPV_ARGS(&dxgiAdapter1));
	
	D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_12_1;
	
	result = D3D12CreateDevice(
				dxgiAdapter1.Get(),
				featureLevel,
				IID_PPV_ARGS(&m_pDevice)
	);

	return result;
}

HRESULT Dx12SampleBase::CreateCommandQueues()
{
	HRESULT result = S_OK;

	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
	cmdQueueDesc.Type  = D3D12_COMMAND_LIST_TYPE_DIRECT;
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

	result = m_pDevice->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&m_pCmdQueue));

	return result;
}

HRESULT Dx12SampleBase::WaitForFenceCompletion(ID3D12CommandQueue* pCmdQueue)
{
	HRESULT             result = S_OK;

	m_fenceValue++;
	m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	UINT64 fenceValue = m_fenceValue;
	result = pCmdQueue->Signal(m_fence.Get(), fenceValue);

	UINT64 completedValue = m_fence->GetCompletedValue();

	if (completedValue < fenceValue)
	{
		result = m_fence->SetEventOnCompletion(fenceValue, m_fenceEvent);
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}

	return result;
}


HRESULT Dx12SampleBase::CreateSwapChain(HWND hwnd)
{
	HRESULT    result               = S_OK;
	const UINT swapChainBufferCount = GetSwapChainBufferCount();

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount           = swapChainBufferCount;
	swapChainDesc.Width                 = m_width;
	swapChainDesc.Height                = m_height;
	swapChainDesc.Format                = GetBackBufferFormat();
	swapChainDesc.SwapEffect            = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count      = 1;
	swapChainDesc.SampleDesc.Quality    = 0;

	///@note Samples  -> Render to offline RTV
	///      Copy RTV -> Swapchain
	///      State Trasition: Use Resource Barriers
	swapChainDesc.BufferUsage          = DXGI_USAGE_RENDER_TARGET_OUTPUT;

	ComPtr<IDXGISwapChain1> swapChain;

	result = m_dxgiFactory7->CreateSwapChainForHwnd(
					m_pCmdQueue.Get(),                         ///< In Dx12, Present -> needs cmdQueue. In Dx11, we pass in pDevice.
					hwnd,
					&swapChainDesc,
					nullptr,                                  ///< Full Screen Desc
					nullptr,                                  ///< pRestrict to Output
					&swapChain);

	result = m_dxgiFactory7->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);
	result = swapChain.As(&m_swapChain4);

	m_swapChainBuffers.resize(swapChainBufferCount);

	for (UINT i = 0; i < swapChainBufferCount; i++)
	{
		result = m_swapChain4->GetBuffer(i, IID_PPV_ARGS(&m_swapChainBuffers[i]));
	}

	return result;
}

HRESULT Dx12SampleBase::CreateFence()
{
	HRESULT result = S_OK;

	ID3D12Device* pDevice = GetDevice();

	//Create fence and wait till GPU copy is complete
	m_fenceValue = 0;
	pDevice->CreateFence(m_fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence));

	return S_OK;
}

HRESULT Dx12SampleBase::CreateRenderTargetDescriptorHeap(UINT numDescriptors)
{
	HRESULT result                          = S_OK;

	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	descHeapDesc.NumDescriptors             = numDescriptors;
	descHeapDesc.Type                       = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	descHeapDesc.Flags                      = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	result = m_pDevice->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&m_rtvDescHeap));

	return result;
}

HRESULT Dx12SampleBase::CreateShaderResourceViewDescriptorHeap(UINT numDescriptors)
{
	HRESULT result                          = S_OK;
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	descHeapDesc.NumDescriptors             = numDescriptors;
	descHeapDesc.Type                       = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	descHeapDesc.Flags                      = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	result = m_pDevice->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&m_srvDescHeap));
	return result;
}

HRESULT Dx12SampleBase::CreateRenderTargetViews(UINT numRTVs, BOOL isInternal)
{
	HRESULT    result = S_OK;
	const UINT start  = (isInternal == TRUE) ? 0 : GetSwapChainBufferCount();

	///@todo repeats code A
	UINT    rtvHeapSize = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvDescHeap.Get()->GetCPUDescriptorHandleForHeapStart());
	rtvHandle.Offset(start, rtvHeapSize);

	for (UINT i = start; i < start + numRTVs; i++)
	{
		ID3D12Resource* rtvResource = nullptr;
		if (isInternal)
		{
			rtvResource = m_swapChainBuffers[i].Get();
		}
		else
		{
			///@note RTV heap layout is swapchain RT vews first, then app specific RTVs
			rtvResource = m_rtvResources[i - start].Get();
		}

		m_pDevice->CreateRenderTargetView(rtvResource, nullptr, rtvHandle);
		rtvHandle.Offset(1, rtvHeapSize);
	}

	return result;
}

D3D12_CPU_DESCRIPTOR_HANDLE Dx12SampleBase::GetRenderTargetView(UINT rtvIndex, BOOL isInternal)
{
	const UINT rtvOffset = (isInternal == TRUE) ? 0 : GetSwapChainBufferCount();
	///@todo repete code A
	UINT rtvHeapSize = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvDescHeap.Get()->GetCPUDescriptorHandleForHeapStart());
	rtvHandle.Offset(rtvIndex + rtvOffset, rtvHeapSize);
	
	return rtvHandle;
}

HRESULT Dx12SampleBase::CreateCommandList()
{
	HRESULT result = S_OK;

	result = m_pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_pCommandAllocator));
	result = m_pDevice->CreateCommandList(0,
										D3D12_COMMAND_LIST_TYPE_DIRECT,
										m_pCommandAllocator.Get(),
										nullptr, 
										IID_PPV_ARGS(&m_pCmdList));
	m_pCmdList->Close();
	return result;
}

/*
* Creates render target resources and their shader resource views.
* Using this function, the app can create offscreen render targets to render into.
* SRVs is needed to compose the final image on the swapchain backbuffer.
*/
HRESULT Dx12SampleBase::CreateRenderTargetResourceAndSRVs(UINT numResources)
{
	HRESULT result = S_OK;

	m_rtvResources.clear();
	m_rtvResources.resize(numResources);

	///@note CD3DX12_RESOURCE_DESC::Tex2D(GetBackBufferFormat(), m_width, m_height);

	D3D12_RESOURCE_DESC resourceDesc = {};
	resourceDesc.Dimension           = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesc.Format	             = GetBackBufferFormat();
	resourceDesc.MipLevels           = 1;
	resourceDesc.Alignment           = 0;
	resourceDesc.DepthOrArraySize    = 1;
	resourceDesc.Width               = m_width;
	resourceDesc.Height              = m_height;
	resourceDesc.Flags               = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	resourceDesc.Layout              = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resourceDesc.SampleDesc.Count    = 1;

	auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	const UINT srvDescHeapSize = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(m_srvDescHeap->GetCPUDescriptorHandleForHeapStart());

	for (UINT i=0; i<numResources; i++)
	{
		m_pDevice->CreateCommittedResource(&heapProperties,
			                               D3D12_HEAP_FLAG_NONE,
										   &resourceDesc,
										   D3D12_RESOURCE_STATE_RENDER_TARGET,
										   nullptr,
										   IID_PPV_ARGS(&m_rtvResources[i]));

		m_pDevice->CreateShaderResourceView(m_rtvResources[i].Get(),
										 nullptr,
			                             srvHandle);
		srvHandle.Offset(1, srvDescHeapSize);
	}

	return result;
}

VOID Dx12SampleBase::GetInputLayoutDesc_Layout1(D3D12_INPUT_LAYOUT_DESC& layout1)
{
	// Define the vertex input layout.
	static const D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};

	layout1.NumElements = _countof(inputElementDescs);
	layout1.pInputElementDescs = inputElementDescs;
}

ComPtr<ID3D12PipelineState> Dx12SampleBase::GetGfxPipelineStateWithShaders(const LPCWSTR vertexShaderName,
																	       const LPCWSTR pixelShaderName,
	                                                                       ID3D12RootSignature* signature,
																		   const D3D12_INPUT_LAYOUT_DESC& iaLayout)
{
	ComPtr<ID3D12PipelineState> gfxPipelineState;
	ComPtr<ID3DBlob>            vertexShader;
	ComPtr<ID3DBlob>            pixelShader;
	
	///@todo compileFlags = 0 for release
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;

	std::wstring compiledVertexShaderPath = m_assetReader.GetFullAssetFilePath(vertexShaderName);
	std::wstring compiledPixelShaderPath  = m_assetReader.GetFullAssetFilePath(pixelShaderName);

	vertexShader = m_assetReader.LoadShaderBlobFromAssets(compiledVertexShaderPath);
	pixelShader  = m_assetReader.LoadShaderBlobFromAssets(compiledPixelShaderPath);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineStateDesc = {};
	pipelineStateDesc.InputLayout                        = iaLayout;
	pipelineStateDesc.pRootSignature                     = signature;
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
	
	m_pDevice->CreateGraphicsPipelineState(&pipelineStateDesc, IID_PPV_ARGS(&gfxPipelineState));

	return gfxPipelineState;
}

HRESULT Dx12SampleBase::InitializeFrameComposition()
{
	HRESULT result = S_OK;

	///@note initialize root signarure
	{
		ComPtr<ID3DBlob> rootSigBlob;
		ComPtr<ID3DBlob> errorBlob;

		auto descriptorTable = CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

		CD3DX12_ROOT_PARAMETER rootParameters[1];
		rootParameters[0].InitAsDescriptorTable(1,
												&descriptorTable,
												D3D12_SHADER_VISIBILITY_PIXEL);

		CD3DX12_STATIC_SAMPLER_DESC staticSampler(0, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR);
		auto rootSignature = CD3DX12_ROOT_SIGNATURE_DESC(1, rootParameters, 1, &staticSampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		D3D12SerializeRootSignature(&rootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &rootSigBlob, &errorBlob);

		if (errorBlob != NULL)
		{
			OutputDebugStringA((char*)errorBlob->GetBufferPointer());
		}

		m_pDevice->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), IID_PPV_ARGS(&m_simpleComposition.rootSignature));
	}

	D3D12_INPUT_LAYOUT_DESC layout1 = {};
	GetInputLayoutDesc_Layout1(layout1);
	m_simpleComposition.pipelineState = GetGfxPipelineStateWithShaders(L"FrameSimpleVS.cso",
		                                                               L"FrameSimplePS.cso",
		                                                               m_simpleComposition.rootSignature.Get(),
		                                                               layout1);

	///@note instead of drawing two triangles to form a quad, we draw a triangle which covers the full screen.
	///      This helps in avoiding diagonal interpolation issues.
	/// 
	///      Normalized coordinates for Dx12 (x,y):
	///      (-1, -1)	(+1, -1)
	///      (-1, +1)   (+1, +1)
	///      Simple Vertex is position[3], color[3], texcoord[2]
	SimpleVertex fullScreenTri[] =
	{
		{-1.0f, -1.0f, +1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f},
		{-1.0f, +3.0f, +1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 2.0f},
		{+3.0f, -1.0f, +1.0f, 1.0f, 1.0f, 1.0f, 2.0f, 0.0f}
	};
	UINT dataSizeInBytes = sizeof(fullScreenTri);

	m_simpleComposition.vertexBuffer = CreateBufferWithData(fullScreenTri, dataSizeInBytes);

	m_simpleComposition.vertexBufferView.BufferLocation = m_simpleComposition.vertexBuffer->GetGPUVirtualAddress();
	m_simpleComposition.vertexBufferView.SizeInBytes    = dataSizeInBytes;
	m_simpleComposition.vertexBufferView.StrideInBytes  = sizeof(SimpleVertex);

	result = (m_simpleComposition.vertexBuffer != NULL) ? S_OK : E_FAIL;

	return result;
}

ComPtr<ID3D12Resource> Dx12SampleBase::CreateBufferWithData(void* cpuData, UINT sizeInBytes)
{
	ID3D12GraphicsCommandList*  pCmdList  = m_pCmdList.Get();
	ID3D12CommandQueue*         pCmdQueue = m_pCmdQueue.Get();

	ComPtr<ID3D12Resource> bufferResource;

	auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeInBytes);

	m_pDevice->CreateCommittedResource(&heapProps,
										D3D12_HEAP_FLAG_NONE,
										&resourceDesc,
										D3D12_RESOURCE_STATE_COMMON,
										nullptr,
										IID_PPV_ARGS(&bufferResource));

	pCmdList->Reset(m_pCommandAllocator.Get(), nullptr);

	UploadCpuDataAndWaitForCompletion(cpuData,
							   sizeInBytes,
							   pCmdList,
							   pCmdQueue,
							   bufferResource.Get(),
							   D3D12_RESOURCE_STATE_COMMON,
							   D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

	return bufferResource;

}

HRESULT Dx12SampleBase::UploadCpuDataAndWaitForCompletion(void*                      cpuData,
	                                                      UINT                       dataSizeInBytes,
	                                                      ID3D12GraphicsCommandList* pCmdList,
	                                                      ID3D12CommandQueue*        pCmdQueue,
														  ID3D12Resource*            pDstRes,
														  D3D12_RESOURCE_STATES      dstStateBefore,
														  D3D12_RESOURCE_STATES      dstStateAfter)
{
	HRESULT result = S_OK;

	ComPtr<ID3D12Resource> stagingResource;

	//Create an staging resource for upload
	{
		CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_UPLOAD);
		auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(dataSizeInBytes);

		result = m_pDevice->CreateCommittedResource(&heapProperties,
													 D3D12_HEAP_FLAG_NONE,
													 &resourceDesc,
													 D3D12_RESOURCE_STATE_COPY_SOURCE,
													 nullptr,
													 IID_PPV_ARGS(&stagingResource));
	}

	VOID* pMappedPtr;
	CD3DX12_RANGE readRange(0, 0);

	//@note specifying nullptr as read range indicates CPU can read entire resource
	stagingResource->Map(0, &readRange, &pMappedPtr);
	memcpy(pMappedPtr, cpuData, dataSizeInBytes);
	stagingResource->Unmap(0, nullptr);

	if (dstStateBefore != D3D12_RESOURCE_STATE_COPY_DEST)
	{
		auto toCopyDst = CD3DX12_RESOURCE_BARRIER::Transition(pDstRes,
															  dstStateBefore,
															  D3D12_RESOURCE_STATE_COPY_DEST);
		pCmdList->ResourceBarrier(1, &toCopyDst);
	}

	pCmdList->CopyBufferRegion(pDstRes,
							   0,
		                       stagingResource.Get(),
		                       0,
		                       dataSizeInBytes);

	if (dstStateAfter != D3D12_RESOURCE_STATE_COPY_DEST)
	{
		auto toStateAfter = CD3DX12_RESOURCE_BARRIER::Transition(pDstRes, D3D12_RESOURCE_STATE_COPY_DEST, dstStateAfter);
		pCmdList->ResourceBarrier(1, &toStateAfter);
	}

	pCmdList->Close();

	ID3D12CommandList* ppCommandLists[] = { pCmdList };
	pCmdQueue->ExecuteCommandLists(1, ppCommandLists);

	WaitForFenceCompletion(pCmdQueue);

	return result;
}

HRESULT Dx12SampleBase::Run()
{
	HRESULT result = S_OK;
	
	result = m_pCmdList->Reset(m_pCommandAllocator.Get(), nullptr);

	CD3DX12_VIEWPORT viewPort(0.0f, 0.0f, FLOAT(m_width), FLOAT(m_height));
	CD3DX12_RECT     rect(0, 0, LONG(m_width), LONG(m_height));

	m_pCmdList->RSSetViewports(1, &viewPort);
	m_pCmdList->RSSetScissorRects(1, &rect);

	result = RenderFrame();

	return result;
}

HRESULT Dx12SampleBase::RenderRtvContentsOnScreen(UINT rtvResourceIndex)
{
	HRESULT result      = S_OK;
	FLOAT clearColor[4] = { 0.4f, 0.4f, 0.8f, 1.0f };

	const UINT      currentFrameIndex  = m_swapChain4->GetCurrentBackBufferIndex();
	ID3D12Resource* pCurrentBackBuffer = m_swapChainBuffers[currentFrameIndex].Get();
	ID3D12Resource* pRtvResource       = m_rtvResources[rtvResourceIndex].Get();

	D3D12_RESOURCE_BARRIER preResourceBarriers[2];

	preResourceBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(pCurrentBackBuffer,
															 D3D12_RESOURCE_STATE_PRESENT,
															 D3D12_RESOURCE_STATE_RENDER_TARGET);
	preResourceBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(pRtvResource,
		                                                       D3D12_RESOURCE_STATE_RENDER_TARGET,
		                                                       D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
 

	m_pCmdList->ResourceBarrier(2, preResourceBarriers);

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = GetRenderTargetView(currentFrameIndex, TRUE);
	m_pCmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
	m_pCmdList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	m_pCmdList->SetPipelineState(m_simpleComposition.pipelineState.Get());
	m_pCmdList->SetGraphicsRootSignature(m_simpleComposition.rootSignature.Get());
	m_pCmdList->SetDescriptorHeaps(1, m_srvDescHeap.GetAddressOf());
	m_pCmdList->SetGraphicsRootDescriptorTable(0,
		CD3DX12_GPU_DESCRIPTOR_HANDLE(m_srvDescHeap->GetGPUDescriptorHandleForHeapStart(),
			rtvResourceIndex,
			m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)));
	m_pCmdList->IASetVertexBuffers(0, 1, &m_simpleComposition.vertexBufferView);
	m_pCmdList->DrawInstanced(3, 1, 0, 0);

	D3D12_RESOURCE_BARRIER postResourceBarriers[2];

	postResourceBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(pCurrentBackBuffer,
															 D3D12_RESOURCE_STATE_RENDER_TARGET,
															 D3D12_RESOURCE_STATE_PRESENT);
	postResourceBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(pRtvResource,
		                                                       D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
															   D3D12_RESOURCE_STATE_RENDER_TARGET);
	m_pCmdList->ResourceBarrier(2, postResourceBarriers);
	m_pCmdList->Close();
	
	ID3D12CommandList* pCmdLists[] = { m_pCmdList.Get()};
	m_pCmdQueue->ExecuteCommandLists(_countof(pCmdLists), pCmdLists);


	m_swapChain4->Present(1, 0);

	WaitForFenceCompletion(m_pCmdQueue.Get());

	return result;

}


/*
* Ordering is important. Most samples put all the code in one function.
* 
* Concepts:
* DxgiFactory represents OS interface to all graphics functionality
* Using factory, enumerate all adapters and choose a DxgiAdapter e.g. WARP, Intel, Nvidia.
* From dxgiAdapter, create a D3D12Device for a specific feature level.
* Use the Device and Create CommandQueue.
* Use Command Queue and Create swapchain and make the window association
*/
HRESULT Dx12SampleBase::OnInit(HWND hwnd)
{
	HRESULT result = S_OK;

	result = CreateDevice();
	result = CreateCommandQueues();
	result = CreateCommandList();
	result = CreateSwapChain(hwnd);
	result = CreateFence();
	result = InitializeFrameComposition();

	///@todo refactor into a function
	{
		const UINT numRTVsForComposition = GetSwapChainBufferCount();
		const UINT numRTVsForApp         = NumRTVsNeededForApp();

		CreateRenderTargetDescriptorHeap(numRTVsForComposition + numRTVsForApp);
		CreateShaderResourceViewDescriptorHeap(numRTVsForApp);
		CreateRenderTargetViews(numRTVsForComposition, TRUE);
	}

	return result;
}


XMMATRIX Dx12SampleBase::GetMVPMatrix(XMMATRIX& modelMatrix)
{
	FLOAT aspectRatio = ((FLOAT)m_width) / m_height;
	///@note FOV, width/height, near and far clipping plane.
	XMMATRIX projection = XMMatrixPerspectiveFovLH(XMConvertToRadians(45.0f),
												   aspectRatio,
												   0.1f,
												   100.0f);

	///@note camera position, camera forward vector, up direction
	XMMATRIX view = XMMatrixLookAtLH(XMVectorSet(0.0f, 2.0f, -5.0f, 1.0f),
									 XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f),
									 XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));

	XMMATRIX finalMvp = XMMatrixTranspose(modelMatrix * view * projection);

	return finalMvp;
}

DXGI_FORMAT Dx12SampleBase::GltfGetDxgiFormat(int tinyGltfComponentType, int components)
{
	DXGI_FORMAT dxgiFormat = DXGI_FORMAT_UNKNOWN;

	switch (tinyGltfComponentType)
	{
	case TINYGLTF_COMPONENT_TYPE_FLOAT:
		dxgiFormat = GetDxgiFloatFormat(components);
		break;

	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
		dxgiFormat = GetDxgiUnsignedShortFormat(components);
		break;
	default:
		break;
	}

	return dxgiFormat;
}

/*
#define TINYGLTF_TYPE_VEC2 (2)
#define TINYGLTF_TYPE_VEC3 (3)
#define TINYGLTF_TYPE_VEC4 (4)
#define TINYGLTF_TYPE_MAT2 (32 + 2)
#define TINYGLTF_TYPE_MAT3 (32 + 3)
#define TINYGLTF_TYPE_MAT4 (32 + 4)
#define TINYGLTF_TYPE_SCALAR (64 + 1)
#define TINYGLTF_TYPE_VECTOR (64 + 4)
#define TINYGLTF_TYPE_MATRIX (64 + 16) 
*/
DXGI_FORMAT Dx12SampleBase::GetDxgiFloatFormat(int numComponents)
{
	DXGI_FORMAT dxgiFormat = DXGI_FORMAT_UNKNOWN;

	switch (numComponents)
	{
	case TINYGLTF_TYPE_SCALAR:
		dxgiFormat = DXGI_FORMAT_R32_FLOAT;
		break;
	case TINYGLTF_TYPE_VEC2:
		dxgiFormat = DXGI_FORMAT_R32G32_FLOAT;
		break;
	case TINYGLTF_TYPE_VEC3:
		dxgiFormat = DXGI_FORMAT_R32G32B32_FLOAT;
		break;
	case TINYGLTF_TYPE_VEC4:
		dxgiFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
		break;
	default:
		break;
	}

	return dxgiFormat;
}

DXGI_FORMAT Dx12SampleBase::GetDxgiUnsignedShortFormat(int numComponents)
{
	DXGI_FORMAT dxgiFormat = DXGI_FORMAT_UNKNOWN;

	switch (numComponents)
	{
	case TINYGLTF_TYPE_SCALAR:
		dxgiFormat = DXGI_FORMAT_R16_UINT;
		break;
	case TINYGLTF_TYPE_VEC2:
		dxgiFormat = DXGI_FORMAT_R16G16_UINT;
		break;
	default:
		break;
	}

	return dxgiFormat;
}


