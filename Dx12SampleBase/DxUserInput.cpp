#include "pch.h"
#include "DxUserInput.h"
#include "DxPrintUtils.h"
#include "Dx12SampleBase.h"

DxUserInput::DxUserInput(DxCamera* camera)
{
	m_isMouseLeftButtonDown = FALSE;
	m_prevMouseX = 0;
	m_prevMouseY = 0;
	m_camera = camera;

	m_rotatedAngleX = 0;
	m_rotatedAngleY = 0;

	m_cameraPosition = XMVectorSet(0.0f, 0.0f, -5.0f, 1.0f);
	m_lookAt         = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
	m_up             = XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f);

}

DxUserInput::~DxUserInput()
{
}

VOID DxUserInput::OnMouseDown(UINT x, UINT y, BOOL isLeftButton)
{
	m_isMouseLeftButtonDown = TRUE;
	x = static_cast<FLOAT>(x);
	y = static_cast<FLOAT>(y);
	m_prevMouseX = x;
	m_prevMouseY = y;
}

VOID DxUserInput::OnMouseUp(UINT x, UINT y, BOOL isLeftButton)
{
	m_isMouseLeftButtonDown = FALSE;
}

VOID DxUserInput::OnMouseMove(UINT x, UINT y)
{
	static float speed = 30.0f;

	x = static_cast<FLOAT>(x);
	y = static_cast<FLOAT>(y);

	if (m_isMouseLeftButtonDown == TRUE)
	{
		FLOAT dx = x - m_prevMouseX;
		FLOAT dy = y - m_prevMouseY;
		FLOAT frameDeltaTime = Dx12SampleBase::s_frameDeltaTime;

		FLOAT dxInDegrees = dx * speed * frameDeltaTime;
		FLOAT dyInDegrees = dy * speed * frameDeltaTime;


		//@note X mouse movement corresponds to rotation around Y axis and Y mouse movement corresponds to rotation around X axis
		m_rotatedAngleY += dxInDegrees; //preserves up direction
		m_rotatedAngleX += dyInDegrees; //messe up up direction and cause forward to be aligned with up

		FLOAT xRotInRadians = XMConvertToRadians(m_rotatedAngleX);
		FLOAT yRotInRadians = XMConvertToRadians(m_rotatedAngleY);

		///@note rotate camera located at "cameraPosition" "cameraRotateAngleInDeg" around "sceneCenter".
		const XMVECTOR cameraVector = m_cameraPosition - m_lookAt;
		const XMMATRIX rotationMatrixX = XMMatrixRotationX(xRotInRadians);
		const XMMATRIX rotationMatrixY = XMMatrixRotationY(yRotInRadians);
		XMMATRIX rotation = rotationMatrixX * rotationMatrixY;
		const XMVECTOR newCameraVector = XMVector3Transform(cameraVector, rotation);
		const XMVECTOR newCameraPos = newCameraVector + m_lookAt;

		XMVECTOR worldUp = XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f);
		XMVECTOR forward = XMVector3Normalize(m_lookAt - newCameraPos);
		XMVECTOR right = XMVector3Normalize(XMVector3Cross(worldUp, forward));
		m_up = XMVector3Normalize(XMVector3Cross(forward, right));

		XMFLOAT3 temp;
		XMStoreFloat3(&temp, m_up);
		m_camera->UpdateCameraViewMatrix(newCameraPos, m_lookAt, m_up);

		m_prevMouseX = x;
		m_prevMouseY = y;
	}
}
