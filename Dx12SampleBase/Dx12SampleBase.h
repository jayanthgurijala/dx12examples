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
#include "CameraLightsMaterialsBuffer.h"
#include "DxDescriptorHeapManager.h"
#include "dxhelper.h"


using namespace DirectX;
using namespace Microsoft::WRL;



class Dx12SampleBase
{
public:
    Dx12SampleBase(UINT width, UINT height);
    ~Dx12SampleBase();
    HRESULT Initialize();
    VOID InitlializeDeviceCmdQueueAndCmdList();
    VOID InitializeDescriptorHeapManagerResourcesAndDescriptors();
    VOID InitializeImgui();

    virtual VOID OnInit() { };

    HRESULT NextFrame(FLOAT frameDeltaTime);

    virtual VOID RenderFrameGfxDraw();

    /******************************************************/
    VOID RenderGfxDrawInit(ID3D12RootSignature* rootSignature, UINT viewProjIndex);
    //virtual VOID RenderGfxDrawSetupRtvDsv() = 0;
    VOID RenderSceneGfxDraw();
    /*****************************************************/

    virtual VOID RenderFrame() { };
    virtual HRESULT PostRun() { return S_OK; };
    VOID RenderRtvContentsOnScreen();
    VOID ComposeSrvContents(FLOAT x, FLOAT y, FLOAT width, FLOAT height, DxAppFrameInfo& appFrameInfo);
    
    FLOAT inline GetFrameDeltaTime() { return m_frameDeltaTime; }

    VOID SetupWindow(HINSTANCE hInstance, int nCmdShow);
    int RenderLoop();

    int RunApp(HINSTANCE hInstance, int nCmdShow);

    VOID RenderModel(ID3D12GraphicsCommandList* pCommandList, UINT sceneIdx, UINT nodeIndex = 0, UINT primitiveIndex = 0);


    static FLOAT s_frameDeltaTime;


protected:
    VOID CreateRenderTargetResources();
    VOID CreateRenderTargetViews();
    VOID CreateDsvResourcesAndViews();

    ComPtr<ID3D12PipelineState> GetGfxPipelineStateWithShaders(const std::string& vertexShaderName,
                                                               const std::string& hullShaderName,
                                                               const std::string& domainShaderName,
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
        return m_descriptorHeapManager->GetSrvUavCbvDescriptorHeap();
    }

    inline D3D12_CPU_DESCRIPTOR_HANDLE GetRtvCpuHandle(UINT appHeapOffset)
    {
        return m_descriptorHeapManager->GetRtvHeapHandle<CD3DX12_CPU_DESCRIPTOR_HANDLE>(appHeapOffset);
    }

    inline D3D12_CPU_DESCRIPTOR_HANDLE GetDsvCpuHandle(UINT appHeapOffset)
    {
        return m_descriptorHeapManager->GetDsvHeapHandle<CD3DX12_CPU_DESCRIPTOR_HANDLE>(appHeapOffset);
    }

    inline D3D12_GPU_DESCRIPTOR_HANDLE GetPerPrimSrvGpuHandle(UINT linearPrimIdx)
    {
        return m_descriptorHeapManager->GetPerPrimSrvHeapHandle<CD3DX12_GPU_DESCRIPTOR_HANDLE>(linearPrimIdx, 0);
    }

    inline D3D12_GPU_DESCRIPTOR_HANDLE GetUavGpuHandle(UINT appUavIndex)
    {
        return m_descriptorHeapManager->GetUavHeapHandle<CD3DX12_GPU_DESCRIPTOR_HANDLE>(appUavIndex);
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

    inline BOOL IsPrimitiveTransparent(DxPrimitiveInfo& primInfo)
    {
        BOOL isBlend = (((primInfo.materialCbData.flags & AlphaModeBlend) == 0) ? FALSE : TRUE);
        BOOL isAlphaMask = (((primInfo.materialCbData.flags & AlphaModeMask) == 0) ? FALSE : TRUE);
        BOOL isTransparent = (isBlend == TRUE || isAlphaMask == TRUE);
        return isTransparent;
    }

    inline BOOL IsPrimitiveTransparent(UINT sceneIdx, UINT nodeIndex, UINT primitiveIndex)
    {
        auto curPrim = GetPrimitiveInfo(sceneIdx, nodeIndex, primitiveIndex);
        return IsPrimitiveTransparent(curPrim);
    }

    inline DxPrimVertexData& GetPrimitiveVertexData(UINT sceneIdx, UINT nodeIndex, UINT primitiveIndex, UINT vbIndex)
    {
        return GetPrimitiveInfo(sceneIdx, nodeIndex, primitiveIndex).vertexBufferInfo[vbIndex];
    }

    inline DxPrimIndexData& GetPrimitiveIndexData(UINT sceneIdx, UINT nodeIndex, UINT primitiveIndex)
    {
        return GetPrimitiveInfo(sceneIdx, nodeIndex, primitiveIndex).indexBufferInfo;
    }

    inline DxDrawPrimitive& GetModelDrawInfo(UINT sceneIdx, UINT nodeIndex, UINT primitiveIndex)
    {
        return GetPrimitiveInfo(sceneIdx, nodeIndex, primitiveIndex).modelDrawPrimitive;
    }

    inline UINT NumVertexAttributesInPrimitive(UINT sceneIdx, UINT nodeIndex, UINT primitiveIndex)
    {
        return static_cast<UINT>(GetPrimitiveInfo(sceneIdx, nodeIndex, primitiveIndex).vertexBufferInfo.size());
    }

    ///@todo Assumptions, POSITION, NORMAL, TEXCOORD0, TEXCOORD1 etc as per semantic order in gltfloader
    ///      But these can be in any order
    inline const DxPrimVertexData* GetVertexBufferInfo(const DxPrimitiveInfo& primInfo, GltfVertexAttribIndex attribIndex)
    {
        const DxPrimVertexData* vbData = nullptr;

        if (attribIndex < primInfo.vertexBufferInfo.size())
        {
            vbData = &primInfo.vertexBufferInfo[attribIndex];
        }

        return vbData;
    }

    
    inline const DxPrimIndexData& GetIndexBufferInfo(const DxPrimitiveInfo& primInfo)
    {
        return primInfo.indexBufferInfo;
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

    inline VOID AddFrameInfo(UINT heapOffset, DxDescriptorType type)
    {
        m_appFrameInfo.emplace_back(type, heapOffset);
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

    //<---------------------------- scene load info ----------------------------------->
    inline UINT NumElementsInSceneLoad()
    {
        return m_sceneInfo.sceneLoadInfo.size();
    }

    inline DxSceneLoadInfo& SceneElementInstance(UINT idx)
    {
        return m_sceneInfo.sceneLoadInfo[idx];
    }

    virtual inline DxCamera* GetCamera() { return m_camera.get(); }
    virtual inline DXGI_FORMAT GetBackBufferFormat() { return DXGI_FORMAT_R8G8B8A8_UNORM; }
    virtual inline DXGI_FORMAT GetDepthStencilResourceFormat() { return DXGI_FORMAT_R32_TYPELESS; }
    virtual inline DXGI_FORMAT GetDepthStencilDsvFormat() { return DXGI_FORMAT_D32_FLOAT; }
    virtual inline DXGI_FORMAT GetDepthStencilSrvFormat() { return DXGI_FORMAT_R32_FLOAT; }

    virtual inline UINT NumRTVsNeededForApp()         { return 0; }
    virtual inline UINT NumSRVsPerPrimNeededForApp()  { return 0; }
    virtual inline UINT NumDSVsNeededForApp()         { return 0; }
    virtual inline UINT NumUAVsNeededForApp()         { return 0; }
    virtual inline UINT NumRootConstantsForApp()      { return 0; }
    virtual inline UINT NumRootSrvDescriptorsForApp() { return 0; }
    virtual ID3D12RootSignature* GetRootSignature() { return nullptr; }
    virtual inline const std::vector<std::string> GltfFileName() { return { "triangle.gltf" }; }
    virtual inline std::array<FLOAT, 4> RenderTargetClearColor() { return{ 0.2f, 0.2f, 0.2f, 1.0f }; }

    virtual inline BOOL EnablePBRShading()
    {
        return FALSE;
    }

    virtual inline std::string GetVertexShaderName(SIZE_T numVertexAttributes)
    {
        char shaderName[64];
        snprintf(shaderName, 64, "Simple%zu_VS.cso", numVertexAttributes);
        return std::string(shaderName);
    }

    virtual inline std::string GetHullShaderName()
    {
        return std::string();
    }

    virtual inline std::string GetDomainShaderName()
    {
        return std::string();
    }

    virtual inline std::string GetPixelShaderName()
    {
        char shaderName[64];
        snprintf(shaderName, 64, "SimplePS.cso");
        return std::string(shaderName);
    }

    template<typename Func>
    void ForEachSceneElementLoadedSceneNode(Func&& func)
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

    template<typename Func>
    void ForEachSceneElementLoadedSceneNodePrim(Func&& func)
    {
        const UINT numSceneElements = NumSceneElementsLoaded();

        for (UINT idx = 0; idx < numSceneElements; idx++)
        {
            const UINT numNodesInScene = NumNodesInScene(idx);

            for (UINT nodeIdx = 0; nodeIdx < numNodesInScene; nodeIdx++)
            {
                const UINT numPrimitivesInNodeMesh = NumPrimitivesInNodeMesh(idx, nodeIdx);
                for (UINT primIdx = 0; primIdx < numPrimitivesInNodeMesh; primIdx++)
                {
                    func(idx, nodeIdx, primIdx);
                }
            }
        }
    }

    template<typename Func>
    void ProcessSceneInstancesNodesPrims(Func&& func)
    {
        const UINT numElementsInSceneLoad = NumElementsInSceneLoad();
        UINT flatInstanceNodeIdx = 0;
        for (UINT idx = 0; idx < numElementsInSceneLoad; idx++)
        {
            auto& sceneLoadElement     = SceneElementInstance(idx);
            const UINT sceneElementIdx = sceneLoadElement.sceneElementIdx;
            const UINT numNodes        = NumNodesInScene(sceneElementIdx);
            const UINT numInstances    = sceneLoadElement.numInstances;

            for (UINT instanceIdx = 0; instanceIdx < numInstances; instanceIdx++)
            {
                for (UINT nodeIdx = 0; nodeIdx < numNodes; nodeIdx++)
                {
                    const UINT numPrims = NumPrimitivesInNodeMesh(sceneElementIdx, nodeIdx);
                    for (UINT primIdx = 0; primIdx < numPrims; primIdx++)
                    {
                        func(idx, instanceIdx, nodeIdx, primIdx, flatInstanceNodeIdx, sceneElementIdx);

                    }
                    flatInstanceNodeIdx++;
                }
            }
        }
    }

    virtual VOID LoadSceneDescription(std::vector< DxSceneElementInstance>& sceneDescription);


    VOID CreateAppSrvDescriptorAtIndex(UINT linearPrimIdx, UINT offsetInPrim, ID3D12Resource* srvResource);
    VOID CreateAppUavDescriptorAtIndex(UINT appUavIndex, ID3D12Resource* uavResource);
    VOID CreateAppBufferSrvDescriptorAtIndex(UINT linearPrimIdx, UINT offsetInPrim, ID3D12Resource* srvResource, UINT numElements, UINT elementSize);

    static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

    VOID LoadGltfFiles();
    VOID LoadSceneMaterialInfo();
    VOID LoadScene();
    HRESULT CreatePerPrimGfxPipelineState();

    VOID StartBuildingAccelerationStructures();
    VOID ExecuteBuildAccelerationStructures();

    HRESULT CreateSceneMVPMatrix();
    VOID CreateSceneMaterialCb();

    UINT CreateInputElementDesc(const std::vector<DxPrimVertexData>& inData, std::vector<D3D12_INPUT_ELEMENT_DESC>& outData);

    inline D3D12_GPU_VIRTUAL_ADDRESS GetViewProjGpuVa()
    {
        return m_camLightsMaterialsManager->GetViewProjGpuVa();
    }

    inline D3D12_GPU_VIRTUAL_ADDRESS GetPerInstanceDataGpuVa(UINT linearIdx)
    {
        return m_camLightsMaterialsManager->GetPerInstanceDataGpuVa(linearIdx);
    }

    inline static  Dx12SampleBase* GetInstance()
    {
        return s_sampleBase;
    }

    inline void CreateSrvBufferForPrimitive(const DxPrimitiveInfo& primInfo, GltfVertexAttribIndex attribIndex, UINT offset, BOOL isIndexBuffer = FALSE)
    {
        assert(attribIndex == UINT_MAX || isIndexBuffer == FALSE);

        ID3D12Resource* pRes = nullptr;
        UINT            numElements = 0;
        UINT            elementSize = 0;

        if (isIndexBuffer == FALSE)
        {
            auto* vbPosInfo = GetVertexBufferInfo(primInfo, attribIndex);
            if (vbPosInfo != nullptr)
            {
                auto posVbView = vbPosInfo->modelVbv;

                elementSize    = posVbView.StrideInBytes;
                numElements    = posVbView.SizeInBytes / elementSize;
                pRes           = vbPosInfo->modelVbBuffer.Get();
            }
        }
        else
        {
            auto ibPosInfo = GetIndexBufferInfo(primInfo);
            auto ibView    = ibPosInfo.modelIbv;
            elementSize    = ibPosInfo.bufferStrideInBytes;
            numElements    = ibView.SizeInBytes / elementSize;
            pRes           = ibPosInfo.indexBuffer.Get();

        }

        assert(pRes == nullptr || elementSize == 12 || elementSize == 8 || elementSize == 4);
        CreateAppBufferSrvDescriptorAtIndex(primInfo.primLinearIdxInSceneElements,
                                            offset,
                                            pRes,
                                            numElements,
                                            elementSize);
    }

private:
    friend class DxGfxDrawRenderObject;
    inline ID3D12CommandQueue* GetCommandQueue() { return m_pCmdQueue.Get(); }

    inline VOID SetNumTotalPrimitivesInScene(UINT numPrims, UINT sceneIdx)
    {
        m_sceneElements[sceneIdx].numTotalPrimitivesInScene = numPrims;
    }

    inline VOID IncrementPerInstanceDataCount(UINT increment)
    {
        m_camLightsMaterialsManager->IncrementPerInstanceDataCount(increment);
    }

    inline VOID IncrementMaterialDataCount(UINT increment)
    {
        m_camLightsMaterialsManager->IncrementMaterialDataCount(increment);
    }

    inline BOOL IsTessEnabled()
    {
        return ((GetHullShaderName().length() > 0) ? TRUE : FALSE);
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


    VOID Imgui_CreateDescriptorHeap();
    UINT    GetBackBufferCount();

    UINT                       m_width;
    UINT                       m_height;

    ComPtr<IDXGIFactory7>      m_dxgiFactory7;    ///< OS graphics infracture e.g. creation of swapchain
    ComPtr<ID3D12Device>       m_pDevice;
    ComPtr<ID3D12CommandQueue> m_pCmdQueue;
    ComPtr<ID3D12CommandQueue> m_pComputeCmdQueue;
    ComPtr<IDXGISwapChain4>    m_swapChain4;

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
    std::unique_ptr<CameraLightsMaterialsBuffer> m_camLightsMaterialsManager;
    std::unique_ptr<DxDescriptorHeapManager> m_descriptorHeapManager;

    FrameComposition    m_simpleComposition;
    ComputeGenerateMips m_computeGenerateMips;

    ComPtr<ID3D12Resource>      m_materialConstantBuffer;

    ///@todo associate fences with command queues?
    ComPtr<ID3D12Fence> m_fence;
    UINT64              m_fenceValue;
    HANDLE              m_fenceEvent;
    FLOAT               m_frameDeltaTime;

    UINT m_samplerDescriptorSize;
    UINT m_rtvDescriptorSize;
    UINT m_dsvDescriptorSize;

    HWND m_hwnd;

    ComPtr<ID3D12DescriptorHeap>   m_imguiDescHeap;
    std::vector<DxAppFrameInfo>    m_appFrameInfo;
    std::unique_ptr<DxGltfLoader>  m_gltfLoader;

    std::vector<DxSceneElements>	    m_sceneElements;
    static 	Dx12SampleBase*             s_sampleBase;

    std::vector<DxSceneElementInstance> m_sceneDescription;

    DxSceneInfo m_sceneInfo;

};


