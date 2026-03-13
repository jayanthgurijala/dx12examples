#include "pch.h"
#include "CameraLightsMaterialsBuffer.h"
#include "dxhelper.h"


CameraLightsMaterialsBuffer::CameraLightsMaterialsBuffer() :
    m_bufferResource(nullptr),
    m_sceneDataChunkSize(0),
    m_sceneDataAlignedChunkSize(0),
    m_instanceDataChunkSize(0),
    m_instanceDataAlignedChunkSize(0),
    m_materialDataChunkSize(0),
    m_materialDataAlignedChunkSize(0),
    m_instanceDataTotalAlignedSize(0),
    m_materialDataTotalAlignedSize(0),
    m_totalBufferSize(0),
    m_pBufferResCpuPtr(nullptr),
    m_pBufferResGpuVa(0)
{
    dxhelper::GetSizeAndAlignedSizeInfo<DxCBSceneData>(m_sceneDataChunkSize, m_sceneDataAlignedChunkSize);
    dxhelper::GetSizeAndAlignedSizeInfo<DxCBPerInstanceData>(m_instanceDataChunkSize, m_instanceDataAlignedChunkSize);
    dxhelper::GetSizeAndAlignedSizeInfo<DxMaterialCB>(m_materialDataChunkSize, m_materialDataAlignedChunkSize);  
}

VOID CameraLightsMaterialsBuffer::Finalize(ID3D12Device *pDevice)
{
    m_instanceDataTotalAlignedSize = m_numPerInstanceDataCount * m_instanceDataAlignedChunkSize;
    m_materialDataAlignedChunkSize = m_numMaterialDataCount * m_materialDataAlignedChunkSize;
    m_totalBufferSize              = m_sceneDataAlignedChunkSize + m_instanceDataTotalAlignedSize + m_materialDataAlignedChunkSize;
    dxhelper::AllocateBufferResource(pDevice, m_totalBufferSize, &m_bufferResource, "CameraLightsMaterialBuffer", D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COMMON, TRUE);
    MapAndInitializeBaseAddress();
}




