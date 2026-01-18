#include "Dx12HelloWorld.h"
#include <d3dx12.h>
#include <iostream>
#include "DxPrintUtils.h"
#include "DxGltfUtils.h"
#include "tiny_gltf.h"

using namespace DxGltfUtils;



Dx12HelloWorld::Dx12HelloWorld(UINT width, UINT height) :
	m_vertexBufferSizeInBytes(0),
	m_vertexStrideInBytes(0),
	m_modelExtents{},
	m_modelDrawPrimitive{},
	Dx12SampleBase(width, height)
{
	m_fileReader = FileReader();
}

HRESULT Dx12HelloWorld::PreRun()
{
	HRESULT result = S_OK;

	//result = CreatePipelineState();
	result = TestTinyGLTFLoading();
	result = CreatePipelineStateFromModel();
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

	const SIZE_T numAttributes = m_modelIaSemantics.size();
	std::vector<D3D12_INPUT_ELEMENT_DESC> inputElementDescs;
	inputElementDescs.resize(numAttributes);

	char vertexShaderName[64];
	char pixelShaderName[64];

	snprintf(vertexShaderName, 64, "Simple%zu_VS.cso", numAttributes);
	snprintf(pixelShaderName, 64, "Simple%zu_PS.cso", numAttributes);

	for (UINT i = 0; i < numAttributes; i++)
	{
		const UINT semanticIndex				  = m_modelIaSemantics[i].isIndexValid ? m_modelIaSemantics[i].index : 0;
		inputElementDescs[i].SemanticName         = m_modelIaSemantics[i].name.c_str();
		inputElementDescs[i].AlignedByteOffset    = 0;
		inputElementDescs[i].SemanticIndex        = semanticIndex;
		inputElementDescs[i].InputSlotClass       = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		inputElementDescs[i].InstanceDataStepRate = 0;
		inputElementDescs[i].Format               = m_modelIaSemantics[i].format;

		///@note depends on VB allocation. Need gltf to DX converter to give this info
		inputElementDescs[i].InputSlot = i;
	}

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = {};
	inputLayoutDesc.NumElements        = static_cast<UINT>(numAttributes);
	inputLayoutDesc.pInputElementDescs = inputElementDescs.data();

	//@todo Root signature
	//Root CBV
	//desc table -> SRV
	CD3DX12_ROOT_PARAMETER parameters[1];
	parameters[0].InitAsConstantBufferView(0);
	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(1, parameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> error;
	ComPtr<ID3DBlob> signature;

	D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
	pDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_modelRootSignature));

	m_modelPipelineState = GetGfxPipelineStateWithShaders(vertexShaderName, pixelShaderName, m_modelRootSignature.Get(), inputLayoutDesc, TRUE);

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

		std::string compiledVertexShaderPath = m_fileReader.GetFullAssetFilePath("VertexShader.cso");
		std::string compiledPixelShaderPath  = m_fileReader.GetFullAssetFilePath("PixelShader.cso");

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
	return result;
}

HRESULT Dx12HelloWorld::RenderFrame()
{

	BOOL renderTriangle = FALSE;

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
	
	if (renderTriangle == TRUE)
	{
		pCmdList->SetGraphicsRootSignature(m_rootSignarure.Get());
		pCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		pCmdList->SetPipelineState(m_pipelineState.Get());
		pCmdList->IASetVertexBuffers(0, 1, &vertexBufferView);
		pCmdList->DrawInstanced(3, 1, 0, 0);
	}
	else
	{
		pCmdList->SetGraphicsRootSignature(m_modelRootSignature.Get());

		///@todo get this from mode of primitive
		pCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		pCmdList->SetPipelineState(m_modelPipelineState.Get());
		pCmdList->SetGraphicsRootConstantBufferView(0, m_modelConstantBuffer->GetGPUVirtualAddress());
		
		const SIZE_T numVertexBufferViews = m_modelVbvs.size();
		D3D12_VERTEX_BUFFER_VIEW vbv = {};
		vbv.BufferLocation = m_modelVbvs[0].BufferLocation;
		vbv.SizeInBytes    = m_modelVbvs[0].SizeInBytes;
		vbv.StrideInBytes  = m_modelVbvs[0].StrideInBytes;
		assert(vbv.SizeInBytes > 0);
		assert(vbv.StrideInBytes > 0);
		assert(vbv.BufferLocation != 0);
		pCmdList->IASetVertexBuffers(0, numVertexBufferViews, &m_modelVbvs[0]);

		if (m_modelDrawPrimitive.isIndexedDraw == TRUE)
		{
			pCmdList->IASetIndexBuffer(&m_modelIbv);
			pCmdList->DrawIndexedInstanced(m_modelDrawPrimitive.numIndices, 1, 0, 0, 0);
		}
		else
		{
			pCmdList->DrawInstanced(m_modelDrawPrimitive.numVertices, 1, 0, 0);
		}
	}

	RenderRtvContentsOnScreen(0);

	return S_OK;
}

HRESULT Dx12HelloWorld::CreateSceneMVPMatrix()
{
	HRESULT result = S_OK;

	XMMATRIX modelMatrix;
	
	const std::vector<double>& meshTranslation = m_meshTransformInfo.translation;
	const std::vector<double>& meshRotation    = m_meshTransformInfo.rotation;
	const std::vector<double>& meshScale       = m_meshTransformInfo.scale;

	if (m_meshTransformInfo.hasMatrix == FALSE)
	{
		XMVECTOR translation = (m_meshTransformInfo.hasTranslation == TRUE) ? XMVectorSet((FLOAT)meshTranslation[0],
																						  (FLOAT)meshTranslation[1],
																						  (FLOAT)meshTranslation[2], 1.0f) : XMVectorZero();

		//XMVECTOR rotation =  XMMatrixIdentity();

		XMVECTOR scale = (m_meshTransformInfo.hasScale == TRUE) ? XMVectorSet((FLOAT)meshScale[0],
																			  (FLOAT)meshScale[1],
																			  (FLOAT)meshScale[2],
																			  1.0f) : XMVectorSplatOne();

		XMMATRIX T = XMMatrixTranslationFromVector(translation);
		XMMATRIX R = XMMatrixRotationY(XM_PIDIV2);
		XMMATRIX S = XMMatrixScalingFromVector(scale);

		modelMatrix = S * R * T;
		PrintUtils::PrintXMMatrix("Model Matrix", modelMatrix);
	}
	else
	{
		const std::vector<double>& m4x4 = m_meshTransformInfo.matrix;


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
		modelMatrix = XMMatrixTranspose(tempMat);
	}

	XMMATRIX mvpMatrix = GetMVPMatrix(modelMatrix);
	XMFLOAT4X4 cbData;
	XMStoreFloat4x4(&cbData, mvpMatrix);
	
	PrintUtils::PrintXMMatrix("Final MVP Matrix", mvpMatrix);
	

	///@note constant buffer data layout
	/// MVP matrix (16 floats)
	const UINT constantBufferSizeInBytes = (sizeof(float) * 16);
	m_modelConstantBuffer = CreateBufferWithData(&cbData, constantBufferSizeInBytes);

	return result;
}

XMMATRIX Dx12HelloWorld::GetMVPMatrix(XMMATRIX& modelMatrix)
{
	const FLOAT width = (FLOAT)GetWidth();
	const UINT  height      = GetHeight();
	const FLOAT aspectRatio = ((FLOAT)width) / height;

	assert(m_modelExtents.hasValidExtents == TRUE);

	///@note FOV, width/height, near and far clipping plane.
	const XMMATRIX projection = XMMatrixPerspectiveFovLH(XMConvertToRadians(45.0f),
														 aspectRatio,
														 0.1f,
														 100.0f);

	PrintUtils::PrintXMMatrix("Projection Matrix", projection);

	const XMVECTOR minExtent = XMVectorSet(m_modelExtents.min[0], m_modelExtents.min[1], m_modelExtents.min[2], 1.0f);
	const XMVECTOR maxExtent = XMVectorSet(m_modelExtents.max[0], m_modelExtents.max[1], m_modelExtents.max[2], 1.0f);

	const XMVECTOR modelCenter  = (minExtent + maxExtent) / 2.0f;
	const XMVECTOR modelDiagnol = (maxExtent - minExtent);
	
	const float diagLength = XMVectorGetX(XMVector3Length(modelDiagnol)) * 1.5f;
	const XMVECTOR cameraPosition = modelCenter - XMVectorSet(0.0f, diagLength, 0.0f, 0.0f);


	///@note camera position, camera forward vector, up direction
	///@note this is stored in row-major order
	const XMMATRIX view = XMMatrixLookAtLH(XMVectorSet(0,0,-15,0),
		                             XMVectorZero(),
		                             XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));

	PrintUtils::PrintXMMatrix("View Matrix", view);

	const XMMATRIX mvpMatrix = modelMatrix * view * projection;

	PrintUtils::PrintXMMatrix("MVP without Transpose", mvpMatrix);

	const XMMATRIX finalMvp = XMMatrixTranspose(mvpMatrix);

	return finalMvp;

	//XMMATRIX model = XMMatrixScaling(0.389073f, 0.389073f, 0.389073f)	*
	//	             XMMatrixTranslation(0.f, 0.078706f, 0.304987f);

	//XMMATRIX model = XMMatrixScaling(1.0f, 1.0f, 1.0f) *
	//	             XMMatrixTranslation(0.f, 0.0f, 0.0f);

	//XMMATRIX view = XMMatrixTranslation(0.f, 0.f, 15.f); // camera at z=-15

	//XMMATRIX proj = XMMatrixPerspectiveFovLH(
	//	XM_PIDIV4, 16.f / 9.f, 0.1f, 100.f
	//);

	//XMMATRIX mvpMatrix = model * view * proj;

	//XMMATRIX finalMvp = XMMatrixTranspose(mvpMatrix);

	//PrintUtils::PrintXMMatrix("Model Matrix", model);
	//PrintUtils::PrintXMMatrix("View  Matrix", view);
	//PrintUtils::PrintXMMatrix("Projection Matrix", proj);
	//PrintUtils::PrintXMMatrix("MVP Matrix", mvpMatrix);
	//PrintUtils::PrintXMMatrix("Final MVP Matrix", finalMvp);

	//return model;
}

HRESULT Dx12HelloWorld::TestTinyGLTFLoading()
{
	HRESULT result = S_OK;
	tinygltf::Model    model;
	tinygltf::TinyGLTF loader;
	std::string        err;
	std::string        warn;
	std::string modelPath = m_fileReader.GetFullAssetFilePath("deer.gltf");
	bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, modelPath);
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
		result = E_FAIL;
	}

	if (result == S_OK)
	{
		const tinygltf::Scene& firstScene = model.scenes[0];
		const UINT firstNode = firstScene.nodes[0];
		const tinygltf::Node& firstNodeDesc = model.nodes[0];
		const UINT firstMesh = firstNodeDesc.mesh;

		///@todo Need to make cbuffer
		m_meshTransformInfo.hasScale       = (firstNodeDesc.scale.size() != 0);
		m_meshTransformInfo.hasTranslation = (firstNodeDesc.translation.size() != 0);
		m_meshTransformInfo.hasRotation    = (firstNodeDesc.rotation.size() != 0);
		m_meshTransformInfo.hasMatrix      = (firstNodeDesc.matrix.size()   != 0);
		m_meshTransformInfo.translation    = firstNodeDesc.translation;
		m_meshTransformInfo.rotation       = firstNodeDesc.rotation;
		m_meshTransformInfo.scale          = firstNodeDesc.scale;

		const BOOL hasMatrix = (firstNodeDesc.matrix.size() != 0);

		

		const tinygltf::Mesh& firstMeshDesc = model.meshes[firstMesh];
		const tinygltf::Primitive& firstPrimitive = firstMeshDesc.primitives[0];

		const tinygltf::Accessor& accessors = model.accessors[0];
		const SIZE_T numAccessors = model.accessors.size();

		///@note load IA Layout and VB info and resources
		{
			m_modelVbBuffers.resize(firstPrimitive.attributes.size());
			m_modelVbvs.resize(firstPrimitive.attributes.size());
			m_modelIaSemantics.resize(firstPrimitive.attributes.size());

			std::string positionAttributeName = "POSITION";

			std::vector<std::string> supportedAttributeNames;
			supportedAttributeNames.push_back("POSITION");
			supportedAttributeNames.push_back("NORMAL");
			supportedAttributeNames.push_back("TEXCOORD_0");
			supportedAttributeNames.push_back("TEXCOORD_1");
			supportedAttributeNames.push_back("TEXCOORD_2");

			//primitive -> attribute names -> accessor index -> accessor -> bufferview -> buffer
			auto attributeIt = supportedAttributeNames.begin();
			UINT attrIndx = 0;
			SIZE_T numTotalVertices = 0;
			while (attributeIt != supportedAttributeNames.end())
			{
				auto it = firstPrimitive.attributes.find(*attributeIt);
				if (it != firstPrimitive.attributes.end())
				{
					const std::string attributeName = it->first;
					const int accessorIdx = it->second;
					const tinygltf::Accessor& accessorDesc = model.accessors[accessorIdx];
					const UINT componentDataType    = accessorDesc.componentType; 
					const UINT componentVecType     = accessorDesc.type;
					const UINT componentSizeInBytes = GetComponentTypeSizeInBytes(componentDataType);
					const UINT numComponents        = GetNumComponentsInType(componentVecType);

					///@note for POSITION attriubte, extents are needed to setup camera
					if (attributeName == positionAttributeName)
					{
						std::vector<double> maxValues = accessorDesc.maxValues;
						std::vector<double> minValues = accessorDesc.minValues;
						memcpy(m_modelExtents.min, accessorDesc.minValues.data(), sizeof(m_modelExtents.min));
						memcpy(m_modelExtents.max, accessorDesc.maxValues.data(), sizeof(m_modelExtents.max));
						m_modelExtents.hasValidExtents = TRUE;
					}
					const int bufferViewIdx = accessorDesc.bufferView;
					const tinygltf::BufferView bufViewDesc = model.bufferViews[bufferViewIdx];

					///@todo need to support interleaved data
					assert(bufViewDesc.byteStride == 0);

					const int    bufferIdx = bufViewDesc.buffer;
					const size_t buflength = bufViewDesc.byteLength;
					const size_t bufOffset = bufViewDesc.byteOffset;

					const size_t accessorByteOffset = accessorDesc.byteOffset;
					m_modelIaSemantics[attrIndx].format = GltfGetDxgiFormat(componentDataType, componentVecType);
					numTotalVertices += accessorDesc.count;
					const tinygltf::Buffer bufferDesc = model.buffers[bufferIdx];
					const unsigned char* attributedata = bufferDesc.data.data() + accessorByteOffset + bufOffset; //upto buf length, makes up one resource
					m_modelVbBuffers[attrIndx] = CreateBufferWithData((void*)attributedata, (UINT)buflength);

					m_modelVbvs[attrIndx].BufferLocation = m_modelVbBuffers[attrIndx].Get()->GetGPUVirtualAddress();
					m_modelVbvs[attrIndx].SizeInBytes = (UINT)buflength;
					m_modelVbvs[attrIndx].StrideInBytes = componentSizeInBytes * numComponents;

					auto& currentSemantic = m_modelIaSemantics[attrIndx];
					///@todo make a string utils class to check for stuff
					auto pos = attributeName.find("_");

					if (pos != std::string::npos)
					{
						std::string left = attributeName.substr(0, pos);
						std::string right = attributeName.substr(pos + 1);
						int index = std::stoi(right);
						currentSemantic.name = left;
						currentSemantic.index = index;
						currentSemantic.isIndexValid = TRUE;
					}
					else
					{
						currentSemantic.name = attributeName;
						currentSemantic.isIndexValid = FALSE;
					}
					attrIndx++;
				}
				attributeIt++;
			}
			m_modelDrawPrimitive.numVertices = (UINT)(numTotalVertices);
		}

		//load index buffers
		{
			const int bufViewIdx = firstPrimitive.indices;
			if (bufViewIdx != -1)
			{
				const tinygltf::Accessor& accessorDesc = model.accessors[bufViewIdx];
				const int bufferViewIdx = accessorDesc.bufferView;
				const tinygltf::BufferView& bufViewDesc = model.bufferViews[bufferViewIdx];
				const int bufferIdx = bufViewDesc.buffer;
				const tinygltf::Buffer bufferDesc = model.buffers[bufferIdx];

				const size_t accessorByteOffset = accessorDesc.byteOffset;
				const UINT bufferSizeInBytes = (UINT)bufViewDesc.byteLength;
				const size_t bufferViewOffset = bufViewDesc.byteOffset;

				const size_t byteOffsetIntoBuffer = accessorByteOffset + bufferViewOffset;


				m_modelIbBuffer = CreateBufferWithData((void*)(bufferDesc.data.data() + byteOffsetIntoBuffer), bufferSizeInBytes);
				m_modelIbv.BufferLocation = m_modelIbBuffer->GetGPUVirtualAddress();
				m_modelIbv.Format = GltfGetDxgiFormat(accessorDesc.componentType, accessorDesc.type);
				m_modelIbv.SizeInBytes = bufferSizeInBytes;
				m_modelDrawPrimitive.isIndexedDraw = TRUE;
				m_modelDrawPrimitive.numIndices = (UINT)accessorDesc.count;
			}
			else
			{
				m_modelDrawPrimitive.isIndexedDraw = FALSE;
			}
		}
	}

	CreateSceneMVPMatrix();

	return result;
}


