#pragma once

#include "dxtypes.h"

class DxRayTracingShaderTable
{
public:
    DxRayTracingShaderTable(UINT numShaderRecords, UINT numGpuVAs, UINT numGpuDescHandles);
    VOID AddShaderRecord(VOID* shaderIdentifier);
    VOID AddShaderRecord(VOID* shaderIdentifier, D3D12_GPU_VIRTUAL_ADDRESS arg1, D3D12_GPU_DESCRIPTOR_HANDLE arg2);


    inline UINT64 GetAlignedShaderTableSize() { return m_alignedShaderTableSize; }
    inline BYTE* GetShaderTableData() { return m_pCpuAllocPtr.get(); }




private:

    VOID WriteShaderRecordData(VOID* data, SIZE_T dataSizeInBytes);
    const BYTE* const PreAddShaderRecordData(UINT numGpuVAs, UINT numGpuDescHandles);
    VOID PostAddShaderRecordData(const BYTE* const pStartPtr);
    static UINT GetShaderRecordSize(UINT numGpuVAs, UINT numGpuDescHandles);

    UINT   m_shaderRecordSize;
    UINT   m_alignedShaderRecordSize;
    UINT64 m_shaderTableSize;
    UINT64 m_alignedShaderTableSize;

    std::unique_ptr<BYTE[]> m_pCpuAllocPtr;
    BYTE* m_pWritePtr;
};

