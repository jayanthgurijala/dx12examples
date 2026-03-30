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
    /// Buffer Layout [ Scene Data | Lights (if any)  | Per Instance Geometry Data | Material Data]
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
        m_worldMatrixData.count += increment;
    }

    inline void IncrementMaterialDataCount(UINT increment)
    {
        m_materialData.count += increment;
    }

    inline void IncrementLightsCount(UINT increment)
    {
        m_lightsData.count += increment;
    }

    inline D3D12_GPU_VIRTUAL_ADDRESS GetViewProjGpuVa()
    {
        return GetBaseGpuVa() + ViewProjDataOffset();
    }

    inline D3D12_GPU_VIRTUAL_ADDRESS GetPerInstanceDataGpuVa(UINT linearIdx)
    {
        return GetBaseGpuVa() + PerInstanceDataOffset(linearIdx);
    }

    inline BYTE* GetPerInstanceDataCpuVa(UINT linearIdx)
    {
        return m_pBufferResCpuPtr + PerInstanceDataOffset(linearIdx);
    }

    inline VOID WriteViewProjDataAtIndex(UINT index, DxCBSceneData& sceneData)
    {
        BYTE* pWritePtr = m_pBufferResCpuPtr + index * m_viewProjData.alignedSize;

        assert(sizeof(DxCBSceneData) == m_viewProjData.size);
        memcpy(pWritePtr, &sceneData, m_viewProjData.size);
    }

    inline VOID WritePerInstanceWorldMatrixData(UINT index, DxCBPerInstanceData& perInstanceData)
    {
        BYTE* pWritePtr = m_pBufferResCpuPtr + PerInstanceDataOffset(index);

        assert(sizeof(DxCBPerInstanceData) == m_worldMatrixData.size);
        memcpy(pWritePtr, &perInstanceData, m_viewProjData.size);
    }


protected:
private:

    inline VOID FinalizeCalcTotalSize()
    {
        m_viewProjData.CalcTotalSize();
        m_worldMatrixData.CalcTotalSize();
        m_materialData.CalcTotalSize();
        m_lightsData.CalcTotalSize();
        m_totalBufferSize = m_viewProjData.totalAlignedSize    +
                            m_lightsData.totalAlignedSize      +
                            m_worldMatrixData.totalAlignedSize +
                            m_materialData.totalAlignedSize;
                             
    }

    inline UINT64 ViewProjDataOffset()
    {
        return 0;
    }

    inline UINT64 PerInstanceDataOffset(UINT linearIdx)
    {
        return m_viewProjData.totalAlignedSize + linearIdx * m_worldMatrixData.alignedSize;
    }

    inline D3D12_GPU_VIRTUAL_ADDRESS GetBaseGpuVa()
    {
        return m_bufferResource->GetGPUVirtualAddress();
    }

    inline VOID MapAndInitializeBaseAddress()
    {
        CD3DX12_RANGE readRange(0, 0);
        //@note specifying nullptr as read range indicates CPU can read entire resource
        VOID* pCpuPtr;
        m_bufferResource->Map(0, &readRange, &pCpuPtr);
        m_pBufferResCpuPtr = static_cast<BYTE*>(pCpuPtr);
    }

    ComPtr<ID3D12Resource> m_bufferResource;

    DxSizeAlignedSize m_viewProjData;
    DxSizeAlignedSize m_lightsData;
    DxSizeAlignedSize m_worldMatrixData;
    DxSizeAlignedSize m_materialData;

    SIZE_T m_totalBufferSize;

    BYTE* m_pBufferResCpuPtr;
};

