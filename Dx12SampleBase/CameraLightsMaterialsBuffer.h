#pragma once



#include "dxtypes.h"


class CameraLightsMaterialsBuffer
{

    ///@note Data in the buffer has different frequency of change
    ///Ordered from least to highest frequency -
    ///Projection Matrix - almost never changes    (lowest change frequency)
    ///Lights - only changes if moving else constant
    ///View matrix -  same as Lights
    ///Materials - changes per primitive
    ///Model matrix - changes per instance (highest change frequency)
    /// 
    /// 
    /// Buffer Layout [ Scene Data | Per Instance Geometry Data | Material Data]
    /// 
    /// Scene Data (just one value for each which might change e.g. user interaction)
    ///          values: camera position, FovY, Inverse_viewProj(RayTracing)
    ///          data: from DxCamera because user interaction changes this
    /// 
    ///  Per Instance Geometry Data (a ton of values, each matrix has different values per instance)
    /// e.g. OakTree has 2 nodes, if scene has 10 instances, we need 20 below matrices
    ///               values: model matrix, normal matrix, 
    ///               data: gltf trs matrix + instance trs from scene description
    ///               [data for
    /// 
    /// Material Data (per primitive)
    /// e.g OakTree has 2 primitives and each primitive has one MaterialCB so total of 2.
    /// Instances of OakTree share MaterialData
    ///                values: MaterialCB
    ///                data: loaded from gltf and some flags
    /// 
    /// Class responsibility:
    /// - Calculate total buffer size 
    /// - allocate buffer
    /// - fill in data
    /// - return GPUVA for each chunk i.e scene, geometry, material
    /// - 
    ///           
    
public:
    CameraLightsMaterialsBuffer();
    VOID Finalize(ID3D12Device *pDevice);

    inline void IncrementPerInstanceDataCount(UINT increment)
    {
        m_numPerInstanceDataCount += increment;
    }

    inline void IncrementMaterialDataCount(UINT increment)
    {
        m_numMaterialDataCount += increment;
    }

    inline UINT GetPerInstanceDataCount()
    {
        return m_numPerInstanceDataCount;
    }


protected:
private:

    inline VOID MapAndInitializeBaseAddress()
    {
        CD3DX12_RANGE readRange(0, 0);
        //@note specifying nullptr as read range indicates CPU can read entire resource
        VOID* pCpuPtr;
        m_bufferResource->Map(0, &readRange, &pCpuPtr);
        m_pBufferResCpuPtr = static_cast<BYTE*>(pCpuPtr);
        m_pBufferResGpuVa = m_bufferResource->GetGPUVirtualAddress();
    }

    ComPtr<ID3D12Resource> m_bufferResource;
    UINT m_sceneDataChunkSize;
    UINT m_sceneDataAlignedChunkSize;

    UINT m_instanceDataChunkSize;
    UINT m_instanceDataAlignedChunkSize;

    UINT m_materialDataChunkSize;
    UINT m_materialDataAlignedChunkSize;

    SIZE_T m_instanceDataTotalAlignedSize;
    SIZE_T m_materialDataTotalAlignedSize;

    SIZE_T m_totalBufferSize;

    BYTE* m_pBufferResCpuPtr;
    D3D12_GPU_VIRTUAL_ADDRESS m_pBufferResGpuVa;

    UINT m_numPerInstanceDataCount;
    UINT m_numMaterialDataCount;
};

