#pragma once

#include "dxtypes.h"
#include "DxRayTracingShaderTable.h"

class DxShaderBindingTable
{
public:
    DxShaderBindingTable();
    VOID CreateShaderTable(DxShaderTableType tableType, UINT numShaderRecords, UINT numGpuVAs, UINT numGpuDesc);
    VOID FinalizeSBT();

    inline DxRayTracingShaderTable* GetShaderTable(DxShaderTableType tableType) { return m_shaderTables[tableType].get(); }
    inline UINT64 GetTotalSbtSize() { return m_totalSbtSize; }
    const BYTE* const GetSbtData() { return m_pCpuAllocPtr.get(); }

private:

    UINT64 m_totalSbtSize;

    std::vector<std::unique_ptr<DxRayTracingShaderTable>> m_shaderTables;
    std::vector<DxShaderTableOffsetAndStride>             m_tableOffsetAndStride;
    std::unique_ptr<BYTE[]>                               m_pCpuAllocPtr;

};

