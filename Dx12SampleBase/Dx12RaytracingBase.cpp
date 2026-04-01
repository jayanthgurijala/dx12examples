#include "pch.h"
#include "Dx12RaytracingBase.h"


static const wchar_t* const c_pSimpleRGShader = L"MyRaygenShader";

static const wchar_t* const c_pCHSTexColShader = L"CHSBaseColorTexturing";
static const wchar_t* const c_pCHSShadowShader = L"CHSShadow";

static const wchar_t* const c_pAHSAlphaCutOff = L"AHSAlphaCutOff";

static const wchar_t* const c_pSimpleMissShader = L"MyMissShader";
static const wchar_t* const c_pShadowMissShader = L"ShadowMissShader";

static const wchar_t* const c_pHitGroupOpaque = L"hitgroup_opaque";
static const wchar_t* const c_pHitGroupAlphaCutOff = L"hitgroup_alphacutoff";

static const wchar_t* const c_pHitGroupOpaqueShadow = L"hitgroup_opaque_shadow";
static const wchar_t* const c_pHitGroupAlphaCutOffShadow = L"hitgroup_alphacutoff_shadow";

struct RayPayload
{
	float color[4];
	UINT currentRecursionDepth;
};

VOID Dx12RaytracingBase::OnInit()
{
	Dx12SampleBase::OnInit();

	ID3D12Device* pDevice = GetDevice();
	assert(pDevice != nullptr);

	ID3D12GraphicsCommandList* pCmdList = GetCmdList();
	assert(pCmdList != nullptr);

	pDevice->QueryInterface(IID_PPV_ARGS(&m_dxrDevice));
	assert(m_dxrDevice != nullptr);

	pCmdList->QueryInterface(IID_PPV_ARGS(&m_dxrCommandList));
	assert(m_dxrCommandList != nullptr);

	CreateGlobalRootSignature();
	CreateLocalRootSignature();
	CreatePerPrimSrvs();
	CreateRayTracingStateObject();

	BuildBlasAndTlas();
	BuildShaderTables();
	CreateUAVOutput();
}


Dx12RaytracingBase::Dx12RaytracingBase(UINT width, UINT height) :
	Dx12SampleBase(width, height)
{
}

/*
* Simply put, global root signature has everything needed by ray gen shader
*
* @todo Sampler state is used by closest hit, check if this needs to be moved.
*
*/
VOID Dx12RaytracingBase::CreateGlobalRootSignature()
{
	//@todo gltf description has sampler info for each texture
	CD3DX12_STATIC_SAMPLER_DESC staticSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR);

	const UINT numUavsInScene = NumUAVsNeededForApp();

	///@note One range for model meterial SRVs(space = 0), one range for vertex buffer SRV and index buffer srv (space = 1), and one for UAV
	const UINT numDescTableRanges = 1;
	std::vector<CD3DX12_DESCRIPTOR_RANGE> descTableRanges(numDescTableRanges);

	///@note In Raytracing, root signature needs to be divided into local and global root signature.
	///      Camera Data, TLAS, UAV Output and sampler (for now) - part of global root sig

	//output UAV
	descTableRanges[0] = dxhelper::GetUAVDescRange(numUavsInScene, 0, 0);
	auto rootDescriptorTable = dxhelper::GetRootDescTable(descTableRanges);

	//scene tlas
	auto rootSrv = dxhelper::GetRootSrv(0, 2); //tlas srv at register space 2

	//View Projection Matrix
	auto viewProj = dxhelper::GetRootCbv(0);    //camera buffer at register space 0

	//World Matrices
	auto perInstance = dxhelper::GetRootCbv(1);    //camera buffer at register space 0


	dxhelper::DxCreateRootSignature
	(
		m_dxrDevice.Get(),

		//camera buffer, texture stv, output uav, AS, vertex buffer, index buffer
		&m_rootSignature,
		{
			viewProj,
			perInstance,
			rootDescriptorTable,
			rootSrv
		},
		{ staticSampler }
	);
}

/*
*
* PerPrim Srvs
* @todo need to add material buffer
*
*/
VOID Dx12RaytracingBase::CreateLocalRootSignature()
{
	const UINT numDescTableRanges = 1;
	const UINT numSRVsPerPrim = NumSRVsPerPrimitive();
	const UINT registerSpace = 3;

	//one SRV descriptor table with numSRVsPerPrim primitives
	std::vector<CD3DX12_DESCRIPTOR_RANGE> descTableRanges(numDescTableRanges);

	descTableRanges[0] = dxhelper::GetSRVDescRange(numSRVsPerPrim, 0, registerSpace);
	auto rootDescriptorTable = dxhelper::GetRootDescTable(descTableRanges);
	auto rootCbv = dxhelper::GetRootCbv(0, registerSpace);

	dxhelper::DxCreateRootSignature
	(
		m_dxrDevice.Get(),
		&m_localRootSignature,
		{
			rootCbv,
			rootDescriptorTable
		},
		{},
		D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE
	);
}

/*
*
* SRVs needed for shading - PBR, Emission, etc textures and SRVs are created at earlier.
* RayTracing needs additional information for e.g. mip level selection
*  - Index Buffer
*  - Vertex Buffer
*  - UV BUffer
*/
VOID Dx12RaytracingBase::CreatePerPrimSrvs()
{
	assert(m_localRootSignature != nullptr);

	const UINT numTotalSrvsPerPrim = NumSRVsPerPrimitive();
	const UINT appSrvOffset = AppSrvOffsetForPrim();

	///create the extra two SRV descriptors for raytracing
	ForEachSceneElementLoadedSceneNodePrim([this, appSrvOffset](UINT sceneIdx, UINT nodeIdx, UINT primIdx)
		{
			const auto& curPrimitive = GetPrimitiveInfo(sceneIdx, nodeIdx, primIdx);
			CreateSrvBufferForPrimitive(curPrimitive, GltfVertexAttribPosition, appSrvOffset + 0);
			CreateSrvBufferForPrimitive(curPrimitive, GltfVertexAttribNormal, appSrvOffset + 1);
			CreateSrvBufferForPrimitive(curPrimitive, GltfVertexAttribTexcoord0, appSrvOffset + 2);

			///@note Index buffer
			CreateSrvBufferForPrimitive(curPrimitive, GltfVertexAttribMax, appSrvOffset + 3, TRUE);
		});
}


VOID Dx12RaytracingBase::CreateRayTracingStateObject()
{
	ComPtr<ID3DBlob> compiledShaders = GetCompiledShaderBlob("RaytraceSimpleCHS.cso");

	CD3DX12_STATE_OBJECT_DESC rayTracingPipelineDesc{ D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE };

	auto libSubObject = rayTracingPipelineDesc.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
	D3D12_SHADER_BYTECODE libdxil = CD3DX12_SHADER_BYTECODE(compiledShaders->GetBufferPointer(), compiledShaders->GetBufferSize());
	libSubObject->SetDXILLibrary(&libdxil);

	auto AddExport = [&libSubObject](const wchar_t* name)
		{
			libSubObject->DefineExport(name, nullptr, D3D12_EXPORT_FLAG_NONE);
		};

	AddExport(c_pSimpleRGShader);

	AddExport(c_pCHSTexColShader);
	AddExport(c_pCHSShadowShader);

	AddExport(c_pAHSAlphaCutOff);

	AddExport(c_pSimpleMissShader);
	AddExport(c_pShadowMissShader);

	auto AddHitGroupSubObject = [&rayTracingPipelineDesc](const wchar_t* hitgroupExportName, const wchar_t* intersectionShaderName, const wchar_t* chsName, const wchar_t* ahsName)
		{
			assert(hitgroupExportName != nullptr);
			auto hitGroupSubObject = rayTracingPipelineDesc.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
			hitGroupSubObject->SetIntersectionShaderImport(intersectionShaderName);
			hitGroupSubObject->SetClosestHitShaderImport(chsName);
			hitGroupSubObject->SetAnyHitShaderImport(ahsName);
			hitGroupSubObject->SetHitGroupExport(hitgroupExportName);
		};

	AddHitGroupSubObject(c_pHitGroupOpaque, nullptr, c_pCHSTexColShader, nullptr);
	AddHitGroupSubObject(c_pHitGroupOpaqueShadow, nullptr, c_pCHSShadowShader, nullptr);

	AddHitGroupSubObject(c_pHitGroupAlphaCutOff, nullptr, c_pCHSTexColShader, c_pAHSAlphaCutOff);
	AddHitGroupSubObject(c_pHitGroupAlphaCutOffShadow, nullptr, c_pCHSShadowShader, c_pAHSAlphaCutOff);

	auto shaderConfigSubObject = rayTracingPipelineDesc.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
	const UINT payloadSize = sizeof(RayPayload); //ray payload
	const UINT attributeSize = sizeof(FLOAT) * 2; //bary centrics
	shaderConfigSubObject->Config(payloadSize, attributeSize);

	auto globalRootSigSubObject = rayTracingPipelineDesc.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
	globalRootSigSubObject->SetRootSignature(m_rootSignature.Get());

	auto pipelineConfigSubObject = rayTracingPipelineDesc.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
	const UINT maxRecursionDepth = MaxRecursionDepth();
	pipelineConfigSubObject->Config(maxRecursionDepth);

	auto localRootSigSubObject = rayTracingPipelineDesc.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
	localRootSigSubObject->SetRootSignature(m_localRootSignature.Get());

	auto localRootSigAssociationSubObj = rayTracingPipelineDesc.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
	localRootSigAssociationSubObj->AddExport(c_pHitGroupAlphaCutOff);
	localRootSigAssociationSubObj->AddExport(c_pHitGroupOpaque);
    localRootSigAssociationSubObj->AddExport(c_pHitGroupOpaqueShadow);
    localRootSigAssociationSubObj->AddExport(c_pHitGroupAlphaCutOffShadow);
	localRootSigAssociationSubObj->SetSubobjectToAssociate(*localRootSigSubObject);


	m_dxrDevice->CreateStateObject(rayTracingPipelineDesc, IID_PPV_ARGS(&m_rtpso));

	assert(m_rtpso != nullptr);
}


VOID Dx12RaytracingBase::BuildShaderTables()
{

	ComPtr<ID3D12StateObjectProperties> props;
	m_rtpso->QueryInterface(IID_PPV_ARGS(&props));

	auto GetHitGroupShaderIdentifier = [](ComPtr<ID3D12StateObjectProperties> props, BOOL needsAnyHit, UINT rayTypeIdx)
		{
			void* hitGroupID = nullptr;
			//rayTypeIdx 0 for radiance ray and 1 for shadow ray
			if (rayTypeIdx == 0)
			{
				hitGroupID = (needsAnyHit == TRUE) ? props->GetShaderIdentifier(c_pHitGroupAlphaCutOff) : props->GetShaderIdentifier(c_pHitGroupOpaque);
			}
			else
			{
				hitGroupID = (needsAnyHit == TRUE) ? props->GetShaderIdentifier(c_pHitGroupAlphaCutOffShadow) : props->GetShaderIdentifier(c_pHitGroupOpaqueShadow);
			}

		    assert(hitGroupID != nullptr);

		    return hitGroupID;
		};

	auto GetMissShaderIdentifier = [](ComPtr<ID3D12StateObjectProperties> props, UINT missIndex)
		{
			void* missShaderID = nullptr;
			if (missIndex == 0)
			{
				missShaderID = props->GetShaderIdentifier(c_pSimpleMissShader);
			}
			else
			{
				missShaderID = props->GetShaderIdentifier(c_pShadowMissShader);
			}
			assert(missShaderID != nullptr);
			return missShaderID;
		};


	/// -Scene Load Elements (1)Deer (2)OakTree (3)Terrain
	/// -Scene can be composed or X Deers, Y OakTrees and Z Terrains
	/// How many SBTs should hit groups have?
	/// To answer this question, need to understand how indexing works.
	/// Ray contributions in Trace Ray offset and allot blocks of hitgroups
	/// Instance Contribution = sceneloadElementIdx 
	///  so Deer = 0, OakTree = 1 and 2, Terrain = 3 (order of scene load elements but then constant)
	/// so even if we have X,Y,Z Deers, OakTrees and Terrains, we will ever need only 4 SBT entries, i.e. as many prims.

	std::vector<DxPrimitiveInfo*> primitiveInfo;
	UINT numSceneLoadElements = NumElementsInSceneLoad();
	for (UINT sceneIdx = 0; sceneIdx < numSceneLoadElements; sceneIdx++)
	{
        auto& info = SceneElementInstance(sceneIdx);
		const UINT sceneElementIdx = info.sceneElementIdx;
		UINT numNodesInSceneElement = NumNodesInScene(sceneElementIdx);
		for (UINT nodeIdx = 0; nodeIdx < numNodesInSceneElement; nodeIdx++)
		{
			UINT numPrimsInNodeMesh = NumPrimitivesInNodeMesh(sceneElementIdx, nodeIdx);
			for (UINT primIdx = 0; primIdx < numPrimsInNodeMesh; primIdx++)
			{
				primitiveInfo.push_back(&GetPrimitiveInfo(sceneElementIdx, nodeIdx, primIdx));
			}
		}
	}


	///@note Every primitive has a material and needs an SBT.
	///      There are two hitgroups for now - opaque and transparent
	const UINT totalPrimsInScene   = primitiveInfo.size();
	const UINT numHitGroupsPerPrim = 2; //radiance and shadow ray
    const UINT numMissShaders      = 2; //radiance ray miss shader and shadow ray miss shader



	const UINT shaderRecordSize = D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT; // 32 bytes
	const UINT sectionAlignment = D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;  //64 bytes alignment

	UINT64 rayGenTableSize = shaderRecordSize;
	UINT64 rayGenAlignedSize = dxhelper::DxAlign(rayGenTableSize, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);

	UINT64 hitGroupTableSize = shaderRecordSize + sizeof(D3D12_GPU_VIRTUAL_ADDRESS) + sizeof(D3D12_GPU_DESCRIPTOR_HANDLE);
	UINT64 hitGroupAlignedSize = dxhelper::DxAlign(hitGroupTableSize, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
	UINT64 hitGroupTotalSizeInBytes = hitGroupAlignedSize * totalPrimsInScene * numHitGroupsPerPrim;

	UINT64 missTableSize = shaderRecordSize;
	UINT64 missTableAlignedSize = dxhelper::DxAlign(missTableSize, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
    UINT64 missTableTotalSizeInBytes = missTableAlignedSize * numMissShaders;

	const UINT64 sbtTableTotalSize = rayGenAlignedSize + hitGroupTotalSizeInBytes + missTableTotalSizeInBytes;
	BYTE* const sbtDataStart = static_cast<BYTE*>(malloc(sbtTableTotalSize));
	BYTE* sbtDataWritePtr = sbtDataStart;


	void* raygenShaderID = props->GetShaderIdentifier(c_pSimpleRGShader);
	assert(raygenShaderID != nullptr);
	memcpy(sbtDataWritePtr, raygenShaderID, rayGenTableSize);
	sbtDataWritePtr += rayGenAlignedSize;

	assert((sbtDataWritePtr - sbtDataStart) == rayGenAlignedSize);
	BYTE* const pHitGroupStartWritePtr = sbtDataWritePtr;
	{
		for (UINT primIdx = 0; primIdx < totalPrimsInScene; primIdx++)
		{
			for (UINT rayIndex = 0; rayIndex < numHitGroupsPerPrim; rayIndex++)
			{
                UINT hitgroupIndex = (primIdx * numHitGroupsPerPrim + rayIndex);

				auto* curPrimitive = primitiveInfo[primIdx];
				BOOL  needsAnyHit = IsPrimitiveTransparent(*curPrimitive);
				auto  gpuVAMaterialsCB = curPrimitive->materialTextures.meterialCb;
				auto  gpuVAMatTex = GetPerPrimSrvGpuHandle(curPrimitive->primLinearIdxInSceneElements);

                void* curHitGroupID = GetHitGroupShaderIdentifier(props, needsAnyHit, rayIndex);

				sbtDataWritePtr = pHitGroupStartWritePtr + hitGroupAlignedSize * hitgroupIndex;
				memcpy(sbtDataWritePtr, curHitGroupID, hitGroupTableSize);
				sbtDataWritePtr += shaderRecordSize;
				assert(pHitGroupStartWritePtr + hitgroupIndex * hitGroupAlignedSize + shaderRecordSize == sbtDataWritePtr);
				memcpy(sbtDataWritePtr, &gpuVAMaterialsCB, sizeof(gpuVAMaterialsCB));
				sbtDataWritePtr += sizeof(gpuVAMaterialsCB);
				assert(pHitGroupStartWritePtr + hitgroupIndex * hitGroupAlignedSize + shaderRecordSize + sizeof(gpuVAMaterialsCB) == sbtDataWritePtr);
				memcpy(sbtDataWritePtr, &gpuVAMatTex, sizeof(gpuVAMatTex));
				sbtDataWritePtr += sizeof(gpuVAMatTex);

				assert(sbtDataWritePtr == pHitGroupStartWritePtr + hitgroupIndex * hitGroupAlignedSize + hitGroupTableSize);
			}
		}
	}

	sbtDataWritePtr = pHitGroupStartWritePtr + hitGroupAlignedSize * totalPrimsInScene * numHitGroupsPerPrim;

	assert(sbtDataWritePtr == sbtDataStart + rayGenAlignedSize + hitGroupAlignedSize * totalPrimsInScene * numHitGroupsPerPrim);
	memcpy(sbtDataWritePtr, GetMissShaderIdentifier(props, 0), missTableSize);
    sbtDataWritePtr += missTableAlignedSize;
	memcpy(sbtDataWritePtr, GetMissShaderIdentifier(props, 1), missTableSize);
    sbtDataWritePtr += missTableAlignedSize;

    assert(sbtDataWritePtr == sbtDataStart + rayGenAlignedSize + hitGroupAlignedSize * totalPrimsInScene * numHitGroupsPerPrim + missTableAlignedSize * numMissShaders);

	m_shaderBindingTable = CreateBufferWithData(sbtDataStart, sbtTableTotalSize, "ShaderBindingTable");

	m_rayGenBaseAddress.StartAddress = m_shaderBindingTable->GetGPUVirtualAddress();
	m_rayGenBaseAddress.SizeInBytes = rayGenTableSize;

	m_hitTableBaseAddress.StartAddress = m_rayGenBaseAddress.StartAddress + rayGenAlignedSize;
	m_hitTableBaseAddress.SizeInBytes = hitGroupTotalSizeInBytes;
	m_hitTableBaseAddress.StrideInBytes = hitGroupAlignedSize;

	m_missTableBaseAddress.StartAddress = m_hitTableBaseAddress.StartAddress + hitGroupTotalSizeInBytes;
	m_missTableBaseAddress.SizeInBytes = missTableTotalSizeInBytes;
	m_missTableBaseAddress.StrideInBytes = missTableAlignedSize;

	free(sbtDataStart);
}

VOID Dx12RaytracingBase::CreateUAVOutput()
{
	dxhelper::AllocateTexture2DResource(m_dxrDevice.Get(),
		&m_uavOutputResource,
		GetWidth(),
		GetHeight(),
		GetBackBufferFormat(),
		L"RayTracingUAVOutput",
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nullptr,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	//@todo this is a bit confusing beacuse UAVs are created after all SRVs.
	// in this case after srvs for materials and ray tracing srvs.
	CreateAppUavDescriptorAtIndex(0, m_uavOutputResource.Get());
}

VOID Dx12RaytracingBase::BuildBlasAndTlas()
{
	StartBuildingAccelerationStructures();

	const D3D12_RESOURCE_STATES resultState = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;

	//@note should this be in UAV? Sample apps use UAV but there is DebugLayer warning
	const D3D12_RESOURCE_STATES scratchState = D3D12_RESOURCE_STATE_COMMON;

	const UINT numSceneElements = NumElementsInSceneLoad();
	for (UINT sceneElemIdx = 0; sceneElemIdx < numSceneElements; sceneElemIdx++)
	{
		//------> for each node need a Tlas but Tlas needs to be built from scene description i.e. LoadScene() info
		auto& info = SceneElementInstance(sceneElemIdx);
        const UINT sceneElementIdx = info.sceneElementIdx;
		const UINT numNodesInScene = NumNodesInScene(sceneElementIdx);
		for (UINT nodeIdx = 0; nodeIdx < numNodesInScene; nodeIdx++)
		{
			//------> for each prim need a GeomDesc, all Geom Desc go into one Blas
			const UINT numPrimsInNodeMesh = NumPrimitivesInNodeMesh(sceneElementIdx, nodeIdx);
			m_sceneBlas.sceneElementsBlas.emplace_back();
			DxASDesc& blasForNode = m_sceneBlas.sceneElementsBlas.back();
			std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> geomDescs(numPrimsInNodeMesh);
			for (UINT primIdx = 0; primIdx < numPrimsInNodeMesh; primIdx++)
			{
				auto& geomDesc = geomDescs[primIdx];
				const auto curPrim = GetPrimitiveInfo(sceneElementIdx, nodeIdx, primIdx);
				const D3D12_INDEX_BUFFER_VIEW indexBufferView = GetIndexBufferInfo(curPrim).modelIbv;

				//@note we should always have position
				const D3D12_VERTEX_BUFFER_VIEW vertexBufferView = GetVertexBufferInfo(curPrim, GltfVertexAttribPosition)->modelVbv;
				const DxDrawPrimitive         drawInfo = GetDrawInfo(sceneElementIdx, nodeIdx, primIdx);
				const BOOL isPrimTransparent = IsPrimitiveTransparent(sceneElementIdx, nodeIdx, primIdx);
				geomDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
				geomDesc.Triangles.IndexBuffer = drawInfo.isIndexedDraw ? indexBufferView.BufferLocation : 0;
				geomDesc.Triangles.IndexCount = drawInfo.isIndexedDraw ? drawInfo.numIndices : 0;
				geomDesc.Triangles.IndexFormat = drawInfo.isIndexedDraw ? indexBufferView.Format : DXGI_FORMAT_UNKNOWN;

				geomDesc.Triangles.VertexBuffer = { vertexBufferView.BufferLocation, vertexBufferView.StrideInBytes };
				geomDesc.Triangles.VertexCount = drawInfo.numVertices;
				geomDesc.Triangles.VertexFormat = GetVertexPositionBufferFormat(sceneElementIdx, nodeIdx, primIdx);

				//Object space -> new object space
				geomDesc.Triangles.Transform3x4 = 0;
				geomDesc.Flags = ((isPrimTransparent == FALSE) ? D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE : D3D12_RAYTRACING_GEOMETRY_FLAG_NONE);
			}
			D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS bottomLevelInputs = {};
			bottomLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
			bottomLevelInputs.NumDescs = geomDescs.size();
			bottomLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
			bottomLevelInputs.pGeometryDescs = geomDescs.data();
			bottomLevelInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
			// Get required sizes for an acceleration structure.
			D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO bottomLevelPrebuildInfo = {};
			m_dxrDevice->GetRaytracingAccelerationStructurePrebuildInfo(&bottomLevelInputs, &bottomLevelPrebuildInfo);
			dxhelper::AllocateBufferResource(m_dxrDevice.Get(), bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes, &blasForNode.resultBuffer, "BlasResult", D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, resultState);
			dxhelper::AllocateBufferResource(m_dxrDevice.Get(), bottomLevelPrebuildInfo.ScratchDataSizeInBytes, &blasForNode.scratchBuffer, "BlasScratch", D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, scratchState);

			D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC bottomLevelBuildDesc = {};
			bottomLevelBuildDesc.Inputs = bottomLevelInputs;
			bottomLevelBuildDesc.DestAccelerationStructureData = blasForNode.resultBuffer->GetGPUVirtualAddress();
			bottomLevelBuildDesc.ScratchAccelerationStructureData = blasForNode.scratchBuffer->GetGPUVirtualAddress();
			m_dxrCommandList->BuildRaytracingAccelerationStructure(&bottomLevelBuildDesc, 0, nullptr);

			ExecuteBuildAccelerationStructures();
		}
	}


	DxCamera* pCamera = GetCamera();
	const UINT numNodeTransforms = pCamera->NumModelTransforms();
	std::vector< D3D12_RAYTRACING_INSTANCE_DESC> instanceDescs(numNodeTransforms);
	const UINT numElementsInSceneLoad = NumElementsInSceneLoad();
	UINT transformsLinearIdx = 0;
	UINT sceneElementNodeLineadIdx = 0;
	for (UINT idx = 0; idx < numElementsInSceneLoad; idx++)
	{
		
		auto& info = SceneElementInstance(idx);
		const UINT sceneElementIdx = info.sceneElementIdx;
		const UINT numNodesInScene = NumNodesInScene(sceneElementIdx);
		//one node can have 2 BLAS e.g. oaktree
		for (UINT nodeIdx = 0; nodeIdx < numNodesInScene; nodeIdx++)
		{
			auto& blasDesc = m_sceneBlas.sceneElementsBlas[sceneElementNodeLineadIdx];
			const UINT numInstances = info.numInstances;
			for (UINT instanceIdx = 0; instanceIdx < numInstances; instanceIdx++)
			{
				D3D12_RAYTRACING_INSTANCE_DESC& instanceDesc = instanceDescs[transformsLinearIdx];
				const XMFLOAT4X4 pData = pCamera->GetDxrModelTransposeMatrix(transformsLinearIdx);
				memcpy(instanceDesc.Transform, &pData, sizeof(instanceDesc.Transform));
				instanceDesc.InstanceMask = 1;
				instanceDesc.InstanceContributionToHitGroupIndex = sceneElementNodeLineadIdx * 2;
				instanceDesc.AccelerationStructure = blasDesc.resultBuffer->GetGPUVirtualAddress();
				transformsLinearIdx++;
			}
			sceneElementNodeLineadIdx++;
		}
	}

	//@note Important we use a fence and block on CPU, so this will not get deallocated till TLAS build is complete.
	ComPtr<ID3D12Resource> instanceDesc = CreateBufferWithData(instanceDescs.data(),
		instanceDescs.size() * sizeof(D3D12_RAYTRACING_INSTANCE_DESC),
		"RayTracing_InstanceDesc",
		D3D12_RESOURCE_FLAG_NONE,
		D3D12_RESOURCE_STATE_COMMON,
		TRUE);


	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS topLevelInputs = {};
	topLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	topLevelInputs.NumDescs = instanceDescs.size();
	topLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	topLevelInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
	topLevelInputs.InstanceDescs = instanceDesc->GetGPUVirtualAddress();

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO topLevelPrebuildInfo = {};
	m_dxrDevice->GetRaytracingAccelerationStructurePrebuildInfo(&topLevelInputs, &topLevelPrebuildInfo);
	dxhelper::AllocateBufferResource(m_dxrDevice.Get(), topLevelPrebuildInfo.ResultDataMaxSizeInBytes, &m_sceneTlas.sceneTlas.resultBuffer, "TlasResult", D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, resultState);
	dxhelper::AllocateBufferResource(m_dxrDevice.Get(), topLevelPrebuildInfo.ScratchDataSizeInBytes, &m_sceneTlas.sceneTlas.scratchBuffer, "TlasScratch", D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, scratchState);

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC topLevelBuildDesc = {};
	topLevelBuildDesc.Inputs = topLevelInputs;
	topLevelBuildDesc.DestAccelerationStructureData = m_sceneTlas.sceneTlas.resultBuffer->GetGPUVirtualAddress();
	topLevelBuildDesc.ScratchAccelerationStructureData = m_sceneTlas.sceneTlas.scratchBuffer->GetGPUVirtualAddress();
	m_dxrCommandList->BuildRaytracingAccelerationStructure(&topLevelBuildDesc, 0, nullptr);

	//@note IMPORTANT, depending on CPU wait in this call, else instance descs buffer will get deallocated
	ExecuteBuildAccelerationStructures();
}
