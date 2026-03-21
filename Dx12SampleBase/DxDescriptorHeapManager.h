#pragma once

#include <d3dx12.h>
#include "dxtypes.h"
#include "dxhelper.h"

class DxDescriptorHeapManager
{
    enum DxHeapType
    {
        DxHeapTypeRtv       = 0,
        DxHeapTypeDsv       = 1,
        DxHeapTypeSrvUavCbv = 2,
        DxHeapTypeMax
    };

public:

    DxDescriptorHeapManager(ID3D12Device* pDevice, UINT numRTVs, UINT numDSVs, UINT numUAVs, UINT numSrvs, UINT numSrvsPerPrimitive);


    ///@note for every RTV and DSV, we create a corresponding SRV descriptor, at the same heap offset with the section in SRV.
    ///      See more on SRV heap layout below
    ///      In RTV heap, we have SwapChain RTVs without SRVs
    VOID CreateRtvDescriptor(UINT heapOffset, D3D12_RENDER_TARGET_VIEW_DESC* pRtvDesc, ID3D12Resource* pResource, BOOL isActiveInHeap, BOOL needsSrv);
    VOID CreateDsvDescriptor(UINT heapOffset, D3D12_DEPTH_STENCIL_VIEW_DESC* pDsvDesc, D3D12_SHADER_RESOURCE_VIEW_DESC* pSrvDesc, ID3D12Resource* pResource, BOOL isActiveInHeap);
    VOID CreateUavDescriptor(UINT heapOffset, D3D12_UNORDERED_ACCESS_VIEW_DESC* pUavDesc, ID3D12Resource* pResource, ID3D12Resource* counterResource, BOOL isActiveInHeap);

    inline ID3D12DescriptorHeap* GetSrvUavCbvDescriptorHeap()
    {
        return m_descriptorHeap[DxHeapTypeSrvUavCbv].descriptorHeap.Get();
    }


    template <typename HandleType>
    inline HandleType GetSrvUavCbvSrvHeapHandle(DxRangeTypeSrvUavCbv type, UINT index)
    {
        const UINT offset = GetOffset(type);
        const UINT debugOffset = GetOffset(type + 1);
        assert(offset + index < debugOffset);
        return GetHeapHandle<HandleType>(DxHeapTypeSrvUavCbv, offset + index);
    }

    template <typename HandleType>
    inline HandleType GetPerPrimSrvHeapHandle(UINT linearPrimIndex, UINT offsetInPrim)
    {
        const UINT finalOffset = GetOffset(DxRangeTypePerPrimSrv) + GetPerPrimSrvHeapOffset(linearPrimIndex, offsetInPrim);
        return GetHeapHandle<HandleType>(DxHeapTypeSrvUavCbv, finalOffset);
    }

    template <typename HandleType>
    inline HandleType GetHeapHandle(DxHeapType heapType, UINT finalOffset)
    {
        auto handle = GetHeapStart<HandleType>(m_descriptorHeap[heapType].descriptorHeap.Get());
        dxhelper::OffsetHandle<HandleType>(handle, finalOffset, m_descriptorHeap[heapType].descriptorSize);
        return handle;
    }

    template <typename HandleType>
    inline HandleType GetRtvHeapHandle(UINT finalOffset)
    {
        return GetHeapHandle<HandleType>(DxHeapTypeRtv, finalOffset);
    }

    template <typename HandleType>
    inline HandleType GetDsvHeapHandle(UINT finalOffset)
    {
        return GetHeapHandle<HandleType>(DxHeapTypeDsv, finalOffset);
    }

    template <typename HandleType>
    inline HandleType GetSrvUavCbvHeapHandle(UINT finalOffset)
    {
        return GetHeapHandle<HandleType>(DxHeapTypeSrvUavCbv, finalOffset);
    }
    template <typename HandleType>
    inline HandleType GetUavHeapHandle(UINT appUavOffset)
    {
        return GetHeapHandle<HandleType>(DxHeapTypeSrvUavCbv, m_srvUavCbvOffsetsInfo.uavStartOffset + appUavOffset);
    }



    ///@note App is not aware of srvUavCBv heap layout and hence does not know absolute UAV offset.
    ///      RTV and DSV heap offsets are more straighward but opaque for app.
    D3D12_GPU_DESCRIPTOR_HANDLE GetSrvHandleForDescriptor(DxDescriptorType descriptorType, UINT appBasedHeapOffset)
    {
        UINT srvuavCbvHeapOffset = UINT_MAX;

        if (descriptorType == DxDescriptorTypeRtvSrv)
        {
            srvuavCbvHeapOffset = GetSrvHeapOffsetForRtvHeapOffset(appBasedHeapOffset);
        }
        else if (descriptorType == DxDescriptorTypeDsvSrv)
        {
            srvuavCbvHeapOffset = GetSrvHeapOffsetForDsvHeapOffset(appBasedHeapOffset);
        }
        else if (descriptorType == DxDescriptorTypeUavSrv)
        {
            srvuavCbvHeapOffset = GetSrvHeapOffsetForUavHeapOffset(appBasedHeapOffset);
        }
        else
        {
            assert(0);
        }

        assert(srvuavCbvHeapOffset != UINT_MAX);

        return GetHeapHandle<CD3DX12_GPU_DESCRIPTOR_HANDLE>(DxHeapTypeSrvUavCbv, srvuavCbvHeapOffset);
    }

    inline ID3D12Resource* GetResourceForDescriptorTypeInOffset(DxDescriptorType descriptorType, UINT heapOffset)
    {
        static_assert(DxHeapType::DxHeapTypeRtv == DxDescriptorType::DxDescriptorTypeRtvSrv, "Error: Rtv Descriptor");
        static_assert(DxHeapType::DxHeapTypeDsv == DxDescriptorType::DxDescriptorTypeDsvSrv, "Error: Dsv Descriptor");
        static_assert(DxHeapType::DxHeapTypeSrvUavCbv == DxDescriptorType::DxDescriptorTypeUavSrv, "ERROR: Srv Descriptor");

        //@note resource is only populated in RTV, DSV and UAV
        return m_descriptorHeap[descriptorType].resourceDescInfo[heapOffset].pResource;
    }


protected:

private:

    template<typename HandleType>
    HandleType GetHeapStart(ID3D12DescriptorHeap* heap);

    template<>
    CD3DX12_CPU_DESCRIPTOR_HANDLE
        GetHeapStart<CD3DX12_CPU_DESCRIPTOR_HANDLE>(ID3D12DescriptorHeap* heap)
    {
        return CD3DX12_CPU_DESCRIPTOR_HANDLE(heap->GetCPUDescriptorHandleForHeapStart());
    }

    template<>
    CD3DX12_GPU_DESCRIPTOR_HANDLE
        GetHeapStart<CD3DX12_GPU_DESCRIPTOR_HANDLE>(ID3D12DescriptorHeap* heap)
    {
        return CD3DX12_GPU_DESCRIPTOR_HANDLE(heap->GetGPUDescriptorHandleForHeapStart());
    }

    inline UINT GetSrvHeapOffsetForRtvHeapOffset(UINT appBasedHeapOffset)
    {
        return m_srvUavCbvOffsetsInfo.srvForRtvStartOffset + appBasedHeapOffset;
    }

    inline UINT GetSrvHeapOffsetForDsvHeapOffset(UINT appBasedHeapOffset)
    {
        return m_srvUavCbvOffsetsInfo.srvForDsvStartOffset + appBasedHeapOffset;
    }

    inline UINT GetSrvHeapOffsetForUavHeapOffset(UINT appBasedHeapOffset)
    {
        return m_srvUavCbvOffsetsInfo.srvForUavStartOffset + appBasedHeapOffset;
    }

    inline UINT GetPerPrimSrvHeapOffset(UINT linearPrimIndex, UINT offsetInPrim)
    {
        return linearPrimIndex * m_srvUavCbvOffsetsInfo.numPerPrimSrvs + offsetInPrim;
    }

    VOID CreateDescriptorHeap(DxDescriptorHeap& outCreatedInfo, D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors);

    inline VOID CreateSrvUavCbvDescriptorHeap(UINT numDescriptors)
    {
        CreateDescriptorHeap(m_descriptorHeap[DxHeapTypeSrvUavCbv], D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, numDescriptors);
    }

    inline VOID CreateRtvDescriptorHeap(UINT numDescriptors)
    {
        CreateDescriptorHeap(m_descriptorHeap[DxHeapTypeRtv], D3D12_DESCRIPTOR_HEAP_TYPE_RTV, numDescriptors);
    }

    inline VOID CreateDsvDescriptorHeap(UINT numDescriptors)
    {
        CreateDescriptorHeap(m_descriptorHeap[DxHeapTypeDsv], D3D12_DESCRIPTOR_HEAP_TYPE_DSV, numDescriptors);
    }

    inline VOID InitializeSrvCbvUavOffsets(UINT numRTVs,
        UINT numDSVs,
        UINT numUAVs,
        UINT numSrvs,
        UINT numSrvsPerPrim)
    {
        ///@note numUAVs twice is intentional, two entries are needed to write UAVs and read as SRV
        ///      Same pattern for RTVs, DSVs also but they live in own descriptor heaps
        ///      SrvUavCbv Descriptor heap is organized as
        ///      [SRVs for RTVs | SRVs for DSVs | UAVS | SRVs for UAVs | Per Primitive SRVs from gltf files + Ray/Mesh requirements (See Below) ]
        m_srvUavCbvOffsetsInfo.srvForRtvStartOffset = 0;
        m_srvUavCbvOffsetsInfo.srvForDsvStartOffset = numRTVs;
        m_srvUavCbvOffsetsInfo.uavStartOffset = numRTVs + numDSVs;
        m_srvUavCbvOffsetsInfo.srvForUavStartOffset = numRTVs + numDSVs + numUAVs;
        m_srvUavCbvOffsetsInfo.perPrimitiveSrvOffset = numRTVs + numDSVs + numUAVs + numUAVs;


        //Per Primitive SRVs is organized as numSrvsPerPrim per Primitive in the order of scene elements loaded.
        //e.g. In Hello Forest 
        //         (1) terrain_gridlines
        //         (2) deer
        //         (3) chinesedragon
        //         (4) oaktree
        //     [ 5 SRVs for (1) | app | 5 SRVs for (2) | app | 5 + 5 SRVs for (3) | app | 5 + 5 SRVs for (4) | app | ]
        //
        // Magic number "5" is for PBR shading
        assert(m_srvUavCbvOffsetsInfo.numSrvsForPbrPerPrim == 5);
        assert(numSrvsPerPrim >= m_srvUavCbvOffsetsInfo.numSrvsForPbrPerPrim);

        m_srvUavCbvOffsetsInfo.numPerPrimSrvs = numSrvsPerPrim;
        m_srvUavCbvOffsetsInfo.appSrvOffsetInPerPrimSrv = numSrvsPerPrim - m_srvUavCbvOffsetsInfo.numSrvsForPbrPerPrim;
    }

    inline ID3D12Device* GetDevice() { return m_pDevice; }

    inline UINT GetOffset(DxRangeTypeSrvUavCbv rangeType)
    {
        switch (rangeType)
        {
        case DxRangeTypeRtvAsSrv:
            return m_srvUavCbvOffsetsInfo.srvForRtvStartOffset;
            break;
        case DxRangeTypeDsvAsSrv:
            return m_srvUavCbvOffsetsInfo.srvForDsvStartOffset;
            break;
        case DxRangeTypeUav:
            return m_srvUavCbvOffsetsInfo.uavStartOffset;
            break;
        case DxRangeTypeUavAsSrv:
            return m_srvUavCbvOffsetsInfo.srvForUavStartOffset;
            break;
        case DxRangeTypePerPrimSrv:
            return m_srvUavCbvOffsetsInfo.perPrimitiveSrvOffset;
            break;
        default:
            assert(0);
            break;
        }
    }

    inline VOID AddResourceDescriptorInfo(DxHeapType heapType, UINT heapOffset, ID3D12Resource* pResource)
    {
        DxResourceDescriptorInfo& resDescriptorInfo = m_descriptorHeap[heapType].resourceDescInfo[heapOffset];
        resDescriptorInfo.pResource = pResource;
    }
   

    DxDescriptorHeap m_descriptorHeap[DxHeapTypeMax];


    /*
   * 1. All App Render Target Resources created as SRVs
   * 2. All App Depth Stencil Resources created as SRVs
   * 3. All App UAVs                               --> actual UAVs
   * 4. All App UAV SRVs                           --> UAVs when need to be used as SRV
   * 4. SRVs needed to render the scene, (5 + app specific e.g. SRVs for VBs IBs for Mesh and RayTracing) per primitive
   */
    DxSrvUavCbvOffsetsInfo m_srvUavCbvOffsetsInfo;

    ID3D12Device* m_pDevice;

};

