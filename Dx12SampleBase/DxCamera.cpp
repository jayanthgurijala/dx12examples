#include "pch.h"
#include "DxCamera.h"
#include "DxPrintUtils.h"

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

XMFLOAT4X4 DxCamera::GetDxrModelTransposeMatrix(UINT index)
{
	XMFLOAT4X4 xmFloat4x4;
	XMMATRIX mmModelMatrix    = XMMatrixTranspose(m_transformInfoList[index].modelMatrix);
	XMStoreFloat4x4(&xmFloat4x4, mmModelMatrix);

	return xmFloat4x4;
}

VOID DxCamera::AddTransformInfo(DxNodeTransformInfo transformInfo)
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
	static XMVECTOR    lookAt                    = XMVectorSet(0.0, 0.0, 0.0, 1.0);
	
    XMVECTOR up            = XMVectorSet(0, 1, 0, 1.0f);  
    m_cameraPosition       = XMVectorSet(0.0f, 0.0f, -5.0f, 1.0f);


	///@todo use std::chrono properly
	m_rotatedAngle += cameraSpeedInDegPerSecond * m_frameDeltaTime;

	///@note rotate camera located at "cameraPosition" "cameraRotateAngleInDeg" around "sceneCenter".
	const XMVECTOR cameraVector = m_cameraPosition - lookAt;
	const XMMATRIX rotationMatrix = XMMatrixRotationY(XMConvertToRadians(m_rotatedAngle));
	const XMVECTOR newCameraVector = XMVector3Transform(cameraVector, rotationMatrix);
	const XMVECTOR newCameraPos = newCameraVector + lookAt;
	
	m_cameraPosition = newCameraPos;

	//m_eye = (-3.5, 2.0, -3.5, 1.0)
	//m_at = (0.0, 0.0, 0.0, 1.0)
	//up = (0.26, 0.93, 0.26, 1.0)

	//ratio = 1.77, 1, 125
    
	//XMVECTOR eyePosition = XMVectorSet(0, -0.2, -5, 1.0);
	//XMVECTOR lookAt      = XMVectorSet(0.0, 0.0, 0.0, 1.0);
	//XMVECTOR up          = XMVectorSet(0, 1, 0, 1.0);

    m_viewMatrix = XMMatrixLookAtLH(m_cameraPosition,
		                            lookAt,
                                    up);
}

VOID DxCamera::CreateProjectionMatrix()
{
    m_projectionMatrix = XMMatrixPerspectiveFovLH(XMConvertToRadians(45.0f),
                                                  m_viewportAspectRatio,
                                                  1.0f,
                                                  125.0f);

}

XMMATRIX DxCamera::CreateModelMatrix(const DxNodeTransformInfo& transformInfo)
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

XMFLOAT4X4 DxCamera::GetViewProjectionInverse()
{
	XMMATRIX vpMatrix = m_viewMatrix * m_projectionMatrix;
	//PrintUtils::PrintXMMatrix("ViewProj", vpMatrix);
	XMMATRIX vpInverse = XMMatrixInverse(nullptr, vpMatrix);
	//PrintUtils::PrintXMMatrix("ViewProjInverse", vpInverse);
	XMMATRIX chackMatrix = vpMatrix * vpInverse;
	//PrintUtils::PrintXMMatrix("Check", chackMatrix);
	return GetDataFromMatrix(XMMatrixTranspose(vpInverse));
}



