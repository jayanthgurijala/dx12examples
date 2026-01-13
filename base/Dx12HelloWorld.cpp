#include "Dx12HelloWorld.h"
#include <d3dx12.h>
#include <iostream>

#include "tiny_gltf.h"



Dx12HelloWorld::Dx12HelloWorld(UINT width, UINT height) :
	m_vertexBufferSizeInBytes(0),
	m_vertexStrideInBytes(0),
	Dx12SampleBase(width, height)
{
	m_fileReader = FileReader();
}

HRESULT Dx12HelloWorld::PreRun()
{
	HRESULT result = S_OK;

	result = CreatePipelineState();
	result = CreateAndLoadVertexBuffer();
	result = CreateAppResources();
	return result;
}


HRESULT Dx12HelloWorld::PostRun()
{
	return S_OK;
}

HRESULT Dx12HelloWorld::CreatePipelineStateFromModel()
{
	HRESULT result = S_OK;
	ID3D12Device* pDevice = GetDevice();

	const SIZE_T numAttributes = m_attributeNamesList.size();
	std::vector<D3D12_INPUT_ELEMENT_DESC> inputElementDescs;
	inputElementDescs.resize(numAttributes);

	for (UINT i = 0; i < numAttributes; i++)
	{
		inputElementDescs[i].SemanticName         = m_attributeNamesList[i].c_str();
		inputElementDescs[i].AlignedByteOffset    = 0;
		inputElementDescs[i].SemanticIndex        = 0;
		inputElementDescs[i].InputSlotClass       = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		inputElementDescs[i].InstanceDataStepRate = 0;
		inputElementDescs[i].Format               = m_attributeFormats[i];

		///@note depends on VB allocation. Need gltf to DX converter to give this info
		inputElementDescs[i].InputSlot = i;
	}

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = {};
	inputLayoutDesc.NumElements        = static_cast<UINT>(numAttributes);
	inputLayoutDesc.pInputElementDescs = inputElementDescs.data();

	//@todo Root signature
	//Root CBV
	//desc table -> SRV

	CD3DX12_ROOT_DESCRIPTOR cbvRootDesc;
	cbvRootDesc.Init(0);
	CD3DX12_ROOT_PARAMETER parameters[1];
	parameters[0].Descriptor = cbvRootDesc;
	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(1, parameters);

	ComPtr<ID3DBlob> error;
	ComPtr<ID3DBlob> signature;

	D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &signature, &error);
	pDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_modelRootSignature));


	return result;

}

HRESULT Dx12HelloWorld::CreatePipelineState()
{
	HRESULT       result  = S_OK;
	ID3D12Device* pDevice = GetDevice();

	ComPtr<ID3DBlob>            vertexShader;
	ComPtr<ID3DBlob>            pixelShader;

	//Create Root Signature
	{
		D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
		rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
		rootSignatureDesc.pParameters = nullptr;
		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;

		///@todo Error handling
		D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);

		pDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignarure));
	}


	//Create Shaders
	{
		///@todo compileFlags = 0 for release
		UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;

		std::wstring compiledVertexShaderPath = m_fileReader.GetFullAssetFilePath(L"VertexShader.cso");
		std::wstring compiledPixelShaderPath  = m_fileReader.GetFullAssetFilePath(L"PixelShader.cso");

		vertexShader = m_fileReader.LoadShaderBlobFromAssets(compiledVertexShaderPath);
		pixelShader = m_fileReader.LoadShaderBlobFromAssets(compiledPixelShaderPath);
	}


	D3D12_INPUT_ELEMENT_DESC inputElementDesc[1];
	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = {};
	{
		
		inputElementDesc[0].SemanticName         = "POSITION";							       //gltf: from primitive->arrtibute
		inputElementDesc[0].SemanticIndex        = 0;								           //gltf: mostly always 0 unless accessosor says > VEC4
		inputElementDesc[0].Format               = DXGI_FORMAT_R32G32B32_FLOAT; //x,y,z        //gltf: from accessor
		inputElementDesc[0].InputSlot            = 0;									       //gltf: from how buffer is allocated
		inputElementDesc[0].AlignedByteOffset    = 0;					                       //gltf: how VBs are allocated
		inputElementDesc[0].InputSlotClass       = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA; //gltf: all are per vertex
		inputElementDesc[0].InstanceDataStepRate = 0;                                          //gltf: always 0

		inputLayoutDesc.NumElements             = 1;
		inputLayoutDesc.pInputElementDescs      = inputElementDesc;
	}

	//Create pipeline state
	/*
	* 1. Input Layout
	* 2. Root Signature
	* 3. Shaders
	* 4. Rasterizer State
	* 5. Blend State
	* 6. DepthStencil State
	* 7. Sample Mask
	* 8. Primitive Topology
	* 9. RTVs and DSVs
	* 10. MSAA
	* 11. Flags, Cached PSOs
	*/
	{
		D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineStateDesc = {};
		pipelineStateDesc.InputLayout                        = inputLayoutDesc;
		pipelineStateDesc.pRootSignature                     = m_rootSignarure.Get();
		pipelineStateDesc.VS                                 = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
		pipelineStateDesc.PS                                 = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
		pipelineStateDesc.RasterizerState                    = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		pipelineStateDesc.BlendState                         = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		pipelineStateDesc.DepthStencilState                  = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

		///@todo write some test cases for this
		pipelineStateDesc.SampleMask                         = UINT_MAX;
		pipelineStateDesc.PrimitiveTopologyType              = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		pipelineStateDesc.NumRenderTargets                   = 1;
		pipelineStateDesc.RTVFormats[0]						 = GetBackBufferFormat();

		///@todo experiment with flags
		pipelineStateDesc.Flags                              = D3D12_PIPELINE_STATE_FLAG_NONE;
		
		pipelineStateDesc.SampleDesc.Count                   = 1;

		pDevice->CreateGraphicsPipelineState(&pipelineStateDesc, IID_PPV_ARGS(&m_pipelineState));
	}

	return result;
}

HRESULT Dx12HelloWorld::CreateAndLoadVertexBuffer()
{
	HRESULT result = S_OK;
	ComPtr<ID3D12Resource> stagingBuffer;
	D3D12_RESOURCE_STATES      vbState   = D3D12_RESOURCE_STATE_COPY_DEST;

	SimpleVertex triangleVertices[] =
	{
		{ { 0.0f, 0.25f * m_aspectRatio, 0.0f }, { 1.0f, 0.0f, 0.0f}, {0.0f,0.0f} },
		{ { 0.25f, -0.25f * m_aspectRatio, 0.0f }, { 0.0f, 1.0f, 0.0f}, {0.0f,0.0f} },
		{ { -0.25f, -0.25f * m_aspectRatio, 0.0f }, { 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f} }
	};

	const UINT vertexBufferSize = sizeof(triangleVertices);
	m_vertexBufferSizeInBytes   = vertexBufferSize;
	m_vertexStrideInBytes       = sizeof(SimpleVertex);

	m_vertexBuffer = CreateBufferWithData(triangleVertices, vertexBufferSize);

	return result;
}



HRESULT Dx12HelloWorld::CreateAppResources()
{
	HRESULT    result   = S_OK;
	const UINT numRTVs  = NumRTVsNeededForApp();
	result              = CreateRenderTargetResourceAndSRVs(numRTVs);
	result              = CreateRenderTargetViews(numRTVs, FALSE);
	TestTinyGLTFLoading();
	return result;
}

HRESULT Dx12HelloWorld::RenderFrame()
{
	ID3D12GraphicsCommandList*  pCmdList      = GetCmdList();
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle     = GetRenderTargetView(0, FALSE);
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
	
	vertexBufferView.BufferLocation           = m_vertexBuffer.Get()->GetGPUVirtualAddress();
	vertexBufferView.SizeInBytes              = m_vertexBufferSizeInBytes;
	vertexBufferView.StrideInBytes            = m_vertexStrideInBytes;
	
	FLOAT clearColor[4] = { 0.02f, 0.02f, 0.05f, 1.0f};
	
	pCmdList->OMSetRenderTargets(1,
		                         &rtvHandle,              //D3D12_CPU_DESCRIPTOR_HANDLE
		                         FALSE,                   //RTsSingleHandleToDescriptorRange
		                         nullptr);                //D3D12_CPU_DESCRIPTOR_HANDLE
	
	pCmdList->ClearRenderTargetView(rtvHandle,
		                            clearColor,
		                            0,
		                            nullptr);
	
	pCmdList->SetGraphicsRootSignature(m_rootSignarure.Get());
	pCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pCmdList->SetPipelineState(m_pipelineState.Get());
	pCmdList->IASetVertexBuffers(0, 1, &vertexBufferView);
	pCmdList->DrawInstanced(3, 1, 0, 0);

	RenderRtvContentsOnScreen(0);

	return S_OK;
}

HRESULT Dx12HelloWorld::TestTinyGLTFLoading()
{
	tinygltf::Model    model;
	tinygltf::TinyGLTF loader;
	std::string        err;
	std::string        warn;
	std::wstring modelPath = m_fileReader.GetFullAssetFilePath(L"deer.gltf");
	bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, FileReader::ToNarrowString(modelPath));
	if (!warn.empty())
	{
		std::cout << "Warn: " << warn << std::endl;
	}
	if (!err.empty())
	{
		std::cout << "Err: " << err << std::endl;
	}
	if (!ret)
	{
		std::cout << "Failed to parse glTF" << std::endl;
		return E_FAIL;
	}

	const tinygltf::Scene& firstScene   = model.scenes[0];
	const UINT firstNode          = firstScene.nodes[0];
	const tinygltf::Node& firstNodeDesc = model.nodes[0];
	const UINT firstMesh          = firstNodeDesc.mesh;
	
	///@todo Need to make cbuffer
	const BOOL hasScale           = (firstNodeDesc.scale.size() != 0);
	const BOOL hasTranslation	  = (firstNodeDesc.translation.size() != 0);
	const BOOL hasRotation        = (firstNodeDesc.rotation.size() != 0);
	const BOOL hasMatrix          = (firstNodeDesc.matrix.size() != 0);

	XMMATRIX modelMatrix;
	if (hasMatrix == FALSE)
	{
		XMVECTOR translation = (hasTranslation == TRUE) ? XMVectorSet((FLOAT)firstNodeDesc.translation[0], 
			                                                          (FLOAT)firstNodeDesc.translation[1],
			                                                          (FLOAT)firstNodeDesc.translation[2], 1.0f) : XMVectorZero();

		XMVECTOR rotation    = (hasRotation == TRUE) ? XMVectorSet((FLOAT)firstNodeDesc.rotation[0],
																   (FLOAT)firstNodeDesc.rotation[1],
			                                                       (FLOAT)firstNodeDesc.rotation[2],
																   (FLOAT)firstNodeDesc.rotation[3]) : XMQuaternionIdentity();

		XMVECTOR scale       = (hasScale == TRUE) ? XMVectorSet((FLOAT)firstNodeDesc.scale[0],
			                                                    (FLOAT)firstNodeDesc.scale[1],
			                                                    (FLOAT)firstNodeDesc.scale[2],
																1.0f) : XMVectorSplatOne();

		XMMATRIX T = XMMatrixTranslationFromVector(translation);
		XMMATRIX R = XMMatrixRotationQuaternion(rotation);
		XMMATRIX S = XMMatrixScalingFromVector(scale);

		modelMatrix = T * R * S;
	}
	else
	{
		const std::vector<double> m4x4 = firstNodeDesc.matrix;


		///@note gltf matrix is column major but DirectX Math expects row major.
		///      We could transpose it while construction but looks likt that is "error prone"
		///      Taking the safer approach.
		XMFLOAT4X4 temp = 
		{
			(FLOAT)m4x4[0], (FLOAT)m4x4[1], (FLOAT)m4x4[2], (FLOAT)m4x4[3],
			(FLOAT)m4x4[4], (FLOAT)m4x4[5], (FLOAT)m4x4[6], (FLOAT)m4x4[7],
			(FLOAT)m4x4[8], (FLOAT)m4x4[9], (FLOAT)m4x4[10], (FLOAT)m4x4[11],
			(FLOAT)m4x4[12], (FLOAT)m4x4[13], (FLOAT)m4x4[14], (FLOAT)m4x4[15]
		};

		XMMATRIX tempMat = XMLoadFloat4x4(&temp);
		modelMatrix      = XMMatrixTranspose(tempMat);
	}

	XMMATRIX mvpMatrix = GetMVPMatrix(modelMatrix);
	XMFLOAT4X4 cbData;
	XMStoreFloat4x4(&cbData, mvpMatrix);

	///@note constant buffer data layout
	/// MVP matrix (16 floats)
	const UINT constantBufferSizeInBytes = (sizeof(float) * 16);
	m_constantBufferResource = CreateBufferWithData(&cbData, constantBufferSizeInBytes);

	const tinygltf::Mesh& firstMeshDesc = model.meshes[firstMesh];
	const tinygltf::Primitive& firstPrimitive = firstMeshDesc.primitives[0];

	const tinygltf::Accessor& accessors = model.accessors[0];
	const SIZE_T numAccessors = model.accessors.size();

	m_attributeResources.resize(firstPrimitive.attributes.size());
	m_attributeBufferViews.resize(firstPrimitive.attributes.size());
	m_attributeNamesList.resize(firstPrimitive.attributes.size());
	m_attributeFormats.resize(firstPrimitive.attributes.size());

	std::vector<std::string> supportedAttributeNames;
	supportedAttributeNames.push_back("POSITION");
	supportedAttributeNames.push_back("NORMAL");
	supportedAttributeNames.push_back("TEXCOORD_0");
	supportedAttributeNames.push_back("TEXCOORD_1");
	supportedAttributeNames.push_back("TEXCOORD_2");

	auto attributeIt = supportedAttributeNames.begin();
	UINT attrIndx = 0;
	while (attributeIt != supportedAttributeNames.end())
	{
		auto it = firstPrimitive.attributes.find(*attributeIt);
		if (it != firstPrimitive.attributes.end())
		{
			const std::string attributeName = it->first;
			const int bufferViewIdx = it->second;
			const tinygltf::BufferView& bufViewDesc = model.bufferViews[bufferViewIdx];
			const int    bufferIdx = bufViewDesc.buffer;
			const size_t buflength = bufViewDesc.byteLength;
			const size_t bufOffset = bufViewDesc.byteOffset;
			const size_t bufStride = bufViewDesc.byteStride;

			size_t accessorByteOffset = 0;

			for (UINT i = 0; i < numAccessors; i++)
			{
				if (model.accessors[i].bufferView == bufferViewIdx)
				{
					m_attributeFormats[attrIndx] = GltfGetDxgiFormat(model.accessors[i].componentType, model.accessors[i].type);
					accessorByteOffset = model.accessors[i].byteOffset;
				}
			}

			const tinygltf::Buffer bufferDesc = model.buffers[bufferIdx];
			const unsigned char* attributedata = &bufferDesc.data[accessorByteOffset + bufOffset]; //upto buf length, makes up one resource
			m_attributeResources[attrIndx] = CreateBufferWithData((void*)attributedata, (UINT)buflength);

			m_attributeBufferViews[attrIndx].BufferLocation = m_attributeResources[attrIndx].Get()->GetGPUVirtualAddress();
			m_attributeBufferViews[attrIndx].SizeInBytes    = (UINT)buflength;
			m_attributeBufferViews[attrIndx].StrideInBytes  = (UINT)bufStride;

			m_attributeNamesList[attrIndx] = attributeName;

			attributeIt++;
			attrIndx++;

		}
	}

	return S_OK;
}

