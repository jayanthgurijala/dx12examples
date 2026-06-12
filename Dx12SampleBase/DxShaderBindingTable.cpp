#include "pch.h"
#include "DxShaderBindingTable.h"

DxShaderBindingTable::DxShaderBindingTable() :
    m_totalSbtSize(0)
{
    const UINT numShaderTables = DxShaderTablesMax;

    m_shaderTables.resize(numShaderTables);
    m_tableOffsetAndStride.resize(numShaderTables);

}

VOID DxShaderBindingTable::CreateShaderTable(DxShaderTableType tableType, UINT numShaderRecords, UINT numGpuVAs, UINT numGpuDesc)
{
    m_shaderTables[tableType] = std::make_unique<DxRayTracingShaderTable>(numShaderRecords, numGpuVAs, numGpuDesc);
}

VOID DxShaderBindingTable::FinalizeSBT()
{
    UINT offset = 0;
    for (UINT tableType = 0; tableType < DxShaderTablesMax; tableType++)
    {
        const UINT64 tableSize  = m_shaderTables[tableType]->GetAlignedShaderTableSize();
        const UINT64 recordSize = m_shaderTables[tableType]->GetAlignedShaderRecordSize();

        assert(tableSize > 0);
        assert(recordSize > 0);
        assert(tableSize >= recordSize);

        m_tableOffsetAndStride[tableType].tableOffset  = offset;
        m_tableOffsetAndStride[tableType].recordStride = recordSize;

        m_totalSbtSize += tableSize;
        offset         += tableSize;
    }

    m_pCpuAllocPtr = std::make_unique<BYTE[]>(m_totalSbtSize);
    UINT64 bytesWritten = 0;

    for (UINT tableType = 0; tableType < DxShaderTablesMax; tableType++)
    {
        bytesWritten += m_shaderTables[tableType]->CopyShaderTableData(m_pCpuAllocPtr.get() + bytesWritten, m_totalSbtSize - bytesWritten);
    }

    assert(bytesWritten == m_totalSbtSize);

}
