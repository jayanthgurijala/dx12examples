/*
* Copyright (C) 2026 by Jayanth Gurijala
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/
#pragma once

#include "dxtypes.h"
#include "stdafx.h"
#include <vector>
#include "FileReader.h"
#include <DirectXMath.h>
#include <d3dx12.h>
#include "DxCamera.h"
#include "DxGltfLoader.h"

using namespace DirectX;
using namespace Microsoft::WRL;



class Dx12SampleBase
{
public:
    Dx12SampleBase(UINT width, UINT height);
    ~Dx12SampleBase();
    HRESULT Initialize();
    VOID InitlializeDeviceCmdQueueAndCmdList();
    VOID InitializeRtvDsvDescHeaps();
    VOID InitializeSrvCbvUavDescHeaps();
    VOID InitializeImgui();

    virtual HRESULT OnInit() { return S_OK; };

    HRESULT NextFrame(FLOAT frameDeltaTime);
    virtual HRESULT RenderFrame() { return S_OK; };
    virtual HRESULT PostRun() { return S_OK; };
    HRESULT RenderRtvContentsOnScreen();
    
    FLOAT inline GetFrameDeltaTime() { return m_frameDeltaTime; }

    VOID SetupWindow(HINSTANCE hInstance, int nCmdShow);
    int RenderLoop();

    int RunApp(HINSTANCE hInstance, int nCmdShow);

    VOID RenderModel(ID3D12GraphicsCommandList* pCommandList, UINT sceneIdx, UINT nodeIndex = 0, UINT primitiveIndex = 0);


    static FLOAT s_frameDeltaTime;


protected:
    HRESULT CreateRenderTargetResources(UINT numResources);
    VOID CreateRenderTargetSRVs(UINT numSrvs);
    HRESULT CreateRenderTargetViews(UINT numRTVs, BOOL isInternal);
    HRESULT CreateDsvResources(UINT numResources, BOOL createViews = TRUE);
    D3D12_CPU_DESCRIPTOR_HANDLE GetRenderTargetView(UINT rtvIndex, BOOL isInternal);
    ComPtr<ID3D12PipelineState> GetGfxPipelineStateWithShaders(const std::string& vertexShaderName,
                                                               const std::string& pixelShaderName,
                                                               ID3D12RootSignature* signature,
                                                               const D3D12_INPUT_LAYOUT_DESC& iaLayout,
                                                               BOOL wireframe       = FALSE,
                                                               BOOL doubleSided     = FALSE,
                                                               BOOL useDepthStencil = FALSE,
                                                               BOOL enableBlend     = FALSE);
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
                                                const char* resourceName,
                                                D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE,
                                                D3D12_RESOURCE_STATES initialResourceState = D3D12_RESOURCE_STATE_COMMON,
                                                BOOL isUploadHeap = FALSE);

    ///@note gltf basecolor formats are sRGB
    ComPtr<ID3D12Resource> CreateTexture2DWithData(void* cpuData, SIZE_T sizeInBytes, UINT width, UINT height, DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM);

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

    inline CD3DX12_GPU_DESCRIPTOR_HANDLE GetAppUavGpuHandle(UINT index)
    {
        ///@todo make this more explicit
        UINT appSrvStartIndex = NumRTVsNeededForApp();
        return GetSrvGpuHeapHandle(appSrvStartIndex + index);
    }

    inline CD3DX12_GPU_DESCRIPTOR_HANDLE GetAppSrvGpuHandle(UINT index)
    {
        ///@todo make this more explicit
        UINT appSrvStartIndex = NumRTVsNeededForApp() + NumUAVsNeededForApp();
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
        std::string compiledVertexShaderPath = m_assetReader->GetFullCompiledShaderFilePath(shaderName);
        shaderBlob = m_assetReader->LoadShaderBlobFromAssets(compiledVertexShaderPath);
        return shaderBlob;
    }

    inline UINT GetRootConstantIndex() const
    {
        return 2;
    }

    inline UINT NumSceneElementsLoaded()
    {
        return m_sceneElements.size();
    }


    inline DxNodeInfo& GetNodeInfo(UINT sceneIdx, UINT nodeIndex)
    {
        return m_sceneElements[sceneIdx].nodes[nodeIndex];
    }

    inline DxMeshInfo& GetMeshInfo(UINT sceneIdx, UINT nodeIndex)
    {
        return GetNodeInfo(sceneIdx, nodeIndex).meshInfo; 
    }

    inline DxPrimitiveInfo& GetPrimitiveInfo(UINT sceneIdx, UINT nodeIndex, UINT primitiveIndex)
    {
        return GetMeshInfo(sceneIdx, nodeIndex).primitives[primitiveIndex];
    }

    inline BOOL IsPrimitiveTransparent(UINT sceneIdx, UINT nodeIndex, UINT primitiveIndex)
    {
        auto curPrim = GetPrimitiveInfo(sceneIdx, nodeIndex, primitiveIndex);
        BOOL isBlend = (((curPrim.materialCbData.flags & AlphaModeBlend) == 0) ? FALSE : TRUE);
        BOOL isAlphaMask = (((curPrim.materialCbData.flags & AlphaModeMask) == 0) ? FALSE : TRUE);
        BOOL isTransparent = (isBlend == TRUE || isAlphaMask == TRUE);
        return isTransparent;
    }

    inline DxPrimVertexData& GetPrimitiveVertexData(UINT sceneIdx, UINT nodeIndex, UINT primitiveIndex, UINT vbIndex)
    {
        return GetPrimitiveInfo(sceneIdx, nodeIndex, primitiveIndex).vertexBufferInfo[vbIndex];
    }

    inline DxPrimIndexData& GetPrimitiveIndexData(UINT sceneIdx, UINT nodeIndex, UINT primitiveIndex)
    {
        return GetPrimitiveInfo(sceneIdx, nodeIndex, primitiveIndex).indexBufferInfo;
    }

    inline D3D12_VERTEX_BUFFER_VIEW& const GetModelVertexBufferView(UINT sceneIdx, UINT nodeIndex, UINT primitiveIndex, UINT vbIndex)
    {
        auto& vbv = GetPrimitiveVertexData(sceneIdx, nodeIndex, primitiveIndex, vbIndex).modelVbv;
        assert(vbv.BufferLocation != 0);
        assert(vbv.SizeInBytes != 0);
        assert(vbv.StrideInBytes != 0);
        return vbv;
    }

    inline DxDrawPrimitive& GetModelDrawInfo(UINT sceneIdx, UINT nodeIndex, UINT primitiveIndex)
    {
        return GetPrimitiveInfo(sceneIdx, nodeIndex, primitiveIndex).modelDrawPrimitive;
    }

    inline ID3D12Resource* GetModelIndexBufferResource(UINT sceneIdx, UINT nodeIndex, UINT primitiveIndex)
    {
        return GetPrimitiveIndexData(sceneIdx, nodeIndex, primitiveIndex).indexBuffer.Get();
    }

    inline D3D12_INDEX_BUFFER_VIEW& GetModelIndexBufferView(UINT sceneIdx, UINT nodeIndex, UINT primitiveIndex)
    {
        return  GetPrimitiveIndexData(sceneIdx, nodeIndex, primitiveIndex).modelIbv;
    }

    inline UINT NumVertexAttributesInPrimitive(UINT sceneIdx, UINT nodeIndex, UINT primitiveIndex)
    {
        return static_cast<UINT>(GetPrimitiveInfo(sceneIdx, nodeIndex, primitiveIndex).vertexBufferInfo.size());
    }


    ///@todo Assumptions, POSITION, NORMAL, TEXCOORD0, TEXCOORD1 etc as per semantic order in gltfloader
    inline D3D12_VERTEX_BUFFER_VIEW& GetModelPositionVertexBufferView(UINT sceneIdx, UINT nodeIndex, UINT primitiveIndex)
    {
        return GetPrimitiveVertexData(sceneIdx, nodeIndex, primitiveIndex, 0).modelVbv;
    }

    inline ID3D12Resource* GetModelPositionVertexBufferResource(UINT sceneIdx, UINT nodeIndex, UINT primitiveIndex)
    {
        return GetPrimitiveVertexData(sceneIdx, nodeIndex, primitiveIndex, 0).modelVbBuffer.Get();
    }

    inline ID3D12Resource* GetModelUvVertexBufferResource(UINT sceneIdx, UINT nodeIndex, UINT primitiveIndex, UINT texCoordIndex)
    {
        return GetPrimitiveVertexData(sceneIdx, nodeIndex, primitiveIndex, texCoordIndex + 2).modelVbBuffer.Get();
    }

    inline D3D12_VERTEX_BUFFER_VIEW& GetModelUvBufferView(UINT sceneIdx, UINT nodeIndex, UINT primitiveIndex, UINT texCoordIndex)
    {
        //@note POSITION = 0, NORMAL = 1, TEXCOORD0 = 2, TEXCOORD1 = 3 and so on as per gltfloader semantic order
        return GetPrimitiveVertexData(sceneIdx, nodeIndex, primitiveIndex, texCoordIndex + 2).modelVbv;
    }

    inline DxDrawPrimitive& GetDrawInfo(UINT sceneIdx, UINT nodeIndex, UINT primitiveIndex)
    {
        return m_sceneElements[sceneIdx].nodes[nodeIndex].meshInfo.primitives[0].modelDrawPrimitive;
    }

    inline DXGI_FORMAT GetVertexPositionBufferFormat(UINT sceneIdx, UINT nodeIndex, UINT primitiveIndex)
    {
        ///@note POSITION is always the first semantic in the list as per gltfloader, this is an assumption we are making here
        auto& vertexAttribute = GetPrimitiveInfo(sceneIdx, nodeIndex, primitiveIndex).vertexBufferInfo[0].iaSemantic;

        assert(vertexAttribute.name == "POSITION");

        return vertexAttribute.format;
    }

    inline VOID SetFrameInfo(ID3D12Resource* frameResource = nullptr, UINT rtvIndex = UINT_MAX, D3D12_RESOURCE_STATES resState = D3D12_RESOURCE_STATE_RENDER_TARGET)
    {
        m_appFrameInfo.pResState = resState;
        m_appFrameInfo.pFrameResource = frameResource;
        m_appFrameInfo.rtvIndex = rtvIndex;
        m_appFrameInfo.type = (frameResource != nullptr) ? DxAppFrameType::DxFrameResource : DxAppFrameType::DxFrameRTVIndex;
    }

    inline UINT NumNodesInScene(UINT sceneIdx)
    {
        return m_sceneElements[sceneIdx].nodes.size();
    }

    inline UINT NumPrimitivesInNodeMesh(UINT sceneIdx, UINT nodeIdx)
    {
        return GetMeshInfo(sceneIdx, nodeIdx).primitives.size();
    }

    inline UINT NumSRVsPerPrimForMaterials()
    {
        return 5;
    }

    inline UINT NumSRVsPerPrimitive()
    {
        ///@note based on PBR shading
        return NumSRVsPerPrimForMaterials() + NumSRVsPerPrimNeededForApp();
    }

    inline UINT NumPrimsInScene(UINT sceneIdx)
    {
        return m_sceneElements[sceneIdx].numTotalPrimitivesInScene;
    }

    inline UINT NumSRVsInScene(UINT sceneIdx)
    {
        assert(UINT_MAX / NumSRVsPerPrimitive() > NumPrimsInScene(sceneIdx));
        const UINT numPrimsInScene = NumPrimsInScene(sceneIdx);
        const UINT numSrvsPerPrimitive = NumSRVsPerPrimitive();
        const UINT totalSrvs = numPrimsInScene * numSrvsPerPrimitive;
        return totalSrvs;
    }

    inline UINT AppSrvOffsetForPrim()
    {
        return NumSRVsPerPrimForMaterials();
    }

    virtual inline DxCamera* GetCamera() { return m_camera.get(); }
    virtual inline DXGI_FORMAT GetBackBufferFormat() { return DXGI_FORMAT_R8G8B8A8_UNORM; }
    virtual inline DXGI_FORMAT GetDepthStencilFormat() { return DXGI_FORMAT_D32_FLOAT; }

    virtual inline UINT NumRTVsNeededForApp()         { return 0; }
    virtual inline UINT NumSRVsPerPrimNeededForApp()  { return 0; }
    virtual inline UINT NumDSVsNeededForApp()         { return 0; }
    virtual inline UINT NumUAVsNeededForApp()         { return 0; }
    virtual inline UINT NumRootConstantsForApp()      { return 0; }
    virtual inline UINT NumRootSrvDescriptorsForApp() { return 0; }
    virtual ID3D12RootSignature* GetRootSignature() { return nullptr; }
    virtual inline const std::vector<std::string> GltfFileName() { return { "triangle.gltf" }; }
    virtual inline std::array<FLOAT, 4> RenderTargetClearColor() { return{ 0.2f, 0.2f, 0.2f, 1.0f }; }

    virtual inline void GetVertexShaderName(char* outVsNameString, SIZE_T numVertexAttributes)
    {
        snprintf(outVsNameString, 64, "Simple%zu_VS.cso", numVertexAttributes);
    }

    virtual inline void GetPixelShaderName(char* outPsNameString, SIZE_T numVertexAttributes)
    {
        snprintf(outPsNameString, 64, "Simple%zu_PS.cso", numVertexAttributes);
    }

    template<typename Func>
    void ForEachSceneNode(Func&& func)
    {
        const UINT numSceneElements = NumSceneElementsLoaded();

        for (UINT idx = 0; idx < numSceneElements; idx++)
        {
            const UINT numNodesInScene = NumNodesInScene(idx);

            for (UINT nodeIdx = 0; nodeIdx < numNodesInScene; nodeIdx++)
            {
                func(idx, nodeIdx);
            }
        }
    }

    virtual VOID LoadSceneDescription(std::vector< DxSceneElementInstance>& sceneDescription);


    VOID CreateAppSrvDescriptorAtIndex(UINT appSrvIndex, ID3D12Resource* srvResource);
    VOID CreateAppUavDescriptorAtIndex(UINT appUavIndex, ID3D12Resource* uavResource);
    VOID CreateAppBufferSrvDescriptorAtIndex(UINT appSrvIndex, ID3D12Resource* srvResource, UINT numElements, UINT elementSize);

    static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

    VOID LoadGltfFiles();
    VOID LoadSceneMaterialInfo();
    VOID LoadScene();
    HRESULT CreateVSPSPipelineStateFromModel();

    VOID StartBuildingAccelerationStructures();
    VOID ExecuteBuildAccelerationStructures();

    HRESULT CreateSceneMVPMatrix();
    VOID CreateSceneMaterialCb();

    UINT CreateInputElementDesc(const std::vector<DxPrimVertexData>& inData, std::vector<D3D12_INPUT_ELEMENT_DESC>& outData);

private:

    inline ID3D12CommandQueue* GetCommandQueue() { return m_pCmdQueue.Get(); }

    inline VOID SetNumTotalPrimitivesInScene(UINT numPrims, UINT sceneIdx)
    {
        m_sceneElements[sceneIdx].numTotalPrimitivesInScene = numPrims;
    }

    struct FrameComposition
    {
        ComPtr<ID3D12RootSignature> rootSignature;
        ComPtr<ID3D12PipelineState> pipelineState;
        ComPtr<ID3D12Resource>      vertexBuffer;
        D3D12_VERTEX_BUFFER_VIEW    vertexBufferView;
    };

    struct ComputeGenerateMips
    {
        ComPtr<ID3D12RootSignature> rootSignature;
        ComPtr<ID3D12PipelineState> pipelineState;
        ComPtr<ID3D12Heap>          srvUavCbvHeap;

    };

    HRESULT CreateDevice();
    HRESULT CreateCommandQueues();
    HRESULT CreateSwapChain();
    HRESULT CreateCommandList();

    HRESULT InitializeFrameComposition();
    VOID InitializeMipGenerationPipeline();
    VOID GenerateMipLevels(ID3D12Resource* tex2D, UINT width, UINT height);
    HRESULT CreateFence();

    ///@todo refactor this, maybe use single function
    HRESULT CreateRenderTargetDescriptorHeap(UINT numDescriptors);
    HRESULT CreateSrvUavCbvDescriptorHeap(UINT numDescriptors);
    HRESULT CreateDepthStencilViewDescriptorHeap(UINT numDescriptors);
    VOID Imgui_CreateDescriptorHeap();
    UINT    GetBackBufferCount();

    UINT                       m_width;
    UINT                       m_height;

    ComPtr<IDXGIFactory7>      m_dxgiFactory7;    ///< OS graphics infracture e.g. creation of swapchain
    ComPtr<ID3D12Device>       m_pDevice;
    ComPtr<ID3D12CommandQueue> m_pCmdQueue;
    ComPtr<ID3D12CommandQueue> m_pComputeCmdQueue;
    ComPtr<IDXGISwapChain4>    m_swapChain4;

    ComPtr<ID3D12DescriptorHeap>        m_rtvDescHeap;
    ComPtr<ID3D12DescriptorHeap>        m_dsvDescHeap;
    ComPtr<ID3D12DescriptorHeap>        m_srvUavCbvDescHeap;
    ComPtr<ID3D12CommandAllocator>      m_pCommandAllocator;
    ComPtr<ID3D12GraphicsCommandList>   m_pCmdList;

    ComPtr<ID3D12CommandAllocator>    m_pComputeCmdAllocator;
    ComPtr<ID3D12GraphicsCommandList> m_pComputeCommandList;

    std::vector<ComPtr<ID3D12Resource>> m_swapChainBuffers;
    ///@todo created on application basis, kept here because most samples need it
    std::vector<ComPtr<ID3D12Resource>> m_rtvResources;
    std::vector<ComPtr<ID3D12Resource>> m_dsvResources;

    std::unique_ptr<DxCamera>	m_camera;
    std::unique_ptr<FileReader>	m_assetReader;

    FrameComposition    m_simpleComposition;
    ComputeGenerateMips m_computeGenerateMips;

    ComPtr<ID3D12Resource>      m_mvpCameraConstantBuffer;
    ComPtr<ID3D12Resource>      m_materialConstantBuffer;

    ///@todo associate fences with command queues?
    ComPtr<ID3D12Fence> m_fence;
    UINT64              m_fenceValue;
    HANDLE              m_fenceEvent;
    FLOAT               m_frameDeltaTime;

    UINT m_srvUavCbvDescriptorSize;
    UINT m_samplerDescriptorSize;
    UINT m_rtvDescriptorSize;
    UINT m_dsvDescriptorSize;

    HWND m_hwnd;

    ComPtr<ID3D12DescriptorHeap>   m_imguiDescHeap;
    DxAppFrameInfo                 m_appFrameInfo;
    std::unique_ptr<DxGltfLoader>  m_gltfLoader;

    std::vector<DxSceneElements>	    m_sceneElements;
    static 	Dx12SampleBase*             s_sampleBase;

    std::vector<DxSceneElementInstance> m_sceneDescription;
};


