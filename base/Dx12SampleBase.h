#pragma once

#include "stdafx.h"
#include <vector>
#include <d3d12.h>
#include "FileReader.h"
#include <DirectXMath.h>

using namespace DirectX;

using namespace Microsoft::WRL;


struct SimpleVertex
{
	FLOAT position[3];
	FLOAT color[3];
	FLOAT texCoords[2];
};

class Dx12SampleBase
{
public:
	Dx12SampleBase(UINT width, UINT height);
	HRESULT OnInit(HWND hwnd);
	virtual HRESULT PreRun()  { return S_OK; };
	HRESULT Run();
	virtual HRESULT RenderFrame() { return S_OK; };
	virtual HRESULT PostRun() { return S_OK; };
	FLOAT m_aspectRatio;
	HRESULT RenderRtvContentsOnScreen(UINT rtvResIndex);
	XMMATRIX GetMVPMatrix(XMMATRIX& modelMatrix);

	VOID GetInputLayoutDesc_Layout1(D3D12_INPUT_LAYOUT_DESC& layout1);


protected:
	HRESULT CreateRenderTargetResourceAndSRVs(UINT numResources);
	HRESULT CreateRenderTargetViews(UINT numRTVs, BOOL isInternal);
	D3D12_CPU_DESCRIPTOR_HANDLE GetRenderTargetView(UINT rtvIndex, BOOL isInternal);
	ComPtr<ID3D12PipelineState> GetGfxPipelineStateWithShaders(const LPCWSTR vertexShaderName,
															   const LPCWSTR pixelShaderName,
															   ID3D12RootSignature* signature,
		                                                       const D3D12_INPUT_LAYOUT_DESC& iaLayout);
	HRESULT UploadCpuDataAndWaitForCompletion(void*                      cpuData,
		                                      UINT                       dataSizeInBytes,
		                                      ID3D12GraphicsCommandList* pcmdList,
		                                      ID3D12CommandQueue*        pcmdQueue,
											  ID3D12Resource*            pDstResource,
											  D3D12_RESOURCE_STATES      dstStateBefore,
		                                      D3D12_RESOURCE_STATES      dstStateAfter);
	HRESULT WaitForFenceCompletion(ID3D12CommandQueue* pCmdQueue);
	ComPtr<ID3D12Resource> CreateBufferWithData(void* cpuData, UINT sizeInBytes);

	inline ID3D12Device*              GetDevice()           { return m_pDevice.Get();           }
	inline ID3D12GraphicsCommandList* GetCmdList()          { return m_pCmdList.Get();          }
	inline UINT                       GetAppRTVStartIndex() { return GetSwapChainBufferCount(); }
	virtual DXGI_FORMAT GetBackBufferFormat();
	virtual inline UINT NumRTVsNeededForApp() { return 0; }

	///@todo Use tiny gltf util class
	DXGI_FORMAT GltfGetDxgiFormat(int tinyGltfComponentType, int components);
	DXGI_FORMAT GetDxgiFloatFormat(int numComponents);
	DXGI_FORMAT GetDxgiUnsignedShortFormat(int numComponents);

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
};

