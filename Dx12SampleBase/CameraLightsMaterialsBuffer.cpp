#include "pch.h"
#include "CameraLightsMaterialsBuffer.h"
#include "dxhelper.h"


CameraLightsMaterialsBuffer::CameraLightsMaterialsBuffer() :
    m_bufferResource(nullptr),
    m_viewProjData({}),
    m_worldMatrixData({}),
    m_materialData({}),
    m_totalBufferSize(0),
    m_pBufferResCpuPtr(nullptr)
{
    dxhelper::GetSizeAndAlignedSizeInfo<DxCBSceneData>(m_viewProjData);
    dxhelper::GetSizeAndAlignedSizeInfo<DxCBPerInstanceData>(m_worldMatrixData);
    dxhelper::GetSizeAndAlignedSizeInfo<DxMaterialCB>(m_materialData);
    dxhelper::GetSizeAndAlignedSizeInfo<DxLightDataCB>(m_lightsData);

    //@todo
    m_viewProjData.count = 1;
}

VOID CameraLightsMaterialsBuffer::Finalize(ID3D12Device *pDevice)
{
    //Loading Scene Elements determine number of world matrices required
    FinalizeCalcTotalSize();
    dxhelper::AllocateBufferResource(pDevice, m_totalBufferSize, &m_bufferResource, "CameraLightsMaterialBuffer", D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COMMON, TRUE);
    MapAndInitializeBaseAddress();
}




