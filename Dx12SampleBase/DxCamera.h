/*
* Copyright (C) 2026 by Jayanth Gurijala
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/
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
	DxNodeTransformInfo transformInfo;
	XMMATRIX modelMatrix;
	XMMATRIX normalMatrix;
};

class DxCamera
{
public:
	VOID AddTransformInfo(DxNodeTransformInfo transformInfo);
	XMFLOAT4X4 GetViewProjectionMatrixTranspose();
	XMFLOAT4X4 GetWorldMatrixTranspose(UINT index);
	XMFLOAT4X4 GetModelViewProjectionMatrixTranspose(UINT index);
	XMFLOAT4X4 GetNormalMatrixData(UINT index);
	XMFLOAT4   GetCameraPosition();
	XMFLOAT4X4 GetViewProjectionInverse();
	VOID Update(FLOAT frameDeltaTime);
	DxCamera(UINT width, UINT height);
	XMFLOAT4X4 GetDxrModelTransposeMatrix(UINT index);

	VOID UpdateCameraViewMatrix(XMVECTOR cameraPosition, XMVECTOR lookAt, XMVECTOR up);

private:
	VOID CreateViewMatrix();
	VOID CreateProjectionMatrix();
	XMMATRIX CreateModelMatrix(const DxNodeTransformInfo& transformInfo);

	inline XMMATRIX GetModelMatrix(UINT index) { return m_transformInfoList[index].modelMatrix; }
	inline XMMATRIX GetNormalMatrix(UINT index) { return m_transformInfoList[index].normalMatrix; }

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


