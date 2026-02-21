#pragma once

#include "DxCamera.h"

class DxUserInput
{
public:
	DxUserInput(DxCamera *camera);
	~DxUserInput();
	VOID OnMouseDown(UINT x, UINT y, BOOL isLeftButton);
	VOID OnMouseUp(UINT x, UINT y, BOOL isLeftButton);
	VOID OnMouseMove(UINT x, UINT y);
protected:



private:
	BOOL m_isMouseLeftButtonDown;
	FLOAT m_prevMouseX;
	FLOAT m_prevMouseY;
	DxCamera* m_camera;

	FLOAT    m_rotatedAngleX;
	FLOAT    m_rotatedAngleY;
	XMVECTOR m_cameraPosition;
	XMVECTOR m_lookAt;
	XMVECTOR m_up;
};

