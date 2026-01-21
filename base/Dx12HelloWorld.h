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
	virtual inline UINT NumSRVsNeededForApp() override { return 1; }
private:

	HRESULT CreatePipelineState();		///< Create RootSignature, Compile Shaders, Create Pipeline State
	HRESULT CreatePipelineStateFromModel();
	HRESULT CreateAppResources();
	HRESULT CreateAndLoadVertexBuffer();
	HRESULT CreateSceneMVPMatrix();
	XMMATRIX GetMVPMatrix(XMMATRIX& modelMatrix);

	HRESULT TestTinyGLTFLoading();

	FileReader                  m_fileReader;

	ComPtr<ID3D12RootSignature> m_rootSignarure;
	ComPtr<ID3D12PipelineState> m_pipelineState;
	ComPtr<ID3D12Resource>      m_vertexBuffer;

	UINT                        m_vertexBufferSizeInBytes;
	UINT						m_vertexStrideInBytes;

	///@todo model resources refactor
	std::vector<ComPtr<ID3D12Resource>>   m_modelVbBuffers;
	ComPtr<ID3D12Resource>                m_modelIbBuffer;
	std::vector<D3D12_VERTEX_BUFFER_VIEW> m_modelVbvs;
	D3D12_INDEX_BUFFER_VIEW               m_modelIbv;
	std::vector<DxIASemantic>             m_modelIaSemantics;
	ComPtr<ID3D12Resource>                m_mvpCameraConstantBuffer;
	ComPtr<ID3D12RootSignature>           m_modelRootSignature;
	ComPtr<ID3D12PipelineState>           m_modelPipelineState;
	DxDrawPrimitive                       m_modelDrawPrimitive;
	DxMeshNodeTransformInfo               m_meshTransformInfo;
	DxExtents							  m_modelExtents;
	ComPtr<ID3D12Resource>                m_modelBaseColorTex2D;
};

