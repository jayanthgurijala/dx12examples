/*
* Copyright (C) 2026 by Jayanth Gurijala
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/
#include "pch.h"
#include "Dx12SampleBase.h"
#include "tiny_gltf.h"
#include "DxPrintUtils.h"
#include <memory>
#include <chrono>
#include "WICImageLoader.h"
#include "gltfutils.h"
#include "dxhelper.h"
#include <imgui.h>
#include <backends/imgui_impl_win32.h>
#include <backends/imgui_impl_dx12.h>
#include <backends/imgui_impl_win32.cpp>

using namespace WICImageLoader;
using namespace GltfUtils;

Dx12SampleBase* Dx12SampleBase::m_sampleBase = nullptr;
FLOAT Dx12SampleBase::s_frameDeltaTime = 0;


Dx12SampleBase::Dx12SampleBase(UINT width, UINT height) :
	m_width(width),
	m_height(height),
	m_simpleComposition({}),
	m_fenceValue(0),
	m_fenceEvent(0),
	m_frameDeltaTime(0),
	m_dsvDescHeap(nullptr),
	m_srvUavCbvDescHeap(nullptr),
	m_rtvDescHeap(nullptr),
	m_srvUavCbvDescriptorSize(0),
	m_rtvDescriptorSize(0),
	m_dsvDescriptorSize(0),
	m_samplerDescriptorSize(0),
	m_camera(std::make_unique<DxCamera>(width, height)),
	m_assetReader(std::make_unique<FileReader>()),
	m_userInput(std::make_unique<DxUserInput>(m_camera.get())),
	m_modelDrawPrimitive{},
	m_modelBaseColorTex2D(nullptr),
	m_modelIbv({}),
	m_hwnd(nullptr),
	m_appFrameInfo({}),
	m_gltfLoader(nullptr)
{
	m_sampleBase = this;
}

Dx12SampleBase::~Dx12SampleBase()
{
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

}

LRESULT CALLBACK Dx12SampleBase::WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
		return true;

	bool isimGuiMoving = FALSE;

	if (ImGui::GetCurrentContext() != nullptr)
	{
		ImGuiIO& io = ImGui::GetIO();

		if (io.WantCaptureMouse)
		{
			isimGuiMoving = TRUE;
		}
		else
		{
			isimGuiMoving = FALSE;
		}
	}
	

	switch (message)
	{
	case WM_MOUSEMOVE:
	{
		int x = GET_X_LPARAM(lParam);
		int y = GET_Y_LPARAM(lParam);
		if (isimGuiMoving == FALSE)
		{
			m_sampleBase->m_userInput->OnMouseMove(x, y);
		}
		break;
	}

	case WM_LBUTTONDOWN:
	{
		int x = GET_X_LPARAM(lParam);
		int y = GET_Y_LPARAM(lParam);
		if (isimGuiMoving == FALSE)
		{
			m_sampleBase->m_userInput->OnMouseDown(x, y, TRUE);
		}
		break;
	}

	case WM_LBUTTONUP:
		if (isimGuiMoving == FALSE)
		{
			m_sampleBase->m_userInput->OnMouseUp(0, 0, TRUE);
		}
		break;
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
	Initialize();
	OnInit();
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


UINT Dx12SampleBase::GetBackBufferCount()
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
	const UINT swapChainBufferCount = GetBackBufferCount();

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

HRESULT Dx12SampleBase::CreateSrvUavCbvDescriptorHeap(UINT numDescriptors)
{
	HRESULT result = S_OK;
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	descHeapDesc.NumDescriptors = numDescriptors;
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	result = m_pDevice->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&m_srvUavCbvDescHeap));
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
	const UINT start = (isInternal == TRUE) ? 0 : GetBackBufferCount();
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
	const UINT rtvOffset = (isInternal == TRUE) ? 0 : GetBackBufferCount();
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
	m_pCmdList->Reset(m_pCommandAllocator.Get(), nullptr);
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

	for (UINT i = 0; i < numResources; i++)
	{
		dxhelper::AllocateTexture2DResource(m_pDevice.Get(),
			                                &m_dsvResources[i],
			                                m_width,
			                                m_height,
			                                depthFormat,
			                                L"SampleBaseDepthResN",
			                                D3D12_RESOURCE_STATE_DEPTH_WRITE,
			                                &clearValue,
			                                D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
		if (createViews == TRUE)
		{
			assert(m_dsvDescHeap != nullptr && m_dsvDescHeap->GetDesc().NumDescriptors == numResources);
			auto dsvHeapHandle = GetDsvCpuHeapHandle(i);
			m_pDevice->CreateDepthStencilView(m_dsvResources[i].Get(), nullptr, dsvHeapHandle);
		}
	}

	return result;
}


HRESULT Dx12SampleBase::CreateRenderTargetResources(UINT numResources)
{
	HRESULT result = S_OK;

	m_rtvResources.clear();
	m_rtvResources.resize(numResources);

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
		
		dxhelper::AllocateTexture2DResource(m_pDevice.Get(),
			                                &m_rtvResources[i],
			                                m_width,
			                                m_height,
			                                GetBackBufferFormat(),
			                                L"BaseRtvN",
			                                D3D12_RESOURCE_STATE_RENDER_TARGET,
			                                &clearValue,
			                                D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
	}

	return result;
}

VOID Dx12SampleBase::CreateRenderTargetSRVs(UINT numSrvs)
{
	for (UINT i = 0; i < numSrvs; i++)
	{
		auto srvHandle = GetSrvUavCBvCpuHeapHandle(i);
		m_pDevice->CreateShaderResourceView(m_rtvResources[i].Get(),
			nullptr,
			srvHandle);
	}
}

/*
*
* SRV Heap Layout is
*
* 1. N SRV descriptors for RTV composition NumRTVsNeededForApp()
*		- These are created when app creates RTV resources CreateRenderTargetResourceAndSRVs
*
* 2. M SRV descriptors requested by application NumSRVsNeededForApp()
* 3. P UAV descriptors
*
*/
VOID Dx12SampleBase::CreateAppSrvDescriptorAtIndex(UINT appSrvIndex, ID3D12Resource* srvResource)
{
	const UINT appSrvStartIndex = NumRTVsNeededForApp();	//see comment above
	auto srvHandle = GetSrvUavCBvCpuHeapHandle(appSrvStartIndex + appSrvIndex);
	m_pDevice->CreateShaderResourceView(srvResource, nullptr, srvHandle);

}

VOID Dx12SampleBase::CreateAppUavDescriptorAtIndex(UINT appUavIndex, ID3D12Resource* uavResource)
{
	const UINT uavStartIndexInDescriptorHeap = NumRTVsNeededForApp() + NumSRVsNeededForApp();
	auto uavHandle = GetSrvUavCBvCpuHeapHandle(uavStartIndexInDescriptorHeap + appUavIndex);
	m_pDevice->CreateUnorderedAccessView(uavResource, nullptr, nullptr, uavHandle);
}

VOID Dx12SampleBase::CreateAppBufferSrvDescriptorAtIndex(UINT appSrvIndex, ID3D12Resource* srvResource, UINT numElements, UINT elementSize)
{
	const UINT appSrvStartIndex = NumRTVsNeededForApp();
	auto srvHandle = GetSrvUavCBvCpuHeapHandle(appSrvStartIndex + appSrvIndex);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	if (elementSize == 0)
	{
		srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
		srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
		srvDesc.Buffer.StructureByteStride = 0;
	}
	else
	{
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
		srvDesc.Buffer.StructureByteStride = elementSize;
	}
	
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Buffer.NumElements = numElements;
	
	m_pDevice->CreateShaderResourceView(srvResource, &srvDesc, srvHandle);
}

//VOID Dx12SampleBase::Create

ComPtr<ID3D12PipelineState> Dx12SampleBase::GetGfxPipelineStateWithShaders(const std::string& vertexShaderName,
																		   const std::string& pixelShaderName,
																		   ID3D12RootSignature* signature,
																		   const D3D12_INPUT_LAYOUT_DESC& iaLayout,
																		   BOOL enableWireFrame,
																		   BOOL doubleSided,
																		   BOOL useDepthStencil)
{
	ComPtr<ID3D12PipelineState> gfxPipelineState = nullptr;

	ComPtr<ID3DBlob> vertexShader = GetCompiledShaderBlob(vertexShaderName);
	ComPtr<ID3DBlob> pixelShader = GetCompiledShaderBlob(pixelShaderName);

	assert(vertexShader != nullptr && pixelShader != nullptr);


	auto rast    = dxhelper::GetRasterizerState();
	auto dsState = dxhelper::GetDepthStencilState(useDepthStencil);


		D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineStateDesc = {};
		pipelineStateDesc.InputLayout = iaLayout;
		pipelineStateDesc.pRootSignature = signature;
		pipelineStateDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
		pipelineStateDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
		pipelineStateDesc.RasterizerState = rast;
		pipelineStateDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		pipelineStateDesc.DepthStencilState = dsState;

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

	return gfxPipelineState;
}

HRESULT Dx12SampleBase::InitializeFrameComposition()
{
	HRESULT result = S_OK;

	m_simpleComposition.rootSignature = dxhelper::GetSrvDescTableRootSignature(m_pDevice.Get(), 1);

	auto layout_1 = dxhelper::GetInputLayoutDesc_Layout1();
	m_simpleComposition.pipelineState = GetGfxPipelineStateWithShaders("FrameSimple_VS.cso",
																	   "FrameSimple_PS.cso",
		                                                               m_simpleComposition.rootSignature.Get(),
		                                                               layout_1);

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

	m_simpleComposition.vertexBuffer = CreateBufferWithData(fullScreenTri, dataSizeInBytes, "FrameVB");

	m_simpleComposition.vertexBufferView.BufferLocation = m_simpleComposition.vertexBuffer->GetGPUVirtualAddress();
	m_simpleComposition.vertexBufferView.SizeInBytes = dataSizeInBytes;
	m_simpleComposition.vertexBufferView.StrideInBytes = sizeof(SimpleVertex);

	result = (m_simpleComposition.vertexBuffer != NULL) ? S_OK : E_FAIL;

	return result;
}

ComPtr<ID3D12Resource> Dx12SampleBase::CreateBufferWithData(void* cpuData,
															UINT sizeInBytes,
														    const char* resourceName,
													        D3D12_RESOURCE_FLAGS flags,
	                                                        D3D12_RESOURCE_STATES initialResourceState,
	                                                        BOOL isUploadHeap)
{
	ID3D12GraphicsCommandList* pCmdList = m_pCmdList.Get();
	ID3D12CommandQueue* pCmdQueue = m_pCmdQueue.Get();
	ComPtr<ID3D12Resource> bufferResource;
	dxhelper::AllocateBufferResource(m_pDevice.Get(), sizeInBytes, &bufferResource, resourceName, flags, initialResourceState, isUploadHeap);

	if (isUploadHeap == TRUE && cpuData != NULL && sizeInBytes > 0)
	{
		VOID* pMappedPtr = nullptr;
		CD3DX12_RANGE readRange(0, 0);
		//@note specifying nullptr as read range indicates CPU can read entire resource
		if (bufferResource->Map(0, &readRange, &pMappedPtr) == S_OK)
		{
			memcpy(pMappedPtr, cpuData, sizeInBytes);
			D3D12_RANGE writtenRange = { 0, sizeInBytes }; // Tell GPU which part was written
			bufferResource->Unmap(0, &writtenRange);
		}
		else
		{
			assert(0);
		}

	}
	else if (cpuData != NULL && sizeInBytes > 0)
	{
		UploadCpuDataAndWaitForCompletion(cpuData,
			sizeInBytes,
			pCmdList,
			pCmdQueue,
			bufferResource.Get(),
			D3D12_RESOURCE_STATE_COMMON,
			initialResourceState);
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

	m_pCmdList->Reset(m_pCommandAllocator.Get(), nullptr);

	return result;
}

HRESULT Dx12SampleBase::NextFrame(FLOAT frameDeltaTime)
{
	HRESULT result = S_OK;

	m_appFrameInfo.type = DxFrameInvalid;

	m_frameDeltaTime = frameDeltaTime;
	s_frameDeltaTime = frameDeltaTime;
	m_camera->Update(frameDeltaTime);
	CreateSceneMVPMatrix();

	//@todo only for integrating and testing imgui
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	ImGui::Begin("fps");

	static float values[1000] = {};
	static int values_offset = 0;

	values[values_offset] = ImGui::GetIO().Framerate;
	values_offset = (values_offset + 1) % IM_ARRAYSIZE(values);

	ImGui::PlotLines("FPS", values, IM_ARRAYSIZE(values));
	ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);

	CD3DX12_VIEWPORT viewPort(0.0f, 0.0f, FLOAT(m_width), FLOAT(m_height));
	CD3DX12_RECT     rect(0, 0, LONG(m_width), LONG(m_height));

	m_pCmdList->RSSetViewports(1, &viewPort);
	m_pCmdList->RSSetScissorRects(1, &rect);

	WaitForFenceCompletion(m_pCmdQueue.Get());
	result = RenderFrame();
	WaitForFenceCompletion(m_pCmdQueue.Get());

	assert(m_appFrameInfo.type != DxFrameInvalid);

	ImGui::End();
	RenderRtvContentsOnScreen();

	return result;
}

HRESULT Dx12SampleBase::RenderRtvContentsOnScreen()
{
	HRESULT result = S_OK;
	FLOAT clearColor[4] = { 0.4f, 0.4f, 0.8f, 1.0f };

	const UINT      currentFrameIndex = m_swapChain4->GetCurrentBackBufferIndex();
	ID3D12Resource* pCurrentBackBuffer = m_swapChainBuffers[currentFrameIndex].Get();
	ID3D12Resource* pFrameResource = (m_appFrameInfo.type == DxAppFrameType::DxFrameResource) ? m_appFrameInfo.pFrameResource : 
																	m_rtvResources[m_appFrameInfo.rtvIndex].Get();

	m_pDevice->CreateShaderResourceView(pFrameResource, nullptr, GetSrvUavCBvCpuHeapHandle(0));

	D3D12_RESOURCE_BARRIER preResourceBarriers[2];

	preResourceBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(pCurrentBackBuffer,
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET);
	preResourceBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(pFrameResource,
		m_appFrameInfo.pResState,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);


	m_pCmdList->ResourceBarrier(2, preResourceBarriers);

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = GetRenderTargetView(currentFrameIndex, TRUE);
	ID3D12DescriptorHeap* descHeaps[] = { GetSrvDescriptorHeap() };

	CD3DX12_VIEWPORT viewPort(0.0f, 0.0f, FLOAT(m_width), FLOAT(m_height));
	CD3DX12_RECT     rect(0, 0, LONG(m_width), LONG(m_height));
	m_pCmdList->RSSetViewports(1, &viewPort);
	m_pCmdList->RSSetScissorRects(1, &rect);

	m_pCmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
	m_pCmdList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	m_pCmdList->SetPipelineState(m_simpleComposition.pipelineState.Get());
	m_pCmdList->SetGraphicsRootSignature(m_simpleComposition.rootSignature.Get());
	m_pCmdList->SetDescriptorHeaps(1, descHeaps);
	m_pCmdList->SetGraphicsRootDescriptorTable(0, GetSrvGpuHeapHandle(0));
	m_pCmdList->IASetVertexBuffers(0, 1, &m_simpleComposition.vertexBufferView);
	m_pCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_pCmdList->DrawInstanced(3, 1, 0, 0);

	ID3D12DescriptorHeap* heaps[] = { m_imguiDescHeap.Get() };
	m_pCmdList->SetDescriptorHeaps(1, heaps);
	ImGui::Render();

	ImGui_ImplDX12_RenderDrawData(
		ImGui::GetDrawData(),
		m_pCmdList.Get()
	);

	D3D12_RESOURCE_BARRIER postResourceBarriers[2];

	postResourceBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(pCurrentBackBuffer,
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT);
	postResourceBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(pFrameResource,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		m_appFrameInfo.pResState);
	m_pCmdList->ResourceBarrier(2, postResourceBarriers);
	m_pCmdList->Close();

	ID3D12CommandList* pCmdLists[] = { m_pCmdList.Get() };
	m_pCmdQueue->ExecuteCommandLists(_countof(pCmdLists), pCmdLists);
	m_pCmdList->Reset(m_pCommandAllocator.Get(), nullptr);


	m_swapChain4->Present(1, 0);

	WaitForFenceCompletion(m_pCmdQueue.Get());

	return result;

}


HRESULT Dx12SampleBase::Initialize()
{
	HRESULT result = S_OK;

	InitlializeDeviceCmdQueueAndCmdList();
	InitializeRtvDsvDescHeaps();
	InitializeSrvCbvUavDescHeaps();
	InitializeImgui();
	LoadGltfFile();

	return result;
}

VOID Dx12SampleBase::InitlializeDeviceCmdQueueAndCmdList()
{
	CreateDevice();
	CreateCommandQueues();
	CreateCommandList();
	CreateSwapChain();
	CreateFence();
	InitializeFrameComposition();
}

VOID Dx12SampleBase::InitializeRtvDsvDescHeaps()
{
	const UINT numRTVsForComposition = GetBackBufferCount();
	const UINT numRTVsForApp         = NumRTVsNeededForApp();

	CreateRenderTargetResources(numRTVsForApp);
	CreateRenderTargetDescriptorHeap(numRTVsForComposition + numRTVsForApp);

	const UINT numDSVsForApp = NumDSVsNeededForApp();
	CreateDepthStencilViewDescriptorHeap(numDSVsForApp);
	CreateDsvResources(numDSVsForApp);


	CreateRenderTargetViews(numRTVsForComposition, TRUE);
	CreateRenderTargetViews(numRTVsForApp, FALSE);
}

VOID Dx12SampleBase::InitializeSrvCbvUavDescHeaps()
{
	const UINT numSRVsForApp = NumSRVsNeededForApp();
	const UINT numUAVsForApp = NumUAVsNeededForApp();
	const UINT numRTVsForApp = NumRTVsNeededForApp();
	//@note numRTVsForApp is for creating SRVs for the RTVs.
	CreateSrvUavCbvDescriptorHeap(numRTVsForApp + numSRVsForApp + numUAVsForApp);
	CreateRenderTargetSRVs(numRTVsForApp);
}

VOID Dx12SampleBase::InitializeImgui()
{
	Imgui_CreateDescriptorHeap();
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;   // optional
	//io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;       // optional
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(m_hwnd);

	ImGui_ImplDX12_InitInfo init_info = {};
	init_info.Device = m_pDevice.Get();
	init_info.NumFramesInFlight = GetBackBufferCount(); // usually 2 or 3
	init_info.RTVFormat = GetBackBufferFormat();  // your swapchain format

	///@todo avoid legacy path
	init_info.SrvDescriptorHeap = m_imguiDescHeap.Get();
	init_info.LegacySingleSrvCpuDescriptor = m_imguiDescHeap->GetCPUDescriptorHandleForHeapStart();
	init_info.LegacySingleSrvGpuDescriptor = m_imguiDescHeap->GetGPUDescriptorHandleForHeapStart();


	init_info.CommandQueue = m_pCmdQueue.Get();

	// Optional: provide allocation callbacks
	init_info.SrvDescriptorAllocFn = nullptr; // simple sample
	init_info.SrvDescriptorFreeFn = nullptr;

	ImGui_ImplDX12_Init(&init_info);
}

VOID Dx12SampleBase::RenderModel(ID3D12GraphicsCommandList* pCmdList)
{
	const SIZE_T numVertexBufferViews = m_modelVbvs.size();
	D3D12_VERTEX_BUFFER_VIEW vbv = {};
	vbv.BufferLocation = m_modelVbvs[0].BufferLocation;
	vbv.SizeInBytes = m_modelVbvs[0].SizeInBytes;
	vbv.StrideInBytes = m_modelVbvs[0].StrideInBytes;
	assert(vbv.SizeInBytes > 0);
	assert(vbv.StrideInBytes > 0);
	assert(vbv.BufferLocation != 0);
	pCmdList->IASetVertexBuffers(0, numVertexBufferViews, &m_modelVbvs[0]);

	static UINT s_numTrianglesToDraw = m_modelDrawPrimitive.numIndices / 3;
	static const UINT s_maxTriangles = s_numTrianglesToDraw;
	static INT s_numTrianglesToDrawFromUser = s_numTrianglesToDraw;

	ImGui::Text("Triangle Count");
	ImGui::SameLine();

	if (ImGui::Button("-##tricountminus"))
		s_numTrianglesToDrawFromUser -= 1;

	ImGui::SameLine();


	ImGui::SetNextItemWidth(80);
	ImGui::InputInt("##trianglecount", &s_numTrianglesToDrawFromUser, 0, 0);

	ImGui::SameLine();

	if (ImGui::Button("+##tricountplus"))
		s_numTrianglesToDrawFromUser += 1;

	ImGui::SameLine();


	if (ImGui::Button("Max"))
		s_numTrianglesToDrawFromUser = s_maxTriangles;

	if (ImGui::Button("Zero"))
		s_numTrianglesToDrawFromUser = 0;


	s_numTrianglesToDraw = s_numTrianglesToDrawFromUser;

	if (m_modelDrawPrimitive.isIndexedDraw == TRUE)
	{
		pCmdList->IASetIndexBuffer(&m_modelIbv);
		pCmdList->DrawIndexedInstanced(s_numTrianglesToDraw * 3, 1, 0, 0, 0);
	}
	else
	{
		pCmdList->DrawInstanced(s_numTrianglesToDraw * 3, 1, 0, 0);
	}
}




HRESULT Dx12SampleBase::CreateVSPSPipelineStateFromModel()
{
	HRESULT result = S_OK;

	char vertexShaderName[64];
	char pixelShaderName[64];

	ID3D12RootSignature* pRootSignature = GetRootSignature();

	assert(GetRootSignature() != nullptr && m_meshState.inputElementDesc.size() > 0);

	const SIZE_T numAttributes = m_meshState.inputElementDesc.size();
	snprintf(vertexShaderName, 64, "Simple%zu_VS.cso", numAttributes);
	snprintf(pixelShaderName, 64, "Simple%zu_PS.cso", numAttributes);


	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = {};
	inputLayoutDesc.NumElements = static_cast<UINT>(numAttributes);
	inputLayoutDesc.pInputElementDescs = m_meshState.inputElementDesc.data();


	///@todo tie up cull mode to double sided from gltf
	m_modelPipelineState = GetGfxPipelineStateWithShaders(vertexShaderName, pixelShaderName, pRootSignature, inputLayoutDesc, FALSE, TRUE, TRUE);

	return result;
}


VOID Dx12SampleBase::AddTransformInfo(const DxNodeTransformInfo& transformInfo)
{
	m_camera->AddTransformInfo(transformInfo);
}

HRESULT Dx12SampleBase::CreateSceneMVPMatrix()
{
	HRESULT result = S_OK;

	XMFLOAT4X4 mmData    = m_camera->GetWorldMatrixTranspose(0);
	XMFLOAT4X4 mvpData   = m_camera->GetModelViewProjectionMatrixTranspose(0);
	XMFLOAT4X4 invVpData = m_camera->GetViewProjectionInverse();

	//we are getting the 4x4 matrix but need to ignore translation. That is done in shader by using only 3x3
	XMFLOAT4X4 normalData  = m_camera->GetNormalMatrixData(0);
	XMFLOAT4   camPosition = m_camera->GetCameraPosition();

	const UINT numMatrix         = 4; //model matrix, MPV matrix, Inverse_viewProj(RayTracing) and normal matrix
	const UINT numFloat4         = 1;
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
		m_mvpCameraConstantBuffer = CreateBufferWithData(nullptr, constantBufferSizeInBytes, "Camera", D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COMMON, TRUE);
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
	memcpy(pWritePtr, &invVpData, matrixSizeInBytes);
	pWritePtr += matrixSizeInBytes;
	memcpy(pWritePtr, &normalData, matrixSizeInBytes);
	pWritePtr += matrixSizeInBytes;
	memcpy(pWritePtr, &camPosition, float4SizeInBytes);

	return result;
}

VOID Dx12SampleBase::CreateMeshState()
{
	m_meshState.inputElementDesc.clear();
	const SIZE_T numAttributes = m_modelIaSemantics.size();
	m_meshState.inputElementDesc.resize(numAttributes);
	for (UINT i = 0; i < numAttributes; i++)
	{
		auto& inputElementDesc = m_meshState.inputElementDesc[i];
		const UINT semanticIndex = m_modelIaSemantics[i].isIndexValid ? m_modelIaSemantics[i].index : 0;
		inputElementDesc.SemanticName = m_modelIaSemantics[i].name.c_str();
		inputElementDesc.AlignedByteOffset = 0;
		inputElementDesc.SemanticIndex = semanticIndex;
		inputElementDesc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		inputElementDesc.InstanceDataStepRate = 0;
		inputElementDesc.Format = m_modelIaSemantics[i].format;

		///@note depends on VB allocation. Need gltf to DX converter to give this info
		inputElementDesc.InputSlot = i;
	}
}


HRESULT Dx12SampleBase::LoadGltfFile()
{
	HRESULT result = S_OK;
	const std::string gltfFileName = GltfFileName();
	std::string modelPath = m_assetReader->GetFullModelFilePath(gltfFileName);
	m_gltfLoader = std::make_unique<DxGltfLoader>(modelPath);
	m_gltfLoader->LoadModel();
	

	if (result == S_OK)
	{
		DxNodeTransformInfo meshTransformInfo;
		m_gltfLoader->GetNodeTransformInfo(meshTransformInfo, 0, 0);
		AddTransformInfo(meshTransformInfo);

		DxGltfMeshPrimInfo gltfMeshPrimInfo;
		m_gltfLoader->LoadMeshPrimitiveInfo(gltfMeshPrimInfo, 0, 0, 0);

		UINT numAttributes = gltfMeshPrimInfo.vbInfo.size();
		m_modelVbBuffers.resize(numAttributes);
		m_modelVbvs.resize(numAttributes);
		m_modelIaSemantics.resize(numAttributes);

		for (int i = 0; i < numAttributes; i++)
		{
			const UINT64 offsetInBytesInBuffer = gltfMeshPrimInfo.vbInfo[i].bufferOffsetInBytes;
			const UINT64 bufferSizeInBytes     = gltfMeshPrimInfo.vbInfo[i].bufferSizeInBytes;
			const UINT   bufferIndex           = gltfMeshPrimInfo.vbInfo[i].bufferIndex;
			const UINT   bufferStrideInBytes   = gltfMeshPrimInfo.vbInfo[i].bufferStrideInBytes;

			BYTE* bufferData    = m_gltfLoader->GetBufferData(gltfMeshPrimInfo.vbInfo[i].bufferIndex);
			m_modelVbBuffers[i] = CreateBufferWithData(&bufferData[offsetInBytesInBuffer],
				                                       (UINT)bufferSizeInBytes,
				                                       gltfMeshPrimInfo.vbInfo[i].iaLayoutInfo.name.c_str());

			m_modelVbvs[i].BufferLocation = m_modelVbBuffers[i]->GetGPUVirtualAddress();
			m_modelVbvs[i].SizeInBytes    = bufferSizeInBytes;
			m_modelVbvs[i].StrideInBytes  = bufferStrideInBytes;

			m_modelIaSemantics[i] = gltfMeshPrimInfo.vbInfo[i].iaLayoutInfo;
		}

		
		{
			const UINT64 bufferSizeInBytes = gltfMeshPrimInfo.ibInfo.bufferSizeInBytes;
			BYTE* bufferData               = m_gltfLoader->GetBufferData(gltfMeshPrimInfo.ibInfo.bufferIndex);
			m_modelIbBuffer                = CreateBufferWithData(&bufferData[gltfMeshPrimInfo.ibInfo.bufferOffsetInBytes],
										           bufferSizeInBytes,
				                                   gltfMeshPrimInfo.ibInfo.name.c_str());

			m_modelIbv.BufferLocation = m_modelIbBuffer->GetGPUVirtualAddress();
			m_modelIbv.Format         = gltfMeshPrimInfo.ibInfo.indexFormat;
			m_modelIbv.SizeInBytes    = bufferSizeInBytes;
		}

		m_modelDrawPrimitive = gltfMeshPrimInfo.drawInfo;


		if (gltfMeshPrimInfo.materialInfo.isMaterialDefined == TRUE)
		{
			auto& gltfTextureInfo    = gltfMeshPrimInfo.materialInfo.pbrMetallicRoughness.baseColorTexture.texture.imageBufferInfo;
			BYTE* bufferData         = m_gltfLoader->GetBufferData(gltfTextureInfo.bufferIndex);
			UINT64 bufferSizeInBytes = gltfTextureInfo.bufferSizeInBytes;
			UINT64 bufferOffsetInBytes = gltfTextureInfo.bufferOffsetInBytes;

			ImageData imageData = WICImageLoader::LoadImageFromMemory_WIC(&bufferData[bufferOffsetInBytes], bufferSizeInBytes);
			m_modelBaseColorTex2D = CreateTexture2DWithData(imageData.pixels.data(), imageData.pixels.size(), imageData.width, imageData.height);
			CreateAppSrvDescriptorAtIndex(0, m_modelBaseColorTex2D.Get());
		}
	}

	CreateMeshState();

	return result;
}



//<------------------------------------------- Imgui ----------------------------------------->
VOID Dx12SampleBase::Imgui_CreateDescriptorHeap()
{
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	desc.NumDescriptors = 64;                     // font + optional images
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	desc.NodeMask = 0;
	m_pDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_imguiDescHeap));
}

//<----------------------------------------- Ray Tracing ------------------------------------->
VOID Dx12SampleBase::ExecuteBuildAccelerationStructures()
{
	m_pCmdList->Close();
	ID3D12CommandList* cmdLists[] = { m_pCmdList.Get() };
	m_pCmdQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);
	WaitForFenceCompletion(m_pCmdQueue.Get());
	m_pCmdList->Reset(m_pCommandAllocator.Get(), nullptr);
}

VOID Dx12SampleBase::StartBuildingAccelerationStructures()
{
	//m_pCmdList->Reset(m_pCommandAllocator.Get(), nullptr);
}


