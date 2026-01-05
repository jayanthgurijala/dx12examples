#pragma once


#include "Dx12SampleBase.h"
#include "FileReader.h"
#include "stdafx.h"

using namespace Microsoft::WRL;
using namespace DirectX;

class Dx12HelloWorld : public Dx12SampleBase
{
public:
	Dx12HelloWorld(UINT width, UINT height);
	virtual HRESULT PreRun() override;
	virtual HRESULT RenderFrame() override;
	virtual HRESULT PostRun() override;
protected:
	virtual inline UINT NumRTVsNeededForApp() override { return 1; }
private:

	HRESULT CreatePipelineState();		///< Create RootSignature, Compile Shaders, Create Pipeline State
	HRESULT CreateAppResources();
	HRESULT CreateAndLoadVertexBuffer();

	FileReader                  m_fileReader;

	ComPtr<ID3D12RootSignature> m_rootSignarure;
	ComPtr<ID3D12PipelineState> m_pipelineState;
	ComPtr<ID3D12Resource>      m_vertexBuffer;

	UINT                        m_vertexBufferSizeInBytes;
	UINT						m_vertexStrideInBytes;
};

