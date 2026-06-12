#include "pch.h"
#include "DxRayTracingShaderTable.h"
#include "dxhelper.h"


UINT DxRayTracingShaderTable::GetShaderRecordSize(UINT numGpuVAs, UINT numGpuDescHandles)
{
    const UINT shaderIdentifierSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;

    const UINT sizeofGpuVa          = sizeof(D3D12_GPU_VIRTUAL_ADDRESS);
    const UINT totalSizeofGpuVAs    = sizeofGpuVa * numGpuVAs;

    const UINT sizeofGpuHandle      = sizeof(D3D12_GPU_DESCRIPTOR_HANDLE);
    const UINT totalSizeOfGpuHandle = sizeofGpuHandle * numGpuDescHandles;

    return (shaderIdentifierSize + totalSizeofGpuVAs + totalSizeOfGpuHandle);
}

VOID DxRayTracingShaderTable::WriteShaderRecordData(VOID* data, SIZE_T dataSizeInBytes)
{
    errno_t err = memcpy_s(m_pWritePtr, dataSizeInBytes, data, dataSizeInBytes);
    assert(err == S_OK);
    m_pWritePtr += dataSizeInBytes;
}

const BYTE* const DxRayTracingShaderTable::PreAddShaderRecordData(UINT numGpuVAs, UINT numGpuDescHandles)
{
    assert(m_shaderRecordSize == GetShaderRecordSize(numGpuVAs, numGpuDescHandles));

    const BYTE* const pStartPtr = m_pWritePtr;

    uintptr_t offsetFromStart = ((uintptr_t)(pStartPtr) - (uintptr_t)(m_pCpuAllocPtr.get()));
    assert(offsetFromStart % D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT == 0);

    return pStartPtr;
}

VOID DxRayTracingShaderTable::PostAddShaderRecordData(const BYTE* const pStartPtr)
{
    m_pWritePtr = const_cast<BYTE*>(pStartPtr) + m_alignedShaderRecordSize;
}


DxRayTracingShaderTable::DxRayTracingShaderTable(UINT numShaderRecords, UINT numGpuVAs, UINT numGpuDescHandles) :
    m_pCpuAllocPtr(nullptr)

{
    ///@note each shader record entry
    m_shaderRecordSize = GetShaderRecordSize(numGpuVAs, numGpuDescHandles);
    m_alignedShaderRecordSize = dxhelper::DxAlign(m_shaderRecordSize, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);

    ///@note shader table i.e. all shader record entries
    m_shaderTableSize        = m_alignedShaderRecordSize * numShaderRecords;
    m_alignedShaderTableSize = dxhelper::DxAlign(m_shaderTableSize, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
    m_pCpuAllocPtr           = std::make_unique<BYTE[]>(m_alignedShaderTableSize);
    m_pWritePtr              = m_pCpuAllocPtr.get();
}

VOID DxRayTracingShaderTable::AddShaderRecord(VOID* shaderIdentifier)
{
    auto pStartPtr = PreAddShaderRecordData(0,0);
    WriteShaderRecordData(shaderIdentifier, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    PostAddShaderRecordData(pStartPtr);
}

VOID DxRayTracingShaderTable::AddShaderRecord(VOID* shaderIdentifier, D3D12_GPU_VIRTUAL_ADDRESS arg1, D3D12_GPU_DESCRIPTOR_HANDLE arg2)
{
    auto pStartPtr = PreAddShaderRecordData(1, 1);

    WriteShaderRecordData(shaderIdentifier, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    WriteShaderRecordData(&arg1, sizeof(D3D12_GPU_VIRTUAL_ADDRESS));
    WriteShaderRecordData(&arg2, sizeof(D3D12_GPU_DESCRIPTOR_HANDLE));
    PostAddShaderRecordData(pStartPtr);

}

UINT64 DxRayTracingShaderTable::CopyShaderTableData(VOID* dstPtr, UINT64 minDstBufferSize)
{
    assert(m_alignedShaderTableSize <= minDstBufferSize);
    auto err = memcpy_s(dstPtr, minDstBufferSize, m_pCpuAllocPtr.get(), m_alignedShaderTableSize);
    assert(err == S_OK);
    return m_alignedShaderTableSize;
}



