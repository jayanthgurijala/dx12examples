#include "pch.h"
#include "DxDescriptorHeapManager.h"

DxDescriptorHeapManager::DxDescriptorHeapManager(ID3D12Device* pDevice,
                                                     UINT numRTVs,
                                                     UINT numDSVs,
                                                     UINT numUAVs,
                                                     UINT numSrvs,
                                                     UINT numSrvsPerPrim)
{
    m_pDevice = pDevice;
    InitializeSrvCbvUavOffsets(numRTVs, numDSVs, numUAVs, numSrvs, numSrvsPerPrim);

    ///@note RTV, DSV and UAVs are output but UAVs reside in SrvUavCbv heap as the o/p is directly written by shaders
    ///      Hence we need to create 2 entries. RTV and DSV have their own heap.
    CreateSrvUavCbvDescriptorHeap(numRTVs + numDSVs + numUAVs* 2 + numSrvs);
    CreateRtvDescriptorHeap(numRTVs);
    CreateDsvDescriptorHeap(numDSVs);
}

VOID DxDescriptorHeapManager::CreateDescriptorHeap(DxDescriptorHeap& outCreatedInfo, D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors)
{
    outCreatedInfo.descriptorCount = numDescriptors;
    outCreatedInfo.descriptorSize  = GetDevice()->GetDescriptorHandleIncrementSize(heapType);
    outCreatedInfo.descriptorHeap  = dxhelper::CreateDescriptorHeap(GetDevice(), numDescriptors, heapType);
    outCreatedInfo.resourceDescInfo.resize(numDescriptors);
}

VOID DxDescriptorHeapManager::CreateRtvDescriptor(UINT heapOffset, D3D12_RENDER_TARGET_VIEW_DESC* pRtvDesc, ID3D12Resource* pResource, BOOL isActiveInHeap, BOOL needsSrv)
{

    const UINT heapOffsetInRtvHeap = heapOffset;
    auto cpuHandle = GetRtvHeapHandle<CD3DX12_CPU_DESCRIPTOR_HANDLE>(heapOffset);
    GetDevice()->CreateRenderTargetView(pResource, pRtvDesc, cpuHandle);

    ///@note RTV back buffers will be created after all RTVs needed for app.
    if (needsSrv)
    {
        const UINT heapOffsetInSrvHeap = m_srvUavCbvOffsetsInfo.srvForRtvStartOffset + heapOffset;
        auto       srvCpuHandle        = GetSrvUavCbvHeapHandle<CD3DX12_CPU_DESCRIPTOR_HANDLE>(heapOffsetInSrvHeap);
        GetDevice()->CreateShaderResourceView(pResource, nullptr, srvCpuHandle);
    }
    AddResourceDescriptorInfo(DxHeapTypeRtv, heapOffset, pResource);
}

VOID DxDescriptorHeapManager::CreateDsvDescriptor(UINT heapOffset, D3D12_DEPTH_STENCIL_VIEW_DESC* pDsvDesc, D3D12_SHADER_RESOURCE_VIEW_DESC* pSrvDesc, ID3D12Resource* pResource, BOOL isActiveInHeap)
{
    const UINT heapOffsetInDsvHeap = heapOffset;
    auto       cpuHandle           = GetDsvHeapHandle<CD3DX12_CPU_DESCRIPTOR_HANDLE>(heapOffset);
    GetDevice()->CreateDepthStencilView(pResource, pDsvDesc, cpuHandle);

    const UINT heapOffsetInSrvHeap = m_srvUavCbvOffsetsInfo.srvForDsvStartOffset + heapOffset;
    auto       srvCpuHandle        = GetSrvUavCbvHeapHandle<CD3DX12_CPU_DESCRIPTOR_HANDLE>(heapOffsetInSrvHeap);
    GetDevice()->CreateShaderResourceView(pResource, pSrvDesc, srvCpuHandle);
    AddResourceDescriptorInfo(DxHeapTypeDsv, heapOffset, pResource);
}

VOID DxDescriptorHeapManager::CreateUavDescriptor(UINT heapOffset, D3D12_UNORDERED_ACCESS_VIEW_DESC* pUavDesc, ID3D12Resource* pResource, ID3D12Resource* pCounterResource, BOOL isActiveInHeap)
{
    const UINT heapOffsetForUav = m_srvUavCbvOffsetsInfo.uavStartOffset + heapOffset;
    auto       uavHandle        = GetSrvUavCbvHeapHandle<CD3DX12_CPU_DESCRIPTOR_HANDLE>(heapOffsetForUav);
    GetDevice()->CreateUnorderedAccessView(pResource, pCounterResource, pUavDesc, uavHandle);

    const UINT heapOffsetForUavSrv = m_srvUavCbvOffsetsInfo.srvForUavStartOffset + heapOffset;
    auto       uavSrvHandle        = GetSrvUavCbvHeapHandle<CD3DX12_CPU_DESCRIPTOR_HANDLE>(heapOffsetForUavSrv);
    GetDevice()->CreateShaderResourceView(pResource, nullptr, uavSrvHandle);

    AddResourceDescriptorInfo(DxHeapTypeSrvUavCbv, heapOffset, pResource);
}
