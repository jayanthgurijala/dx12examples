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
#include "DxTransformHelper.h"
#include "DxGltfLoader.h"

using namespace WICImageLoader;
using namespace GltfUtils;

Dx12SampleBase* Dx12SampleBase::s_sampleBase = nullptr;
FLOAT Dx12SampleBase::s_frameDeltaTime = 0;


Dx12SampleBase::Dx12SampleBase(UINT width, UINT height) :
	m_width(width),
	m_height(height),
	m_simpleComposition({}),
	m_fenceValue(0),
	m_fenceEvent(0),
	m_frameDeltaTime(0),
	m_rtvDescriptorSize(0),
	m_dsvDescriptorSize(0),
	m_samplerDescriptorSize(0),
	m_assetReader(std::make_unique<FileReader>()),
	m_modelAssets({}),
	m_hwnd(nullptr),
	m_appFrameInfo({}),
	m_gltfLoader(nullptr),
	m_descriptorHeapManager(nullptr)
{
	s_sampleBase = this;

	
	m_camLightsMaterialsManager = std::make_unique<CameraLightsMaterialsBuffer>();
	m_camera = std::make_unique<DxCamera>(width, height);
	m_camera->Initialize();
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
			s_sampleBase->m_camera->OnMouseMove(x, y);
		}
		break;
	}

	case WM_LBUTTONDOWN:
	{
		int x = GET_X_LPARAM(lParam);
		int y = GET_Y_LPARAM(lParam);
		if (isimGuiMoving == FALSE)
		{
			s_sampleBase->m_camera->OnMouseDown(x, y, TRUE);
		}
		break;
	}

	case WM_LBUTTONUP:
		if (isimGuiMoving == FALSE)
		{
			s_sampleBase->m_camera->OnMouseUp(0, 0, TRUE);
		}
		break;
	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE)
			PostQuitMessage(0);
		if (wParam == 'W')
		{
			s_sampleBase->m_camera->MoveForward();
        }
		if (wParam == 'S')
		{
			s_sampleBase->m_camera->MoveBack();
		}
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

	m_samplerDescriptorSize = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
	m_dsvDescriptorSize = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	m_rtvDescriptorSize = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);


	return result;
}

HRESULT Dx12SampleBase::CreateCommandQueues()
{
	HRESULT result = S_OK;

	{
		D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
		cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

		result = m_pDevice->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&m_pCmdQueue));
	}

	{
		D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
		cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
		cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		result = m_pDevice->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&m_pComputeCmdQueue));

	}

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


VOID Dx12SampleBase::CreateRenderTargetViews()
{

	///@note ORDERING is very important!
	///      RTV Desc heap Layout: [App Specific RTVs | Swap Chain RTVs]

	const UINT numRTVsForApp = m_rtvResources.size();
	assert(numRTVsForApp == NumRTVsNeededForApp());
	assert(m_descriptorHeapManager != nullptr);
	
	for (UINT idx = 0; idx < numRTVsForApp; idx++)
	{
		m_descriptorHeapManager->CreateRtvDescriptor(idx, nullptr, m_rtvResources[idx].Get(), TRUE, TRUE);
	}

	const UINT numBackBuffers = m_swapChainBuffers.size();
	for (UINT idx = 0; idx < numBackBuffers; idx++)
	{
		m_descriptorHeapManager->CreateRtvDescriptor(numRTVsForApp + idx, nullptr, m_swapChainBuffers[idx].Get(), TRUE, FALSE);
	}
}


HRESULT Dx12SampleBase::CreateCommandList()
{
	HRESULT result = S_OK;

	{
		result = m_pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_pCommandAllocator));
		result = m_pDevice->CreateCommandList(0,
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			m_pCommandAllocator.Get(),
			nullptr,
			IID_PPV_ARGS(&m_pCmdList));
		m_pCmdList->Close();
		m_pCmdList->Reset(m_pCommandAllocator.Get(), nullptr);
	}

	{
		m_pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE, IID_PPV_ARGS(&m_pComputeCmdAllocator));
		m_pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COMPUTE,
			                         m_pComputeCmdAllocator.Get(), nullptr,
			                         IID_PPV_ARGS(&m_pComputeCommandList));
		m_pComputeCommandList->Close();
		m_pComputeCommandList->Reset(m_pComputeCmdAllocator.Get(), nullptr);
	}
	return result;
}

VOID Dx12SampleBase::CreateDsvResourcesAndViews()
{
	HRESULT result = S_OK;
	const UINT numDSVsForApp = NumDSVsNeededForApp();

	m_dsvResources.clear();
	m_dsvResources.resize(numDSVsForApp);

	const DXGI_FORMAT dsvResFormat = GetDepthStencilResourceFormat();

	for (UINT i = 0; i < numDSVsForApp; i++)
	{
		dxhelper::AllocateTexture2DResource(m_pDevice.Get(),
			                                &m_dsvResources[i],
			                                m_width,
			                                m_height,
			                                dsvResFormat,
			                                L"SampleBaseDepthResN",
			                                D3D12_RESOURCE_STATE_DEPTH_WRITE,
			                                nullptr,
			                                D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
		dsvDesc.Format                        = DXGI_FORMAT_D32_FLOAT;
		dsvDesc.ViewDimension                 = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Flags                         = D3D12_DSV_FLAG_NONE;

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format                          = DXGI_FORMAT_R32_FLOAT;
		srvDesc.ViewDimension                   = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Shader4ComponentMapping         = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

		srvDesc.Texture2D.MipLevels       = 1;
		srvDesc.Texture2D.MostDetailedMip = 0;

		m_descriptorHeapManager->CreateDsvDescriptor(i, &dsvDesc, &srvDesc, m_dsvResources[i].Get(), TRUE);
	}
}


VOID Dx12SampleBase::CreateRenderTargetResources()
{

	const UINT numRTVsForApp = NumRTVsNeededForApp();

	m_rtvResources.clear();
	m_rtvResources.resize(numRTVsForApp);

	FLOAT* clearColor4 = RenderTargetClearColor().data();

	//@todo find a way to match these 	FLOAT clearColor[4] = { 0.7f, 0.7f, 1.0f, 1.0f};
	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = GetBackBufferFormat();
	clearValue.Color[0] = clearColor4[0];
	clearValue.Color[1] = clearColor4[1];
	clearValue.Color[2] = clearColor4[2];
	clearValue.Color[3] = clearColor4[3];

	auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	for (UINT i = 0; i < numRTVsForApp; i++)
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
}

/*
* Base implementation to render all loaded scene elements once.
*/
VOID Dx12SampleBase::LoadSceneDescription(std::vector<DxSceneElementInstance>& sceneDescription)
{
	const UINT numSceneElements = NumModelAssetsLoaded();
	sceneDescription.resize(numSceneElements);

	for (UINT idx = 0; idx < numSceneElements; idx++)
	{
		sceneDescription[idx].numInstances = 1;
		sceneDescription[idx].sceneElementIdx = idx;
		sceneDescription[idx].addToExtents = TRUE;

		sceneDescription[idx].trsMatrix.resize(1);
		auto& trsMatrix = sceneDescription[idx].trsMatrix[0];

		DxTransformHelper::SetTranslationValues(trsMatrix, 0, 0, 0);
		DxTransformHelper::SetRotationInDegrees(trsMatrix, 0, 0, 0);
		DxTransformHelper::SetScaleValues(trsMatrix, 1, 1, 1);
	}
}


VOID Dx12SampleBase::CreateAppSrvDescriptorAtIndex(UINT linearPrimIdx, UINT offsetInPrim, ID3D12Resource* srvResource)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
	desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	desc.Texture2D.MipLevels = 0;

	D3D12_SHADER_RESOURCE_VIEW_DESC* descPtr = nullptr;
	if (srvResource == nullptr)
	{
		descPtr = &desc;
	}
	auto srvHandle = m_descriptorHeapManager->GetPerPrimSrvHeapHandle<CD3DX12_CPU_DESCRIPTOR_HANDLE>(linearPrimIdx, offsetInPrim);
	m_pDevice->CreateShaderResourceView(srvResource, descPtr, srvHandle);
}

VOID Dx12SampleBase::CreateAppUavDescriptorAtIndex(UINT appUavIndex, ID3D12Resource* uavResource)
{
	m_descriptorHeapManager->CreateUavDescriptor(appUavIndex, nullptr, uavResource, nullptr, TRUE);
}

VOID Dx12SampleBase::CreateAppBufferSrvDescriptorAtIndex(UINT linearPrimIdx, UINT offsetInPrim, ID3D12Resource* srvResource, UINT numElements, UINT elementSize)
{
	auto srvHandle = m_descriptorHeapManager->GetPerPrimSrvHeapHandle<CD3DX12_CPU_DESCRIPTOR_HANDLE>(linearPrimIdx, offsetInPrim);

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

ComPtr<ID3D12PipelineState> Dx12SampleBase::GetGfxPipelineStateWithShaders(const std::string& vertexShaderName,
													                       const std::string& hullShaderName,
																	       const std::string& domainShaderName,
																		   const std::string& pixelShaderName,
																		   ID3D12RootSignature* signature,
																		   const D3D12_INPUT_LAYOUT_DESC& iaLayout,
																		   BOOL enableWireFrame,
																		   BOOL doubleSided,
																		   BOOL useDepthStencil,
																		   BOOL enableBlend)
{
	auto GetShaderByteCodeFromBlob = [](ID3DBlob* blob)
		{
			D3D12_SHADER_BYTECODE shaderByteCode;
			if (blob != nullptr)
			{
				shaderByteCode = CD3DX12_SHADER_BYTECODE(blob);
			}
			else
			{
				shaderByteCode.BytecodeLength = 0;
				shaderByteCode.pShaderBytecode = nullptr;
			}
			return shaderByteCode;
		};

	auto GetShaderBlob = [this](std::string shaderName) {
		ComPtr<ID3DBlob> shaderBlob = nullptr; 
		if (shaderName.length() > 0)
		{
			shaderBlob = GetCompiledShaderBlob(shaderName);
		}
		return shaderBlob;
		};


	ComPtr<ID3D12PipelineState> gfxPipelineState = nullptr;

	auto blobVertexShader = GetShaderBlob(vertexShaderName);
	auto blobHullShader   = GetShaderBlob(hullShaderName);
	auto blobDomainShader = GetShaderBlob(domainShaderName);
	auto blobPixelShader  = GetShaderBlob(pixelShaderName);

	auto primTopology = (blobHullShader != nullptr) ? D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH : D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	BOOL forceWireFrame = (primTopology == D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH) ? TRUE : FALSE;

	if (forceWireFrame == TRUE)
	{
		enableWireFrame = TRUE;
	}

    const D3D12_CULL_MODE cullMode = doubleSided ? D3D12_CULL_MODE_NONE : D3D12_CULL_MODE_BACK;
    const D3D12_FILL_MODE fillMode = enableWireFrame ? D3D12_FILL_MODE_WIREFRAME : D3D12_FILL_MODE_SOLID;
    
	auto rast = dxhelper::GetRasterizerState(cullMode, fillMode);
    auto blend = dxhelper::GetBlendState(enableBlend);
	BOOL enableDepthWrite = (enableBlend == TRUE) ? FALSE : TRUE;
	auto dsState = dxhelper::GetDepthStencilState(useDepthStencil, FALSE, enableDepthWrite);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineStateDesc = {};
	pipelineStateDesc.InputLayout       = iaLayout;
	pipelineStateDesc.pRootSignature    = signature;
	pipelineStateDesc.VS                = GetShaderByteCodeFromBlob(blobVertexShader.Get());
	pipelineStateDesc.HS                = GetShaderByteCodeFromBlob(blobHullShader.Get());
	pipelineStateDesc.DS                = GetShaderByteCodeFromBlob(blobDomainShader.Get());
	pipelineStateDesc.PS                = GetShaderByteCodeFromBlob(blobPixelShader.Get());
	pipelineStateDesc.RasterizerState   = rast;
	pipelineStateDesc.BlendState        = blend;
	pipelineStateDesc.DepthStencilState = dsState;
	
	///@todo write some test cases for this
	pipelineStateDesc.SampleMask = UINT_MAX;
	pipelineStateDesc.PrimitiveTopologyType = primTopology;
	pipelineStateDesc.NumRenderTargets = 1;
	pipelineStateDesc.RTVFormats[0] = GetBackBufferFormat();
	pipelineStateDesc.DSVFormat = (useDepthStencil == TRUE) ? GetDepthStencilDsvFormat() : DXGI_FORMAT_UNKNOWN;
	
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
																	   "",
															           "",
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

VOID Dx12SampleBase::InitializeMipGenerationPipeline()
{
	//compute root signature - srv and uav
	//computepso - compute shader
	//descriptorheap

	UINT numDescHeapRanges = 2;
	std::vector <CD3DX12_DESCRIPTOR_RANGE> descTableRanges(numDescHeapRanges);
	descTableRanges[0] = dxhelper::GetSRVDescRange(1);
	descTableRanges[1] = dxhelper::GetUAVDescRange(1);
	auto descTable = dxhelper::GetRootDescTable(descTableRanges);
	auto rootConstants = dxhelper::GetRootConstants(2);

	dxhelper::DxCreateRootSignature(
		m_pDevice.Get(),
		&m_computeGenerateMips.rootSignature,
		{ descTable,
		  rootConstants},
		{},
		D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS
	);

	ComPtr<ID3DBlob> computeShader = GetCompiledShaderBlob("CsGenerateMips.cso");

	D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};
	desc.pRootSignature = m_computeGenerateMips.rootSignature.Get();
	desc.CS = CD3DX12_SHADER_BYTECODE(computeShader.Get());
	desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

	m_pDevice->CreateComputePipelineState(&desc, IID_PPV_ARGS(&m_computeGenerateMips.pipelineState));
}

VOID Dx12SampleBase::GenerateMipLevels(ID3D12Resource* tex2D, UINT width, UINT height)
{
	const UINT numMipLevels = dxhelper::GetNumMipLevels(width, height);
	const UINT localX = 8;
	const UINT localY = 8;

	///@todo need to wait on CPU for generating mips for multiple textures. Find a better way
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors             = numMipLevels * 2;
	heapDesc.Flags                      = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heapDesc.Type                       = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	ComPtr<ID3D12DescriptorHeap> csMipHeap;
	m_pDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&csMipHeap));
	const UINT handleIncrement = m_pDevice->GetDescriptorHandleIncrementSize(heapDesc.Type);

	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuDescHandleBase(csMipHeap->GetCPUDescriptorHandleForHeapStart());
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuDescHandleBase(csMipHeap->GetGPUDescriptorHandleForHeapStart());

	for (UINT mipLevel = 1; mipLevel < numMipLevels; mipLevel++)
	{
		CD3DX12_SHADER_RESOURCE_VIEW_DESC srvMip = CD3DX12_SHADER_RESOURCE_VIEW_DESC::Tex2D(tex2D->GetDesc().Format, 1, mipLevel - 1);

		assert(tex2D->GetDesc().Format == DXGI_FORMAT_R8G8B8A8_UNORM || tex2D->GetDesc().Format == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
		CD3DX12_UNORDERED_ACCESS_VIEW_DESC uavMip = CD3DX12_UNORDERED_ACCESS_VIEW_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, mipLevel);

	
		m_pDevice->CreateShaderResourceView(tex2D, &srvMip, cpuDescHandleBase);
		cpuDescHandleBase.Offset(1, handleIncrement);
		m_pDevice->CreateUnorderedAccessView(tex2D, nullptr, &uavMip, cpuDescHandleBase);
		cpuDescHandleBase.Offset(1, handleIncrement);
	}

	UINT mipLevelWidth = width;
	UINT mipLevelHeight = height;
	ID3D12DescriptorHeap* descHeaps[] = { csMipHeap.Get() };
	m_pComputeCommandList->SetDescriptorHeaps(_countof(descHeaps), descHeaps);
	m_pComputeCommandList->SetComputeRootSignature(m_computeGenerateMips.rootSignature.Get());
	m_pComputeCommandList->SetPipelineState(m_computeGenerateMips.pipelineState.Get());
	for (UINT mipLevel = 1; mipLevel < numMipLevels; mipLevel++)
	{
		mipLevelWidth  /= 2;
		mipLevelHeight /= 2;

		UINT dispatchX = mipLevelWidth / localX;
		UINT dispatchY = mipLevelHeight / localY;

		if (dispatchX == 0)
		{
			dispatchX = 1;
		}

		if (dispatchY == 0)
		{
			dispatchY = 1;
		}


		m_pComputeCommandList->SetComputeRootDescriptorTable(0, gpuDescHandleBase);
		m_pComputeCommandList->SetComputeRoot32BitConstants(1, 1, &mipLevelWidth, 0);
		m_pComputeCommandList->SetComputeRoot32BitConstants(1, 1, &mipLevelWidth, 1);
		m_pComputeCommandList->Dispatch(dispatchX, dispatchY, 1);
		gpuDescHandleBase.Offset(2, handleIncrement);

		// After dispatch for a mip level
		CD3DX12_RESOURCE_BARRIER uavBarrier = CD3DX12_RESOURCE_BARRIER::UAV(nullptr);
		m_pComputeCommandList->ResourceBarrier(1, &uavBarrier);
	}
	
	m_pComputeCommandList->Close();
	ID3D12CommandList* cmdLists[] = {m_pComputeCommandList.Get()};
	m_pComputeCmdQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);
	m_pComputeCommandList->Reset(m_pComputeCmdAllocator.Get(), nullptr);
	m_pComputeCmdQueue->Signal(m_fence.Get(), m_fenceValue);
	WaitForFenceCompletion(m_pComputeCmdQueue.Get());
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
	auto resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(format, width, height, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

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
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		GenerateMipLevels(texture2D.Get(), width, height);
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

	m_pCommandAllocator->Reset();
	m_pCmdList->Reset(m_pCommandAllocator.Get(), nullptr);

	return result;
}

HRESULT Dx12SampleBase::NextFrame(FLOAT frameDeltaTime)
{
	HRESULT result = S_OK;

	m_appFrameInfo.clear();

	s_frameDeltaTime = frameDeltaTime;
	m_camera->Update(frameDeltaTime);

	CreateSceneMVPMatrix();

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

	RenderFrame();
	WaitForFenceCompletion(m_pCmdQueue.Get());

	assert(m_appFrameInfo.size() > 0);

	ImGui::End();
	RenderRtvContentsOnScreen();

	return result;
}

VOID Dx12SampleBase::RenderFrameGfxDraw()
{
	//Needs to be overridden always
	//1. Call RenderGfxDrawInit
	assert(0);
}

VOID Dx12SampleBase::RenderGfxDrawInit(ID3D12RootSignature* rootSignature, UINT viewProjIndex, D3D12_PRIMITIVE_TOPOLOGY primTopology)
{
	m_pCmdList->IASetPrimitiveTopology(primTopology);
	m_pCmdList->SetGraphicsRootSignature(rootSignature);
	ID3D12DescriptorHeap* descHeaps[] = { GetSrvDescriptorHeap() };
	m_pCmdList->SetDescriptorHeaps(_countof(descHeaps), descHeaps);
}

VOID Dx12SampleBase::RenderSceneGfxDraw()
{
	ForEachModelAssetInstancesInScene([this](UINT modelAssetIdx, UINT primIdx, UINT instanceIdx, UINT flatInstanceIdx)
		{
			auto& curPrimitive     = GetPrimitiveInfo(modelAssetIdx, primIdx);
			auto  srvGpuHandle     = m_descriptorHeapManager->GetPerPrimSrvHeapHandle<CD3DX12_GPU_DESCRIPTOR_HANDLE>(curPrimitive.primLinearIdxInModelAssets, 0);
			m_pCmdList->SetPipelineState(curPrimitive.pipelineState.Get());

			//World matrix
			m_pCmdList->SetGraphicsRootConstantBufferView(1, GetPerInstanceDataGpuVa(flatInstanceIdx));

			//SRVs
			m_pCmdList->SetGraphicsRootDescriptorTable(2, srvGpuHandle);

			//Material props
			m_pCmdList->SetGraphicsRootConstantBufferView(3, curPrimitive.materialTextures.meterialCb);

			RenderModel(m_pCmdList.Get(), modelAssetIdx, primIdx);
		});
}

///@note 
VOID Dx12SampleBase::RenderRtvContentsOnScreen()
{
	FLOAT clearColor[4] = { 0.4f, 0.4f, 0.8f, 1.0f };
	const UINT      currentFrameIndex  = m_swapChain4->GetCurrentBackBufferIndex();
	ID3D12Resource* pCurrentBackBuffer = m_swapChainBuffers[currentFrameIndex].Get();

	D3D12_RESOURCE_BARRIER preResourceBarriers[1];

	preResourceBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(pCurrentBackBuffer,
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET);
	m_pCmdList->ResourceBarrier(1, preResourceBarriers);

	

	///@todo refactor
	const UINT numRTVsForApp = NumRTVsNeededForApp();
	auto       rtvHandle = m_descriptorHeapManager->GetRtvHeapHandle<CD3DX12_CPU_DESCRIPTOR_HANDLE>(numRTVsForApp + currentFrameIndex);
	
	ID3D12DescriptorHeap* descHeaps[] = { GetSrvDescriptorHeap() };

	CD3DX12_RECT     rect(0, 0, LONG(m_width), LONG(m_height));
	m_pCmdList->RSSetScissorRects(1, &rect);
	

	m_pCmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
	m_pCmdList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	m_pCmdList->SetPipelineState(m_simpleComposition.pipelineState.Get());
	m_pCmdList->SetGraphicsRootSignature(m_simpleComposition.rootSignature.Get());
	m_pCmdList->SetDescriptorHeaps(1, descHeaps);
	m_pCmdList->IASetVertexBuffers(0, 1, &m_simpleComposition.vertexBufferView);
	m_pCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	const UINT numCompositionElements = m_appFrameInfo.size();
	const FLOAT fractionForEachElement = 1.0 / numCompositionElements;

	FLOAT compX   = 0;
	FLOAT compY   = 0;
	FLOAT compWidth  = m_width * fractionForEachElement;
	FLOAT fullHeight = m_height;

	for (UINT i = 0; i < numCompositionElements; i++)
	{
		ComposeSrvContents(compX, compY, compWidth, fullHeight, m_appFrameInfo[i]);
		compX += (compWidth + 2);
	}
	

	ID3D12DescriptorHeap* heaps[] = { m_imguiDescHeap.Get() };
	m_pCmdList->SetDescriptorHeaps(1, heaps);
	ImGui::Render();
	
	ImGui_ImplDX12_RenderDrawData(
		ImGui::GetDrawData(),
		m_pCmdList.Get()
	);

	D3D12_RESOURCE_BARRIER postResourceBarriers[1];

	postResourceBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(pCurrentBackBuffer,
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT);
	m_pCmdList->ResourceBarrier(1, postResourceBarriers);

	
	
	m_pCmdList->Close();

	ID3D12CommandList* pCmdLists[] = { m_pCmdList.Get() };
	m_pCmdQueue->ExecuteCommandLists(_countof(pCmdLists), pCmdLists);
	
	m_swapChain4->Present(1, 0);

	WaitForFenceCompletion(m_pCmdQueue.Get());

	m_pCommandAllocator->Reset();
	m_pCmdList->Reset(m_pCommandAllocator.Get(), nullptr);
}

VOID Dx12SampleBase::ComposeSrvContents(FLOAT x, FLOAT y, FLOAT width, FLOAT height, DxAppFrameInfo& appFrameInfo)
{
	CD3DX12_VIEWPORT viewPort(x, y, width, height);

	m_pCmdList->RSSetViewports(1, &viewPort);


	assert(appFrameInfo.heapOffset != UINT_MAX);
	
	ID3D12Resource* pFrameResource = m_descriptorHeapManager->GetResourceForDescriptorTypeInOffset(
		appFrameInfo.descriptorType,
		appFrameInfo.heapOffset);

	assert(pFrameResource != nullptr);

	D3D12_RESOURCE_STATES initialState = dxhelper::ResourceStateFromType(appFrameInfo.descriptorType);
	auto srvGpuHandle = m_descriptorHeapManager->GetSrvHandleForDescriptor(appFrameInfo.descriptorType, appFrameInfo.heapOffset);

	m_pCmdList->SetGraphicsRootDescriptorTable(0, srvGpuHandle);

	D3D12_RESOURCE_BARRIER preResourceBarriers[1];
	preResourceBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(pFrameResource,
		initialState,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	m_pCmdList->ResourceBarrier(1, preResourceBarriers);


	m_pCmdList->DrawInstanced(3, 1, 0, 0);

	D3D12_RESOURCE_BARRIER postResourceBarriers[1];
	postResourceBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(pFrameResource,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		initialState);
	m_pCmdList->ResourceBarrier(1, postResourceBarriers);
}


HRESULT Dx12SampleBase::Initialize()
{
	HRESULT result = S_OK;

	InitlializeDeviceCmdQueueAndCmdList();
	LoadGltfFiles();
	InitializeDescriptorHeapManagerResourcesAndDescriptors();
	LoadSceneMaterialInfo();

	LoadSceneDescription(m_sceneDescription);
	LoadScene();
	CreateSceneMaterialCb();
	CreateSceneMVPMatrix();
	InitializeImgui();

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
	InitializeMipGenerationPipeline();
}

VOID Dx12SampleBase::InitializeDescriptorHeapManagerResourcesAndDescriptors()
{
	
	const UINT numUAVsForApp = NumUAVsNeededForApp();
	const UINT numRTVs       = NumRTVsNeededForApp() + GetBackBufferCount();
	const UINT numDSvsForApp = NumDSVsNeededForApp();
	const UINT numSrvs       = NumSRVsInAllModelAssets();



	m_descriptorHeapManager = std::make_unique<DxDescriptorHeapManager>(m_pDevice.Get(),
		                                                                numRTVs,
		                                                                numDSvsForApp,
		                                                                numUAVsForApp,
		                                                                numSrvs,
		                                                                NumSRVsPerPrimitive());

	CreateRenderTargetResources();
	CreateDsvResourcesAndViews();
	CreateRenderTargetViews();
	
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

VOID Dx12SampleBase::RenderModel(ID3D12GraphicsCommandList* pCmdList, UINT assetIdx, UINT primIdx)
{
	const UINT numVertexBufferViews = NumVertexAttributesInPrimitive(assetIdx, primIdx);

	///need some generalization to support chinese dragon etc
	//assert(numVertexBufferViews == 3);

	const auto& curPrimInfo = GetPrimitiveInfo(assetIdx, primIdx);

	//@note Position and Normal are always there, at 0 and 1 index, chinesedragon as no texture so no UVs
	const auto& positionVbv = GetVertexBufferInfo(curPrimInfo, GltfVertexAttribPosition)->modelVbv;
	const auto& normalVbv   = GetVertexBufferInfo(curPrimInfo, GltfVertexAttribNormal)->modelVbv;

	pCmdList->IASetVertexBuffers(0, 1, &positionVbv);
	pCmdList->IASetVertexBuffers(1, 1, &normalVbv);

	auto* uvVbData = GetVertexBufferInfo(curPrimInfo, GltfVertexAttribTexcoord0);
	if (uvVbData != nullptr)
	{
		const auto& uv0Vbv = GetVertexBufferInfo(curPrimInfo, GltfVertexAttribTexcoord0)->modelVbv;
		pCmdList->IASetVertexBuffers(2, 1, &uv0Vbv);
	}
	else
	{
		//Set dummy buffer for UVs, shader should not read from it as it should check material textures count before reading UVs
        D3D12_VERTEX_BUFFER_VIEW dummyVbv = {};
        pCmdList->IASetVertexBuffers(2, 1, &dummyVbv);
	}

	auto& primitiveDrawInfo = GetModelAssetDrawInfo(assetIdx, primIdx);

	if (primitiveDrawInfo.isIndexedDraw == TRUE)
	{
		auto& indexBufferView = GetIndexBufferInfo(curPrimInfo).modelIbv;
		pCmdList->IASetIndexBuffer(&indexBufferView);
		pCmdList->DrawIndexedInstanced(primitiveDrawInfo.numIndices, 1, 0, 0, 0);
	}
	else
	{
		pCmdList->DrawInstanced(primitiveDrawInfo.numVertices, 1, 0, 0);
	}
}

HRESULT Dx12SampleBase::CreatePerPrimGfxPipelineState()
{
	HRESULT result = S_OK;

	ForEachModelAssetPrimitive([this](UINT assetIdx, UINT primIdx)
	{

		auto& curPrimitive = GetPrimitiveInfo(assetIdx, primIdx);
		ID3D12RootSignature* pRootSignature = GetRootSignature();

		assert(GetRootSignature() != nullptr);

		std::vector<D3D12_INPUT_ELEMENT_DESC> modelIaSemantics;
		const UINT numAttributes = CreateInputElementDesc(curPrimitive.vertexBufferInfo, modelIaSemantics);
		assert(numAttributes == modelIaSemantics.size());

		std::string vertexShaderName = GetVertexShaderName(numAttributes);
		std::string hullShaderName   = GetHullShaderName();
		std::string domainShaderName = GetDomainShaderName();
		std::string pixelShaderName  = GetPixelShaderName();

		D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = { modelIaSemantics.data(), numAttributes, };

		const BOOL doubleSied = ((curPrimitive.materialCbData.flags & MaterialFlagsDoubleSided) == 0) ? FALSE : TRUE;
		const BOOL blendEnabled = ((curPrimitive.materialCbData.flags & MaterialFlagsAlphaModeBlend) == 0) ? FALSE : TRUE;

		curPrimitive.pipelineState = GetGfxPipelineStateWithShaders(vertexShaderName,
																	hullShaderName,
																	domainShaderName,
																	pixelShaderName,
																	pRootSignature,
																	inputLayoutDesc,
																	FALSE,                  //wireframe
																	doubleSied,				//doubleSided
																	TRUE,				    //depth enable?
																	blendEnabled);
	});
	
	return result;
}


HRESULT Dx12SampleBase::CreateSceneMVPMatrix()
{
	HRESULT result = S_OK;

	m_camLightsMaterialsManager->Finalize(m_pDevice.Get());
	
	{
		DxCBSceneData sceneData; //viewProj, invviewProj, cameraPosition, FovY and padding
		sceneData.viewProjMatrix = m_camera->GetViewProjectionMatrixTranspose();
		sceneData.invViewProj    = m_camera->GetViewProjectionInverse();
		sceneData.cameraPosition = m_camera->GetCameraPosition();
		sceneData.cameraFovY.x   = m_camera->GetFovYInRadians();
		sceneData.cameraFovY.y   = sceneData.cameraFovY.z = sceneData.cameraFovY.w = 0;
		sceneData.renderflags    = (((IsTessEnabled() == TRUE) ? RenderFlagsTessEnabled : 0) | ((EnablePBRShading() == TRUE) ? RenderFlagsUsePBR : 0));
	
		m_camLightsMaterialsManager->WriteViewProjDataAtIndex(0, sceneData);
	}

	{
		const UINT numNodeTransforms = m_camera->NumModelTransforms();

		UINT linearIndex = 0;
		DxCBPerInstanceData instanceTransformData;
		for (UINT idx = 0; idx < numNodeTransforms; idx++)
		{
			BYTE* pWritePtr = m_camLightsMaterialsManager->GetPerInstanceDataCpuVa(linearIndex);
			instanceTransformData.modelMatrix = m_camera->GetWorldMatrixTranspose(linearIndex);
			instanceTransformData.normalMatrix = m_camera->GetNormalMatrixData(linearIndex);

			m_camLightsMaterialsManager->WritePerInstanceWorldMatrixData(linearIndex, instanceTransformData);
			linearIndex++;
		}
	}

	return result;
}


/*cbuffer MaterialCB : register(b2)
{
	float4  baseColorFactor;        // RGBA (default 1,1,1,1)

	float3  emissiveFactor;         // default 0,0,0
	float   metallicFactor;         // default 1.0

	float   roughnessFactor;        // default 1.0
	float   normalScale;            // default 1.0
	float   occlusionStrength;      // default 1.0
	float   alphaCutoff;            // default 0.5 (for MASK mode)

	uint    flags;                  // bitmask (see below)
	float3  padding;                // pad to 16-byte boundary
};*/
VOID Dx12SampleBase::CreateSceneMaterialCb()
{
	

	const UINT materialSize        = sizeof(DxMaterialCB);
	const UINT alignedMaterialSize = dxhelper::DxAlign(materialSize, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

	///@todo simplify and store it in LoadGltf files
	UINT totalPrimitivesInScene    = 0;
	ForEachModelAsset([&totalPrimitivesInScene, this](UINT assetIdx)
	{
			totalPrimitivesInScene += NumPrimitivesInModelAsset(assetIdx);
	});

	const UINT alignedTotalMaterialBufferSize = alignedMaterialSize * totalPrimitivesInScene;


	///@todo avoid code duplication, same code used in camera constant buffer
	///@todo avoid using static
	static VOID* pMappedPtr = nullptr;
	static BYTE* pMappedBytePtr = nullptr;
	if (pMappedPtr == nullptr)
	{
		m_materialConstantBuffer = CreateBufferWithData(nullptr, alignedTotalMaterialBufferSize, "Material CB", D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COMMON, TRUE);
		CD3DX12_RANGE readRange(0, 0);
		//@note specifying nullptr as read range indicates CPU can read entire resource
		m_materialConstantBuffer->Map(0, &readRange, &pMappedPtr);
		pMappedBytePtr = static_cast<BYTE*>(pMappedPtr);

		D3D12_GPU_VIRTUAL_ADDRESS baseGpuVa = m_materialConstantBuffer->GetGPUVirtualAddress();


		DxPrimitiveInfo* firstPrim = nullptr;
		DxPrimitiveInfo* lastPrim = nullptr;
		UINT absolutePrimIndex = 0;
		D3D12_GPU_VIRTUAL_ADDRESS lastAddressWritten = 0;
		ForEachModelAssetPrimitive([this, baseGpuVa, alignedMaterialSize, alignedTotalMaterialBufferSize, &firstPrim, &lastPrim, &absolutePrimIndex, &lastAddressWritten](UINT assetIdx, UINT primIdx)
		{
			{
				auto& prim = GetPrimitiveInfo(assetIdx, primIdx);
				BYTE* pWritePtr = pMappedBytePtr + absolutePrimIndex * alignedMaterialSize;
				memcpy(pWritePtr, &prim.materialCbData, materialSize);
				if (firstPrim == nullptr)
				{
					firstPrim = &prim;
				}
				lastPrim = &prim;
				prim.materialTextures.meterialCb = baseGpuVa + alignedMaterialSize * absolutePrimIndex;
				assert(prim.materialTextures.meterialCb < baseGpuVa + alignedTotalMaterialBufferSize);
				lastAddressWritten = prim.materialTextures.meterialCb;
				absolutePrimIndex++;

			}
		});

		assert(firstPrim != nullptr && firstPrim->materialTextures.meterialCb == baseGpuVa);
		assert(lastPrim != nullptr && lastPrim->materialTextures.meterialCb == baseGpuVa + alignedTotalMaterialBufferSize - alignedMaterialSize);
		

	}


	assert(pMappedPtr != nullptr);
	assert(pMappedBytePtr == pMappedPtr);
}

UINT Dx12SampleBase::CreateInputElementDesc(const std::vector<DxPrimVertexData>& inData, std::vector<D3D12_INPUT_ELEMENT_DESC>& outData)
{
	const UINT numAttributes = inData.size();
	outData.resize(numAttributes);
	for (UINT idx = 0; idx < numAttributes; idx++)
	{
		const auto& inElementDesc = inData[idx].iaSemantic;
		auto& curSemantic = outData[idx];

		curSemantic.SemanticName         = inElementDesc.name.c_str();
		curSemantic.AlignedByteOffset    = 0;
		curSemantic.SemanticIndex        = (inElementDesc.isIndexValid == TRUE) ? inElementDesc.index : 0;
		curSemantic.InputSlotClass       = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		curSemantic.InstanceDataStepRate = 0;
		curSemantic.Format               = inElementDesc.format;
		curSemantic.InputSlot            = idx;

	}
	return numAttributes;
}

VOID Dx12SampleBase::LoadGltfFiles()
{
	const std::vector<std::string> gltfFileNames        = GltfFileName();
	const UINT                     numgltfFiles         = gltfFileNames.size();
	UINT                           globalPrimitiveIndex = 0;

	m_modelAssets.modelAssetList.clear();
	m_gltfLoader = std::make_unique<DxGltfLoader>(this);
	m_modelAssets.numTotalPrimitivesInAllAssets = 0;

	for (UINT fileIdx = 0; fileIdx < numgltfFiles; fileIdx++)
	{
		std::string modelPath = m_assetReader->GetFullModelFilePath(gltfFileNames[fileIdx]);

		m_gltfLoader->LoadModel(modelPath);

		m_modelAssets.modelAssetList.emplace_back();
		auto& currentAsset = m_modelAssets.modelAssetList.back();

		m_gltfLoader->LoadGltfModelAsset(currentAsset);
		m_modelAssets.numTotalPrimitivesInAllAssets += currentAsset.primitives.size();
	}
}

VOID Dx12SampleBase::LoadSceneMaterialInfo()
{
    UINT primitiveIndex = 0;

	ForEachModelAssetPrimitive([this, &primitiveIndex](UINT assetIdx, UINT primIdx)
	{
		{
			auto& currentPrim = GetPrimitiveInfo(assetIdx, primIdx);
			auto& primTextureInfo = currentPrim.materialTextures;

			CreateAppSrvDescriptorAtIndex(primitiveIndex, 0, primTextureInfo.pbrBaseColorTexture.textureInfo.Get());			//t0
			CreateAppSrvDescriptorAtIndex(primitiveIndex, 1, primTextureInfo.pbrMetallicRoughnessTexture.textureInfo.Get());	//t1
			CreateAppSrvDescriptorAtIndex(primitiveIndex, 2, primTextureInfo.normalTexture.textureInfo.Get());			    //t2
			CreateAppSrvDescriptorAtIndex(primitiveIndex, 3, primTextureInfo.occlusionTexture.textureInfo.Get());				//t3	
			CreateAppSrvDescriptorAtIndex(primitiveIndex, 4, primTextureInfo.emissiveTexture.textureInfo.Get());				//t4

			primitiveIndex++;
		}
	});
}

/*
* 
* 1. Add all the model transforms required for every instance of every scene element in Scene (later creates one CB and chunks it)
* 2. We now have a scene transform and model transform, need to combine, 
* 
*/
VOID Dx12SampleBase::LoadScene()
{
	const UINT numElementsInSceneDesc = m_sceneDescription.size();
	m_sceneInfo.sceneLoadInfo.clear();
	m_sceneInfo.sceneLoadInfo.resize(numElementsInSceneDesc);

	for (UINT idx = 0; idx < numElementsInSceneDesc; idx++)
	{
		const auto& currentSceneElementDesc = m_sceneDescription[idx];
		auto& curElementSceneLoadInfo       = m_sceneInfo.sceneLoadInfo[idx];
		const UINT numInstances             = currentSceneElementDesc.numInstances;
		const UINT modelAssetIdx            = currentSceneElementDesc.sceneElementIdx;

		curElementSceneLoadInfo.sceneElementIdx = modelAssetIdx;
		curElementSceneLoadInfo.numInstances = numInstances;


		auto& sceneElement = m_modelAssets.modelAssetList[modelAssetIdx];

		const UINT numPrimitives = NumPrimitivesInModelAsset(modelAssetIdx);
		IncrementPerInstanceDataCount(numInstances * numPrimitives);
		for (UINT instance = 0; instance < numInstances; instance++)
		{
			for (UINT primIdx = 0; primIdx < numPrimitives; primIdx++)
			{
				auto& curPrim = GetPrimitiveInfo(modelAssetIdx, primIdx);
				///@note transform into is added as instance1 (node1, node2), instance2(node1, node2), GPUVA is stored in same order
				m_camera->AddTransformInfo(curPrim.worldMatrix, &m_sceneDescription[idx].trsMatrix[instance]);
				if (currentSceneElementDesc.addToExtents == TRUE)
				{
					m_camera->AddMinMaxExtents(curPrim.primitiveExtents);
				}
			}
		}

	}
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

	m_pCommandAllocator->Reset();
	m_pCmdList->Reset(m_pCommandAllocator.Get(), nullptr);
}

VOID Dx12SampleBase::StartBuildingAccelerationStructures()
{
	//m_pCmdList->Reset(m_pCommandAllocator.Get(), nullptr);
}


