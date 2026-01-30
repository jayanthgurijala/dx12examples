#include "pch.h"
#include "Dx12SampleBase.h"
#include "tiny_gltf.h"
#include "DxPrintUtils.h"
#include <memory>
#include <chrono>


Dx12SampleBase::Dx12SampleBase(UINT width, UINT height) :
	m_width(width),
	m_height(height),
	m_simpleComposition({}),
	m_fenceValue(0),
	m_fenceEvent(0),
	m_frameDeltaTime(0),
	m_dsvDescHeap(nullptr),
	m_srvDescHeap(nullptr),
	m_rtvDescHeap(nullptr),
	m_srvUavCbvDescriptorSize(0),
	m_rtvDescriptorSize(0),
	m_dsvDescriptorSize(0),
	m_samplerDescriptorSize(0),
	m_camera(std::make_unique<DxCamera>(width, height)),
	m_assetReader(std::make_unique<FileReader>())
{

}

LRESULT CALLBACK Dx12SampleBase::WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE)
			PostQuitMessage(0);
		break;

	case WM_DESTROY:
		//Post WM_QUIT with the exit code
		PostQuitMessage(0);
		break;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}

int Dx12SampleBase::RunApp(HINSTANCE hInstance, int nCmdShow)
{
	SetupWindow(hInstance, nCmdShow);
	OnInit();
	PreRun();
	int retval = RenderLoop();
	PostRun();

	return retval;
}

int Dx12SampleBase::RenderLoop()
{
	//Main message loop
	MSG msg = {};
	using clock = std::chrono::high_resolution_clock;
	using durationf32 = std::chrono::duration<float>;
	auto previousTime = clock::now();

	while (msg.message != WM_QUIT)
	{

		//process messages in the queue
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		auto currentTime = clock::now();

		// deltaTime in seconds
		durationf32 duration = (currentTime - previousTime);

		NextFrame(duration.count());
		previousTime = currentTime;
	}

	PostRun();

	// Return this part of the WM_QUIT message to Windows.
	return static_cast<char>(msg.wParam);
}

VOID Dx12SampleBase::SetupWindow(HINSTANCE hInstance, int nCmdShow)
{
	//@todo command line arguments

	///@todo understand this w.r.t WIC
	CoInitializeEx(nullptr, COINIT_MULTITHREADED);


	//Create window
	WNDCLASSEX windowClass = {};
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = WindowProc;
	windowClass.hInstance = hInstance;
	windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	windowClass.lpszClassName = L"dx12Examples";
	RegisterClassEx(&windowClass);

	//@todo hard coded size, get it with command line arguments and a default value
	RECT windowRect = { 0, 0, 1280, 720 };
	AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

	m_hwnd = CreateWindow(
		windowClass.lpszClassName,
		L"Dx12 Examples",						///@todo use app name
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		windowRect.right - windowRect.left,
		windowRect.bottom - windowRect.top,
		nullptr,
		nullptr,
		hInstance,
		nullptr 								///@todo need this in the callback to WindowProc
	);

	ShowWindow(m_hwnd, nCmdShow);
}


UINT Dx12SampleBase::GetSwapChainBufferCount()
{
	return 2;
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

	m_srvUavCbvDescriptorSize = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_samplerDescriptorSize = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
	m_dsvDescriptorSize = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	m_rtvDescriptorSize = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);


	return result;
}

HRESULT Dx12SampleBase::CreateCommandQueues()
{
	HRESULT result = S_OK;

	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
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


HRESULT Dx12SampleBase::CreateSwapChain()
{
	HRESULT    result = S_OK;
	const UINT swapChainBufferCount = GetSwapChainBufferCount();

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = swapChainBufferCount;
	swapChainDesc.Width = m_width;
	swapChainDesc.Height = m_height;
	swapChainDesc.Format = GetBackBufferFormat();
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;

	///@note Samples  -> Render to offline RTV
	///      Copy RTV -> Swapchain
	///      State Trasition: Use Resource Barriers
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

	ComPtr<IDXGISwapChain1> swapChain;

	result = m_dxgiFactory7->CreateSwapChainForHwnd(
		m_pCmdQueue.Get(),                         ///< In Dx12, Present -> needs cmdQueue. In Dx11, we pass in pDevice.
		m_hwnd,
		&swapChainDesc,
		nullptr,                                  ///< Full Screen Desc
		nullptr,                                  ///< pRestrict to Output
		&swapChain);

	result = m_dxgiFactory7->MakeWindowAssociation(m_hwnd, DXGI_MWA_NO_ALT_ENTER);
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
	HRESULT result = S_OK;

	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	descHeapDesc.NumDescriptors = numDescriptors;
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	result = m_pDevice->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&m_rtvDescHeap));

	return result;
}

HRESULT Dx12SampleBase::CreateShaderResourceViewDescriptorHeap(UINT numDescriptors)
{
	HRESULT result = S_OK;
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	descHeapDesc.NumDescriptors = numDescriptors;
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	result = m_pDevice->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&m_srvDescHeap));
	return result;
}

HRESULT Dx12SampleBase::CreateDepthStencilViewDescriptorHeap(UINT numDescriptors)
{
	HRESULT result = S_OK;

	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	descHeapDesc.NumDescriptors = numDescriptors;
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	result = m_pDevice->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&m_dsvDescHeap));

	return result;
}

HRESULT Dx12SampleBase::CreateRenderTargetViews(UINT numRTVs, BOOL isInternal)
{
	HRESULT    result = S_OK;
	const UINT start = (isInternal == TRUE) ? 0 : GetSwapChainBufferCount();
	UINT	   rtvOffset = 0;

	for (UINT i = start; i < start + numRTVs; i++)
	{
		auto rtvHandle = GetRtvCpuHeapHandle(start + rtvOffset);
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
		rtvOffset++;

	}

	return result;
}

D3D12_CPU_DESCRIPTOR_HANDLE Dx12SampleBase::GetRenderTargetView(UINT rtvIndex, BOOL isInternal)
{
	const UINT rtvOffset = (isInternal == TRUE) ? 0 : GetSwapChainBufferCount();
	auto rtvHandle = GetRtvCpuHeapHandle(rtvIndex + rtvOffset);

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

HRESULT Dx12SampleBase::CreateDsvResources(UINT numResources, BOOL createViews)
{
	HRESULT result = S_OK;

	m_dsvResources.clear();
	m_dsvResources.resize(numResources);

	DXGI_FORMAT depthFormat = GetDepthStencilFormat();

	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = depthFormat;
	clearValue.DepthStencil.Depth = 1.0f;
	clearValue.DepthStencil.Stencil = 0;

	const auto dsvDesc = CD3DX12_RESOURCE_DESC::Tex2D(depthFormat, m_width, m_height, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
	const auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	for (UINT i = 0; i < numResources; i++)
	{
		m_pDevice->CreateCommittedResource(&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&dsvDesc,
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&clearValue,
			IID_PPV_ARGS(&m_dsvResources[i]));

		if (createViews == TRUE)
		{
			assert(m_dsvDescHeap != nullptr && m_dsvDescHeap->GetDesc().NumDescriptors == numResources);
			auto dsvHeapHandle = GetDsvCpuHeapHandle(i);
			m_pDevice->CreateDepthStencilView(m_dsvResources[i].Get(), nullptr, dsvHeapHandle);
		}
	}

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
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesc.Format = GetBackBufferFormat();
	resourceDesc.MipLevels = 1;
	resourceDesc.Alignment = 0;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.Width = m_width;
	resourceDesc.Height = m_height;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resourceDesc.SampleDesc.Count = 1;

	//@todo find a way to match these 	FLOAT clearColor[4] = { 0.7f, 0.7f, 1.0f, 1.0f};
	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = GetBackBufferFormat();
	clearValue.Color[0] = 0.7f;
	clearValue.Color[1] = 0.7f;
	clearValue.Color[2] = 1.0f;
	clearValue.Color[3] = 1.0f;

	auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	for (UINT i = 0; i < numResources; i++)
	{
		auto srvHandle = GetSrvCpuHeapHandle(i);
		m_pDevice->CreateCommittedResource(&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			&clearValue,
			IID_PPV_ARGS(&m_rtvResources[i]));

		m_pDevice->CreateShaderResourceView(m_rtvResources[i].Get(),
			nullptr,
			srvHandle);
	}

	return result;
}

/*
*
* SRV Heap Layout is
*
* 1. N SRV descriptors for RTV composition NumRTVsNeededForApp()
*		- These are created when app creates RTV resources CreateRenderTargetResourceAndSRVs
*
* 2. M descriptors requested by application NumSRVsNeededForApp()
*
*/
VOID Dx12SampleBase::CreateAppSrvAtIndex(UINT appSrvIndex, ID3D12Resource* srvResource)
{
	const UINT appSrvStartIndex = NumRTVsNeededForApp();	//see comment above
	auto srvHandle = GetSrvCpuHeapHandle(appSrvStartIndex + appSrvIndex);
	m_pDevice->CreateShaderResourceView(srvResource, nullptr, srvHandle);

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

ComPtr<ID3D12PipelineState> Dx12SampleBase::GetGfxPipelineStateWithShaders(const std::string& vertexShaderName,
	const std::string& pixelShaderName,
	ID3D12RootSignature* signature,
	const D3D12_INPUT_LAYOUT_DESC& iaLayout,
	BOOL enableWireFrame,
	BOOL doubleSided,
	BOOL useDepthStencil)
{
	ComPtr<ID3D12PipelineState> gfxPipelineState = nullptr;
	ComPtr<ID3DBlob>            vertexShader;
	ComPtr<ID3DBlob>            pixelShader;

	///@todo compileFlags = 0 for release
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;

	std::string compiledVertexShaderPath = m_assetReader->GetFullAssetFilePath(vertexShaderName);
	std::string compiledPixelShaderPath = m_assetReader->GetFullAssetFilePath(pixelShaderName);

	vertexShader = m_assetReader->LoadShaderBlobFromAssets(compiledVertexShaderPath);
	pixelShader = m_assetReader->LoadShaderBlobFromAssets(compiledPixelShaderPath);

	if (vertexShader == nullptr || pixelShader == nullptr)
	{
		PrintUtils::PrintString("Unable to load shaders");
	}
	else
	{

		CD3DX12_RASTERIZER_DESC rast(D3D12_DEFAULT);

		if (enableWireFrame == TRUE)
		{
			rast.CullMode = D3D12_CULL_MODE_NONE;
			rast.FillMode = D3D12_FILL_MODE_WIREFRAME;
		}

		if (doubleSided == TRUE)
		{
			rast.CullMode = D3D12_CULL_MODE_NONE;
		}


		CD3DX12_DEPTH_STENCIL_DESC depthStencilState(D3D12_DEFAULT);
		if (useDepthStencil == FALSE)
		{
			depthStencilState.DepthEnable = FALSE;
			depthStencilState.StencilEnable = FALSE;
		}


		D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineStateDesc = {};
		pipelineStateDesc.InputLayout = iaLayout;
		pipelineStateDesc.pRootSignature = signature;
		pipelineStateDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
		pipelineStateDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
		pipelineStateDesc.RasterizerState = rast;
		pipelineStateDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		pipelineStateDesc.DepthStencilState = depthStencilState;

		///@todo write some test cases for this
		pipelineStateDesc.SampleMask = UINT_MAX;
		pipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		pipelineStateDesc.NumRenderTargets = 1;
		pipelineStateDesc.RTVFormats[0] = GetBackBufferFormat();
		pipelineStateDesc.DSVFormat = (useDepthStencil == TRUE) ? GetDepthStencilFormat() : DXGI_FORMAT_UNKNOWN;

		///@todo experiment with flags
		pipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

		pipelineStateDesc.SampleDesc.Count = 1;

		m_pDevice->CreateGraphicsPipelineState(&pipelineStateDesc, IID_PPV_ARGS(&gfxPipelineState));
	}

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
	m_simpleComposition.pipelineState = GetGfxPipelineStateWithShaders("FrameSimple_VS.cso",
		"FrameSimple_PS.cso",
		m_simpleComposition.rootSignature.Get(),
		layout1);

	///@note instead of drawing two triangles to form a quad, we draw a triangle which covers the full screen.
	///      This helps in avoiding diagonal interpolation issues.
	/// 
	///      Normalized coordinates for Dx12 (x,y):
	///      (-1, -1) uv=(0,0)	(+1, -1) uv = (1,0)
	///      (-1, +1) uv=(0,1)   (+1, +1) uv = (1,1)
	///      Simple Vertex is position[3], color[3], texcoord[2]
	SimpleVertex fullScreenTri[] =
	{
		{-1.0f, -1.0f, +1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f},   // start only vertex visible at bottom left
		{-1.0f, +3.0f, +1.0f, 1.0f, 1.0f, 1.0f, 0.0f, -1.0f},  // clockwise to top(out of screen) left
		{+3.0f, -1.0f, +1.0f, 1.0f, 1.0f, 1.0f, 2.0f, 1.0f},   // clockwise to third vertex out of screen


	};
	UINT dataSizeInBytes = sizeof(fullScreenTri);

	m_simpleComposition.vertexBuffer = CreateBufferWithData(fullScreenTri, dataSizeInBytes);

	m_simpleComposition.vertexBufferView.BufferLocation = m_simpleComposition.vertexBuffer->GetGPUVirtualAddress();
	m_simpleComposition.vertexBufferView.SizeInBytes = dataSizeInBytes;
	m_simpleComposition.vertexBufferView.StrideInBytes = sizeof(SimpleVertex);

	result = (m_simpleComposition.vertexBuffer != NULL) ? S_OK : E_FAIL;

	return result;
}

ComPtr<ID3D12Resource> Dx12SampleBase::CreateBufferWithData(void* cpuData, UINT sizeInBytes, BOOL isUploadHeap)
{
	ID3D12GraphicsCommandList* pCmdList = m_pCmdList.Get();
	ID3D12CommandQueue* pCmdQueue = m_pCmdQueue.Get();

	ComPtr<ID3D12Resource> bufferResource;

	D3D12_HEAP_TYPE heapType = (isUploadHeap == FALSE) ? D3D12_HEAP_TYPE_DEFAULT : D3D12_HEAP_TYPE_GPU_UPLOAD;

	auto heapProps = CD3DX12_HEAP_PROPERTIES(heapType);
	auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeInBytes);

	m_pDevice->CreateCommittedResource(&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&bufferResource));

	if (cpuData != NULL && sizeInBytes > 0)
	{
		UploadCpuDataAndWaitForCompletion(cpuData,
			sizeInBytes,
			pCmdList,
			pCmdQueue,
			bufferResource.Get(),
			D3D12_RESOURCE_STATE_COMMON,
			D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	}
	return bufferResource;
}

ComPtr<ID3D12Resource> Dx12SampleBase::CreateTexture2DWithData(void* cpuData, SIZE_T sizeInBytes, UINT width, UINT height, DXGI_FORMAT format)
{
	ID3D12GraphicsCommandList* pCmdList = m_pCmdList.Get();
	ID3D12CommandQueue* pCmdQueue = m_pCmdQueue.Get();
	ComPtr<ID3D12Resource> texture2D;

	auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	auto resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(format, width, height, 1, 1);

	m_pDevice->CreateCommittedResource(&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&texture2D));

	auto queryDesc = texture2D.Get()->GetDesc();


	if (cpuData != NULL && sizeInBytes > 0)
	{
		UploadCpuDataAndWaitForCompletion(cpuData,
			sizeInBytes,
			pCmdList,
			pCmdQueue,
			texture2D.Get(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}

	return texture2D;
}

HRESULT Dx12SampleBase::UploadCpuDataAndWaitForCompletion(void* cpuData,
	UINT                       dataSizeInBytes,
	ID3D12GraphicsCommandList* pCmdList,
	ID3D12CommandQueue* pCmdQueue,
	ID3D12Resource* pDstRes,
	D3D12_RESOURCE_STATES      dstStateBefore,
	D3D12_RESOURCE_STATES      dstStateAfter)
{
	HRESULT result = S_OK;

	ComPtr<ID3D12Resource> stagingResource;

	D3D12_RESOURCE_DESC dstResDesc = pDstRes->GetDesc();

	assert(dstResDesc.MipLevels == 1);
	assert(dstResDesc.DepthOrArraySize = 1);

	//@todo test and add more
	assert(dstResDesc.Format == DXGI_FORMAT_UNKNOWN || DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);

	//@todo add more support
	assert(dstResDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER || D3D12_RESOURCE_DIMENSION_TEXTURE2D);



	///@note used for textures.
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
	UINT numRows;
	UINT64 rowSizeInBytes, totalSizeInBytes;
	UINT64 dstResWidth = dstResDesc.Width;
	UINT64 dstResHeight = dstResDesc.Height;
	DXGI_FORMAT dstResFormat = dstResDesc.Format;

	///@note Create an upload heap, for buffers it is straight forward, but for textures we need get the footprint
	{
		CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_UPLOAD);
		CD3DX12_RESOURCE_DESC resourceDesc;

		if (dstResDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
		{
			resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(dataSizeInBytes);
		}
		else if (dstResDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D)
		{
			m_pDevice->GetCopyableFootprints(&dstResDesc, 0, 1, 0, &footprint, &numRows, &rowSizeInBytes, &totalSizeInBytes);
			resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(totalSizeInBytes);
		}
		else
		{
			assert(0);
		}

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
	if (dstResDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
	{
		memcpy(pMappedPtr, cpuData, dataSizeInBytes);
	}
	else
	{
		for (UINT row = 0; row < numRows; row++)
		{
			BYTE* pMappedBytePtr = static_cast<BYTE*>(pMappedPtr);
			BYTE* pCpuBytePtr = static_cast<BYTE*>(cpuData);
			memcpy(pMappedBytePtr + footprint.Offset + row * footprint.Footprint.RowPitch, pCpuBytePtr + row * rowSizeInBytes, rowSizeInBytes);
		}
	}
	stagingResource->Unmap(0, nullptr);

	///@todo not really good to reset here, need to abstract?
	pCmdList->Reset(m_pCommandAllocator.Get(), nullptr);

	if (dstStateBefore != D3D12_RESOURCE_STATE_COPY_DEST)
	{
		auto toCopyDst = CD3DX12_RESOURCE_BARRIER::Transition(pDstRes,
			dstStateBefore,
			D3D12_RESOURCE_STATE_COPY_DEST);
		pCmdList->ResourceBarrier(1, &toCopyDst);
	}

	if (dstResDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
	{
		pCmdList->CopyBufferRegion(pDstRes,
			0,
			stagingResource.Get(),
			0,
			dataSizeInBytes);
	}
	else if (dstResDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D)
	{
		D3D12_TEXTURE_COPY_LOCATION dstCopyLoc;
		dstCopyLoc.pResource = pDstRes;
		dstCopyLoc.SubresourceIndex = 0;
		dstCopyLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;

		D3D12_TEXTURE_COPY_LOCATION srcCopyLoc;
		srcCopyLoc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		srcCopyLoc.pResource = stagingResource.Get();
		srcCopyLoc.PlacedFootprint = footprint;

		pCmdList->CopyTextureRegion(&dstCopyLoc, 0, 0, 0, &srcCopyLoc, nullptr);
	}
	else
	{
		assert(0);
	}

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

HRESULT Dx12SampleBase::NextFrame(FLOAT frameDeltaTime)
{
	HRESULT result = S_OK;

	m_frameDeltaTime = frameDeltaTime;
	m_camera->Update(frameDeltaTime);
	CreateSceneMVPMatrix();
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
	HRESULT result = S_OK;
	FLOAT clearColor[4] = { 0.4f, 0.4f, 0.8f, 1.0f };

	const UINT      currentFrameIndex = m_swapChain4->GetCurrentBackBufferIndex();
	ID3D12Resource* pCurrentBackBuffer = m_swapChainBuffers[currentFrameIndex].Get();
	ID3D12Resource* pRtvResource = m_rtvResources[rtvResourceIndex].Get();

	D3D12_RESOURCE_BARRIER preResourceBarriers[2];

	preResourceBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(pCurrentBackBuffer,
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET);
	preResourceBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(pRtvResource,
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);


	m_pCmdList->ResourceBarrier(2, preResourceBarriers);

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = GetRenderTargetView(currentFrameIndex, TRUE);
	ID3D12DescriptorHeap* descHeaps[] = { GetSrvDescriptorHeap() };

	m_pCmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
	m_pCmdList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	m_pCmdList->SetPipelineState(m_simpleComposition.pipelineState.Get());
	m_pCmdList->SetGraphicsRootSignature(m_simpleComposition.rootSignature.Get());
	m_pCmdList->SetDescriptorHeaps(1, descHeaps);
	m_pCmdList->SetGraphicsRootDescriptorTable(0, GetSrvGpuHeapHandle(rtvResourceIndex));
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

	ID3D12CommandList* pCmdLists[] = { m_pCmdList.Get() };
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
HRESULT Dx12SampleBase::OnInit()
{
	HRESULT result = S_OK;

	result = CreateDevice();
	result = CreateCommandQueues();
	result = CreateCommandList();
	result = CreateSwapChain();
	result = CreateFence();
	result = InitializeFrameComposition();

	///@todo refactor into a function
	{
		const UINT numRTVsForComposition = GetSwapChainBufferCount();
		const UINT numRTVsForApp = NumRTVsNeededForApp();
		const UINT numSRVsForApp = NumSRVsNeededForApp();
		const UINT numDSVsForApp = NumDSVsNeededForApp();

		///@todo sample base should setup descriptor heaps, RTVs and DSVs. App can override and return FALSE - yet to create this
		CreateRenderTargetDescriptorHeap(numRTVsForComposition + numRTVsForApp);
		CreateDepthStencilViewDescriptorHeap(numDSVsForApp);
		CreateShaderResourceViewDescriptorHeap(numRTVsForApp + numSRVsForApp);
		CreateRenderTargetViews(numRTVsForComposition, TRUE);
	}

	return result;
}

VOID Dx12SampleBase::AddTransformInfo(const DxMeshNodeTransformInfo& transformInfo)
{
	m_camera->AddTransformInfo(transformInfo);
}

HRESULT Dx12SampleBase::CreateSceneMVPMatrix()
{
	HRESULT result = S_OK;

	XMFLOAT4X4 mmData = m_camera->GetWorldMatrixTranspose(0);
	XMFLOAT4X4 mvpData = m_camera->GetModelViewProjectionMatrixTranspose(0);

	//we are getting the 4x4 matrix but need to ignore translation. That is done in shader by using only 3x3
	XMFLOAT4X4 normalData = m_camera->GetNormalMatrixData(0);
	XMFLOAT4 camPosition = m_camera->GetCameraPosition();

	const UINT numMatrix = 3; //model matrix, MPV matrix and normal matrix
	const UINT numFloat4 = 1;
	const UINT matrixSizeInBytes = (sizeof(FLOAT) * 16);
	const UINT float4SizeInBytes = (sizeof(FLOAT) * 4);
	const UINT constantBufferSizeInBytes = (matrixSizeInBytes * numMatrix) +
		(float4SizeInBytes * numFloat4);

	///@todo avoid using static
	static VOID* pMappedPtr = nullptr;
	static BYTE* pMappedBytePtr = nullptr;

	///@note constant buffer data layout
	/// MVP matrix (16 floats)
	if (pMappedPtr == nullptr)
	{
		m_mvpCameraConstantBuffer = CreateBufferWithData(nullptr, constantBufferSizeInBytes, TRUE);
		CD3DX12_RANGE readRange(0, 0);
		//@note specifying nullptr as read range indicates CPU can read entire resource
		m_mvpCameraConstantBuffer->Map(0, &readRange, &pMappedPtr);
		pMappedBytePtr = static_cast<BYTE*>(pMappedPtr);
	}
	assert(pMappedPtr != nullptr);
	assert(pMappedBytePtr == pMappedPtr);

	BYTE* pWritePtr = pMappedBytePtr;
	memcpy(pWritePtr, &mmData, matrixSizeInBytes);
	pWritePtr += matrixSizeInBytes;
	memcpy(pWritePtr, &mvpData, matrixSizeInBytes);
	pWritePtr += matrixSizeInBytes;
	memcpy(pWritePtr, &normalData, matrixSizeInBytes);
	pWritePtr += matrixSizeInBytes;
	memcpy(pWritePtr, &camPosition, float4SizeInBytes);

	return result;
}



