#pragma once

#include "stdafx.h"
#include <vector>
#include <d3d12.h>
#include "FileReader.h"
#include <DirectXMath.h>
#include <d3dx12.h>

using namespace DirectX;

using namespace Microsoft::WRL;


struct SimpleVertex
{
	FLOAT position[3];
	FLOAT color[3];
	FLOAT texCoords[2];
};

struct DxExtents
{
	double min[3];
	double max[3];
	BOOL hasValidExtents;
};

struct DxMeshNodeTransformInfo
{
	BOOL hasTranslation;
	BOOL hasRotation;
	BOOL hasScale;
	BOOL hasMatrix;

	std::vector<double> translation;
	std::vector<double> rotation;
	std::vector<double> scale;
	std::vector<double> matrix;
};

struct DxIASemantic
{
	std::string name;
	UINT index;
	BOOL isIndexValid;
	DXGI_FORMAT format;
};

struct DxDrawPrimitive
{
	UINT numVertices;
	UINT numIndices;
	BOOL isIndexedDraw;
};

class Dx12SampleBase
{
public:
	Dx12SampleBase(UINT width, UINT height);
	HRESULT OnInit(HWND hwnd);
	virtual HRESULT PreRun()  { return S_OK; };
	HRESULT Run(float frameDeltaTime);
	virtual HRESULT RenderFrame() { return S_OK; };
	virtual HRESULT PostRun() { return S_OK; };
	FLOAT m_aspectRatio;
	HRESULT RenderRtvContentsOnScreen(UINT rtvResIndex);
	VOID GetInputLayoutDesc_Layout1(D3D12_INPUT_LAYOUT_DESC& layout1);
	FLOAT inline GetFrameDeltaTime() { return m_frameDeltaTime; }


protected:
	HRESULT CreateRenderTargetResourceAndSRVs(UINT numResources);
	HRESULT CreateRenderTargetViews(UINT numRTVs, BOOL isInternal);
	D3D12_CPU_DESCRIPTOR_HANDLE GetRenderTargetView(UINT rtvIndex, BOOL isInternal);
	ComPtr<ID3D12PipelineState> GetGfxPipelineStateWithShaders(const std::string& vertexShaderName,
															   const std::string& pixelShaderName,
															   ID3D12RootSignature* signature,
		                                                       const D3D12_INPUT_LAYOUT_DESC& iaLayout,
														       BOOL wireframe = FALSE);
	HRESULT UploadCpuDataAndWaitForCompletion(void*                      cpuData,
		                                      UINT                       dataSizeInBytes,
		                                      ID3D12GraphicsCommandList* pcmdList,
		                                      ID3D12CommandQueue*        pcmdQueue,
											  ID3D12Resource*            pDstResource,
											  D3D12_RESOURCE_STATES      dstStateBefore,
		                                      D3D12_RESOURCE_STATES      dstStateAfter);
	HRESULT WaitForFenceCompletion(ID3D12CommandQueue* pCmdQueue);
	ComPtr<ID3D12Resource> CreateBufferWithData(void* cpuData, UINT sizeInBytes, BOOL isUploadHeap = FALSE);

	///@note gltf basecolor formats are sRGB
	ComPtr<ID3D12Resource> CreateTexture2DWithData(void* cpuData, SIZE_T sizeInBytes, UINT width, UINT height, DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);

	inline ID3D12Device*              GetDevice()           { return m_pDevice.Get();           }
	inline ID3D12GraphicsCommandList* GetCmdList()          { return m_pCmdList.Get();          }
	inline UINT                       GetAppRTVStartIndex() { return GetSwapChainBufferCount(); }
	inline UINT                       GetWidth()            { return m_width;}
	inline UINT                       GetHeight()           { return m_height;}

	inline ID3D12DescriptorHeap* GetSrvDescriptorHeap()
	{
		return m_srvDescHeap.Get();
	}

	template<typename HandleType>
	inline VOID	OffsetHandle(HandleType& handle, UINT index, UINT descriptorSize)
	{
		assert(descriptorSize != 0);
		handle.Offset(index, descriptorSize);
	}

	inline CD3DX12_CPU_DESCRIPTOR_HANDLE GetSrvCpuHeapHandle(UINT index)
	{
		auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_srvDescHeap->GetCPUDescriptorHandleForHeapStart());
		OffsetHandle<CD3DX12_CPU_DESCRIPTOR_HANDLE>(handle, index, m_srvUavCbvDescriptorSize);

		return handle;
	}

	inline CD3DX12_GPU_DESCRIPTOR_HANDLE GetSrvGpuHeapHandle(UINT index)
	{
		auto handle = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_srvDescHeap->GetGPUDescriptorHandleForHeapStart());
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

	virtual DXGI_FORMAT GetBackBufferFormat();
	virtual inline UINT NumRTVsNeededForApp() { return 0; }
	virtual inline UINT NumSRVsNeededForApp() { return 0; }

	XMMATRIX GetModelMatrix(const DxMeshNodeTransformInfo& transformInfo);
	XMMATRIX GetViewProjMatrix(XMVECTOR minExtent, XMVECTOR maxExtent);

	VOID CreateAppSrvAtIndex(UINT appSrvIndex, ID3D12Resource* srvResource);

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
	HRESULT CreateSwapChain(HWND hWnd);
	HRESULT CreateCommandList();

	HRESULT InitializeFrameComposition();
	HRESULT CreateFence();
	HRESULT CreateRenderTargetDescriptorHeap(UINT numDescriptors);
	HRESULT CreateShaderResourceViewDescriptorHeap(UINT numDescriptors);
	UINT    GetSwapChainBufferCount();

	UINT                       m_width;
	UINT                       m_height;

	ComPtr<IDXGIFactory7>      m_dxgiFactory7;    ///< OS graphics infracture e.g. creation of swapchain
	ComPtr<ID3D12Device>       m_pDevice;
	ComPtr<ID3D12CommandQueue> m_pCmdQueue;
	ComPtr<IDXGISwapChain4>    m_swapChain4;

	ComPtr<ID3D12DescriptorHeap>        m_rtvDescHeap;
	ComPtr<ID3D12DescriptorHeap>        m_srvDescHeap;
	ComPtr<ID3D12CommandAllocator>      m_pCommandAllocator;
	ComPtr<ID3D12GraphicsCommandList>   m_pCmdList;

	std::vector<ComPtr<ID3D12Resource>> m_swapChainBuffers;
	///@todo created on application basis, kept here because most samples need it
	std::vector<ComPtr<ID3D12Resource>> m_rtvResources;


	FileReader                          m_assetReader;
	FrameComposition                    m_simpleComposition;

	///@todo associate fences with command queues?
	ComPtr<ID3D12Fence>         m_fence;
	UINT64                      m_fenceValue;
	HANDLE                      m_fenceEvent;
	FLOAT						m_frameDeltaTime;

	UINT m_srvUavCbvDescriptorSize;
	UINT m_samplerDescriptorSize;
	UINT m_rtvDescriptorSize;
	UINT m_dsvDescriptorSize;
};

