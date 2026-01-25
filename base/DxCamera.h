#pragma once

#include "dxtypes.h"
#include "stdafx.h"
#include <vector>
#include <d3d12.h>
#include "FileReader.h"
#include <DirectXMath.h>
#include <d3dx12.h>

using namespace DirectX;

struct DxModelMatrix
{
	DxMeshNodeTransformInfo transformInfo;
	XMMATRIX modelMatrix;
};

class DxCamera
{
public:
	VOID AddTransformInfo(DxMeshNodeTransformInfo transformInfo);
	XMFLOAT4X4 GetViewProjectionMatrixTranspose();
	XMFLOAT4X4 GetWorldMatrixTranspose(UINT index);
	XMFLOAT4X4 GetModelViewProjectionMatrixTranspose(UINT index);
	XMVECTOR   GetCameraPosition();
	VOID Update(FLOAT frameDeltaTime);
	DxCamera(UINT width, UINT height);

	//@todo remove these once tested
	XMMATRIX GetModelMatrix_Temp(UINT index);
	XMMATRIX GetViewMatrix_Temp();
	XMMATRIX GetProjectionMatrix_Temp();

private:
	VOID CreateViewMatrix();
	VOID CreateProjectionMatrix();
	XMMATRIX CreateModelMatrix(const DxMeshNodeTransformInfo& transformInfo);
	XMMATRIX GetModelMatrix(UINT index);

	std::vector<DxModelMatrix> m_transformInfoList;
	FLOAT m_frameDeltaTime;
	UINT m_viewportWidth;
	UINT m_viewportHeight;
	FLOAT m_viewportAspectRatio;
	FLOAT m_rotatedAngle;

	XMMATRIX m_viewMatrix;
	XMMATRIX m_projectionMatrix;
	XMVECTOR m_cameraPosition;
};

