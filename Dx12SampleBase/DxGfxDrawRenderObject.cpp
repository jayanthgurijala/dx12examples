#include "pch.h"
#include "DxGfxDrawRenderObject.h"

DxGfxDrawRenderObject::DxGfxDrawRenderObject(UINT viewProjOffset, UINT numRtvs, UINT rtvHeapOffset, BOOL needsDsv, UINT dsvHeapOffset)
{
    m_numRtvs        = numRtvs;
	m_rtvHeapStartOffset = rtvHeapOffset;
    m_needsDsv       = needsDsv;
	m_dsvHeapStartOffset = dsvHeapOffset;
	m_sampleBase     = Dx12SampleBase::GetInstance();
}

VOID DxGfxDrawRenderObject::RenderInitViewProjRtvDsv()
{
	auto* pCmdList = m_sampleBase->GetCmdList();
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_sampleBase->GetRtvCpuHandle(m_rtvHeapStartOffset);
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_sampleBase->GetDsvCpuHandle(m_dsvHeapStartOffset);
	pCmdList->OMSetRenderTargets(1,
		&rtvHandle,
		FALSE,                   //RTsSingleHandleToDescriptorRange
		&dsvHandle);
	pCmdList->ClearRenderTargetView(rtvHandle,
		m_sampleBase->RenderTargetClearColor().data(),
		0,
		nullptr);
	pCmdList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	pCmdList->SetGraphicsRootConstantBufferView(0, m_sampleBase->GetViewProjGpuVa());
}


VOID DxGfxDrawRenderObject::Render()
{
	m_sampleBase->RenderSceneGfxDraw();
}
