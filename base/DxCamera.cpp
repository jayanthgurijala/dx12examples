#include "DxCamera.h"

DxCamera::DxCamera(UINT width, UINT height) :
    m_frameDeltaTime(0),
	m_cameraPosition({}),
	m_rotatedAngle(0)
{
    m_transformInfoList.clear();
    m_viewportWidth  = width;
    m_viewportHeight = height;
    m_viewportAspectRatio = (FLOAT)width / height;
	CreateViewMatrix();
	CreateProjectionMatrix();
}

VOID DxCamera::AddTransformInfo(DxMeshNodeTransformInfo transformInfo)
{
	XMMATRIX worldMatrix = CreateModelMatrix(transformInfo);
	XMMATRIX normalMatrix = XMMatrixTranspose(XMMatrixInverse(nullptr, worldMatrix));
    m_transformInfoList.push_back({transformInfo, worldMatrix, normalMatrix});
}

VOID DxCamera::Update(FLOAT frameDeltaTIme)
{
    m_frameDeltaTime = frameDeltaTIme;
	CreateViewMatrix();
}

/*
* Look at the origin and move away arbitrary 5 units away
* Let us use LH notation meaning Z increases away from camera in forward direction
*/
VOID DxCamera::CreateViewMatrix()
{
	static const FLOAT cameraSpeedInDegPerSecond = 10.0f;
    static XMVECTOR up        = XMVectorSet(0, 1, 0, 0);  // Y-up
    static XMVECTOR center    = XMVectorSet(0.0, 0., 0.0, 1.0);
    m_cameraPosition          = XMVectorSet(0.0, 0.0, -10, 1.0);



	///@todo use std::chrono properly
	m_rotatedAngle += cameraSpeedInDegPerSecond * m_frameDeltaTime;

	///@note rotate camera located at "cameraPosition" "cameraRotateAngleInDeg" around "sceneCenter".
	const XMVECTOR cameraVector = m_cameraPosition - center;
	const XMMATRIX rotationMatrix = XMMatrixRotationY(XMConvertToRadians(m_rotatedAngle));
	const XMVECTOR newCameraVector = XMVector3Transform(cameraVector, rotationMatrix);
	const XMVECTOR newCameraPos = newCameraVector + center;

	m_cameraPosition = newCameraPos;
    
    m_viewMatrix = XMMatrixLookAtLH(m_cameraPosition,
                                    center,
                                    up);
}

VOID DxCamera::CreateProjectionMatrix()
{
    m_projectionMatrix = XMMatrixPerspectiveFovLH(XMConvertToRadians(45.0f),
                                                  m_viewportAspectRatio,
                                                  0.01f,
                                                  100.0f);

}

XMMATRIX DxCamera::CreateModelMatrix(const DxMeshNodeTransformInfo& transformInfo)
{
	XMMATRIX modelMatrix;

	const std::vector<double>& meshTranslation = transformInfo.translation;
	const std::vector<double>& meshRotation = transformInfo.rotation;
	const std::vector<double>& meshScale = transformInfo.scale;

	if (transformInfo.hasMatrix == FALSE)
	{
		XMVECTOR translation = (transformInfo.hasTranslation == TRUE) ? XMVectorSet((FLOAT)meshTranslation[0],
			(FLOAT)meshTranslation[1],
			(FLOAT)meshTranslation[2], 1.0f) : XMVectorZero();

		//XMVECTOR rotation =  XMMatrixIdentity();

		XMVECTOR scale = (transformInfo.hasScale == TRUE) ? XMVectorSet((FLOAT)meshScale[0],
			(FLOAT)meshScale[1],
			(FLOAT)meshScale[2],
			1.0f) : XMVectorSplatOne();

		XMMATRIX T = XMMatrixTranslationFromVector(translation);

		///@todo find good way for this rotate by 180 degree gltf to dx RH vs LH conversion
		XMMATRIX R = XMMatrixRotationY(XM_PI);
		XMMATRIX S = XMMatrixScalingFromVector(scale);

		modelMatrix = S * R * T;
	}
	else
	{
		const std::vector<double>& m4x4 = transformInfo.matrix;


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
	return modelMatrix;
}

XMFLOAT4X4 GetDataFromMatrix(XMMATRIX matrix)
{
	XMFLOAT4X4 matrixData;
	XMStoreFloat4x4(&matrixData, matrix);
	return matrixData;
}

XMFLOAT4X4 DxCamera::GetViewProjectionMatrixTranspose()
{
	return GetDataFromMatrix(XMMatrixTranspose(m_viewMatrix * m_projectionMatrix));
}


XMFLOAT4X4 DxCamera::GetWorldMatrixTranspose(UINT index)
{
	return GetDataFromMatrix(XMMatrixTranspose(GetModelMatrix(index)));
}

XMFLOAT4X4 DxCamera::GetModelViewProjectionMatrixTranspose(UINT index)
{
	return GetDataFromMatrix(XMMatrixTranspose(GetModelMatrix(index) * m_viewMatrix * m_projectionMatrix));
}

XMFLOAT4X4 DxCamera::GetNormalMatrixData(UINT index)
{
	return GetDataFromMatrix(GetNormalMatrix(index));
}

XMFLOAT4 DxCamera::GetCameraPosition()
{
	XMFLOAT4 camPosData;
	XMStoreFloat4(&camPosData, m_cameraPosition);
	return camPosData;
}



