#pragma once

#include "dxtypes.h"
#include "stdafx.h"
#include <vector>
#include "FileReader.h"
#include <DirectXMath.h>
#include <d3dx12.h>
#include "DxCamera.h"

using namespace DirectX;
using namespace Microsoft::WRL;

class Dx12SampleBase
{
public:
	Dx12SampleBase(UINT width, UINT height);
	~Dx12SampleBase();
	virtual HRESULT OnInit();
	HRESULT NextFrame(FLOAT frameDeltaTime);
	virtual HRESULT RenderFrame() { return S_OK; };
	virtual HRESULT PostRun() { return S_OK; };
	HRESULT RenderRtvContentsOnScreen(UINT rtvResIndex);
	
	FLOAT inline GetFrameDeltaTime() { return m_frameDeltaTime; }

	VOID SetupWindow(HINSTANCE hInstance, int nCmdShow);
	int RenderLoop();

	int RunApp(HINSTANCE hInstance, int nCmdShow);

	VOID RenderModel(ID3D12GraphicsCommandList* pCommandList);


protected:
	DxMeshState m_meshState;
	ComPtr<ID3D12RootSignature> m_globalRootSignature;
	ComPtr<ID3D12PipelineState> m_modelPipelineState;

	HRESULT CreateRenderTargetResourceAndSRVs(UINT numResources);
	HRESULT CreateRenderTargetViews(UINT numRTVs, BOOL isInternal);
	HRESULT CreateDsvResources(UINT numResources, BOOL createViews = TRUE);
	D3D12_CPU_DESCRIPTOR_HANDLE GetRenderTargetView(UINT rtvIndex, BOOL isInternal);
	ComPtr<ID3D12PipelineState> GetGfxPipelineStateWithShaders(const std::string& vertexShaderName,
		const std::string& pixelShaderName,
		ID3D12RootSignature* signature,
		const D3D12_INPUT_LAYOUT_DESC& iaLayout,
		BOOL wireframe = FALSE,
		BOOL doubleSided = FALSE,
		BOOL useDepthStencil = FALSE);
	HRESULT UploadCpuDataAndWaitForCompletion(void* cpuData,
		UINT                       dataSizeInBytes,
		ID3D12GraphicsCommandList* pcmdList,
		ID3D12CommandQueue* pcmdQueue,
		ID3D12Resource* pDstResource,
		D3D12_RESOURCE_STATES      dstStateBefore,
		D3D12_RESOURCE_STATES      dstStateAfter);
	HRESULT WaitForFenceCompletion(ID3D12CommandQueue* pCmdQueue);
	ComPtr<ID3D12Resource> CreateBufferWithData(void* cpuData,
											    UINT sizeInBytes,
											    const wchar_t* resourceName,
											    D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE,
											    D3D12_RESOURCE_STATES initialResourceState = D3D12_RESOURCE_STATE_COMMON,
											    BOOL isUploadHeap = FALSE);

	///@note gltf basecolor formats are sRGB
	ComPtr<ID3D12Resource> CreateTexture2DWithData(void* cpuData, SIZE_T sizeInBytes, UINT width, UINT height, DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);

	inline ID3D12Device* GetDevice() { return m_pDevice.Get(); }
	inline ID3D12GraphicsCommandList* GetCmdList() { return m_pCmdList.Get(); }
	inline UINT                       GetAppRTVStartIndex() { return GetBackBufferCount(); }
	inline UINT                       GetWidth() { return m_width; }
	inline UINT                       GetHeight() { return m_height; }

	inline ID3D12DescriptorHeap* GetSrvDescriptorHeap()
	{
		return m_srvUavCbvDescHeap.Get();
	}

	template<typename HandleType>
	inline VOID	OffsetHandle(HandleType& handle, UINT index, UINT descriptorSize)
	{
		assert(descriptorSize != 0);
		handle.Offset(index, descriptorSize);
	}

	inline CD3DX12_CPU_DESCRIPTOR_HANDLE GetSrvUavCBvCpuHeapHandle(UINT index)
	{
		auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_srvUavCbvDescHeap->GetCPUDescriptorHandleForHeapStart());
		OffsetHandle<CD3DX12_CPU_DESCRIPTOR_HANDLE>(handle, index, m_srvUavCbvDescriptorSize);

		return handle;
	}

	inline CD3DX12_GPU_DESCRIPTOR_HANDLE GetSrvGpuHeapHandle(UINT index)
	{
		auto handle = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_srvUavCbvDescHeap->GetGPUDescriptorHandleForHeapStart());
		OffsetHandle<CD3DX12_GPU_DESCRIPTOR_HANDLE>(handle, index, m_srvUavCbvDescriptorSize);
		return handle;
	}

	inline CD3DX12_GPU_DESCRIPTOR_HANDLE GetAppSrvGpuHandle(UINT index)
	{
		///@todo make this more explicit
		UINT appSrvStartIndex = NumRTVsNeededForApp();
		return GetSrvGpuHeapHandle(appSrvStartIndex + index);
	}

	///@todo remove this duplication by using arrays or something
	inline CD3DX12_CPU_DESCRIPTOR_HANDLE GetRtvCpuHeapHandle(UINT index)
	{
		auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_rtvDescHeap->GetCPUDescriptorHandleForHeapStart());
		OffsetHandle< CD3DX12_CPU_DESCRIPTOR_HANDLE>(handle, index, m_rtvDescriptorSize);
		return handle;
	}

	inline CD3DX12_CPU_DESCRIPTOR_HANDLE GetDsvCpuHeapHandle(UINT index)
	{
		auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_dsvDescHeap->GetCPUDescriptorHandleForHeapStart());
		OffsetHandle< CD3DX12_CPU_DESCRIPTOR_HANDLE>(handle, index, m_dsvDescriptorSize);
		return handle;
	}

	inline D3D12_GPU_VIRTUAL_ADDRESS GetCameraBuffer()
	{
		return m_mvpCameraConstantBuffer->GetGPUVirtualAddress();
	}

	inline ComPtr<ID3DBlob> GetCompiledShaderBlob(const std::string& shaderName)
	{
		ComPtr<ID3DBlob> shaderBlob;
		std::string compiledVertexShaderPath = m_assetReader->GetFullAssetFilePath(shaderName);
		shaderBlob = m_assetReader->LoadShaderBlobFromAssets(compiledVertexShaderPath);
		return shaderBlob;
	}

	inline UINT GetRootConstantIndex() const
	{
		return 2;
	}

	inline D3D12_INDEX_BUFFER_VIEW& GetModelIndexBufferView(UINT index)
	{
		return m_modelIbv;
	}

	inline D3D12_VERTEX_BUFFER_VIEW& GetModelVertexBufferView(UINT index)
	{
		return m_modelVbvs[m_modelPositionVbIndex];
	}

	inline DxDrawPrimitive& GetDrawInfo(UINT index)
	{
		return m_modelDrawPrimitive;
	}

	inline DXGI_FORMAT GetVertexBufferFormat(UINT index)
	{
		auto& positionInfo = m_modelIaSemantics[m_modelPositionVbIndex];

		assert(positionInfo.name == "POSITION");

		return positionInfo.format;
	}

	virtual inline DXGI_FORMAT GetBackBufferFormat() { return DXGI_FORMAT_R8G8B8A8_UNORM; }
	virtual inline DXGI_FORMAT GetDepthStencilFormat() { return DXGI_FORMAT_D32_FLOAT; }

	virtual inline UINT NumRTVsNeededForApp()         { return 0; }
	virtual inline UINT NumSRVsNeededForApp()         { return 0; }
	virtual inline UINT NumDSVsNeededForApp()         { return 0; }
	virtual inline UINT NumUAVsNeededForApp()         { return 0; }
	virtual inline UINT NumRootConstantsForApp()      { return 0; }
	virtual inline UINT NumRootSrvDescriptorsForApp() { return 0; }

	virtual inline const std::string GltfFileName() { return "triangle.gltf"; }
	virtual inline VOID AppSetRootConstantForModel(ID3D12GraphicsCommandList* pCmdList, UINT rootParamIndex) { assert(NumRootConstantsForApp() == 0); };

	VOID CreateAppSrvDescriptorAtIndex(UINT appSrvIndex, ID3D12Resource* srvResource);
	VOID CreateAppUavDescriptorAtIndex(UINT appUavIndex, ID3D12Resource* uavResource);

	VOID AddTransformInfo(const DxMeshNodeTransformInfo& transformInfo);

	static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	HRESULT LoadGltfFile();
	virtual HRESULT CreatePipelineStateFromModel();

	VOID StartBuildingAccelerationStructures();
	VOID ExecuteBuildAccelerationStructures();

private:

	inline ID3D12CommandQueue* GetCommandQueue() { return m_pCmdQueue.Get(); }

	struct FrameComposition
	{
		ComPtr<ID3D12RootSignature> rootSignature;
		ComPtr<ID3D12PipelineState> pipelineState;
		ComPtr<ID3D12Resource>      vertexBuffer;
		ComPtr<ID3D12Heap>          rtvHeap;
		D3D12_VERTEX_BUFFER_VIEW    vertexBufferView;
	};

	HRESULT CreateDevice();
	HRESULT CreateCommandQueues();
	HRESULT CreateSwapChain();
	HRESULT CreateCommandList();

	HRESULT InitializeFrameComposition();
	HRESULT CreateFence();

	///@todo refactor this, maybe use single function
	HRESULT CreateRenderTargetDescriptorHeap(UINT numDescriptors);
	HRESULT CreateShaderResourceViewDescriptorHeap(UINT numDescriptors);
	HRESULT CreateDepthStencilViewDescriptorHeap(UINT numDescriptors);
	VOID Imgui_CreateDescriptorHeap();
	HRESULT CreateSceneMVPMatrix();
	UINT    GetBackBufferCount();
	VOID CreateMeshState();
	VOID CreateRootSignature();

	UINT                       m_width;
	UINT                       m_height;

	ComPtr<IDXGIFactory7>      m_dxgiFactory7;    ///< OS graphics infracture e.g. creation of swapchain
	ComPtr<ID3D12Device>       m_pDevice;
	ComPtr<ID3D12CommandQueue> m_pCmdQueue;
	ComPtr<IDXGISwapChain4>    m_swapChain4;

	ComPtr<ID3D12DescriptorHeap>        m_rtvDescHeap;
	ComPtr<ID3D12DescriptorHeap>        m_dsvDescHeap;
	ComPtr<ID3D12DescriptorHeap>        m_srvUavCbvDescHeap;
	ComPtr<ID3D12CommandAllocator>      m_pCommandAllocator;
	ComPtr<ID3D12GraphicsCommandList>   m_pCmdList;

	std::vector<ComPtr<ID3D12Resource>> m_swapChainBuffers;
	///@todo created on application basis, kept here because most samples need it
	std::vector<ComPtr<ID3D12Resource>> m_rtvResources;
	std::vector<ComPtr<ID3D12Resource>> m_dsvResources;

	std::unique_ptr<DxCamera>	m_camera;
	std::unique_ptr<FileReader>	m_assetReader;

	FrameComposition			m_simpleComposition;
	ComPtr<ID3D12Resource>      m_mvpCameraConstantBuffer;

	///@todo associate fences with command queues?
	ComPtr<ID3D12Fence>         m_fence;
	UINT64                      m_fenceValue;
	HANDLE                      m_fenceEvent;
	FLOAT						m_frameDeltaTime;

	UINT m_srvUavCbvDescriptorSize;
	UINT m_samplerDescriptorSize;
	UINT m_rtvDescriptorSize;
	UINT m_dsvDescriptorSize;

	HWND m_hwnd;

	///@todo model resources refactor
	std::vector<ComPtr<ID3D12Resource>>   m_modelVbBuffers;
	ComPtr<ID3D12Resource>                m_modelIbBuffer;
	std::vector<D3D12_VERTEX_BUFFER_VIEW> m_modelVbvs;
	D3D12_INDEX_BUFFER_VIEW               m_modelIbv;
	std::vector<DxIASemantic>             m_modelIaSemantics;
	UINT                                  m_modelPositionVbIndex;

	DxDrawPrimitive                       m_modelDrawPrimitive;
	ComPtr<ID3D12Resource>                m_modelBaseColorTex2D;

	ComPtr<ID3D12DescriptorHeap>          m_imguiDescHeap;
};

