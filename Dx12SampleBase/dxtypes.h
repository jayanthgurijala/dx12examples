#pragma once

#include <d3d12.h>
#include <vector>
#include <string>


struct SimpleVertex
{
	FLOAT position[3];
	FLOAT color[3];
	FLOAT texCoords[2];
};

struct DxExtents
{
	double min[3];
	double max[3];
	BOOL hasValidExtents;
};

struct DxMeshNodeTransformInfo
{
	BOOL hasTranslation;
	BOOL hasRotation;
	BOOL hasScale;
	BOOL hasMatrix;

	std::vector<double> translation;
	std::vector<double> rotation;
	std::vector<double> scale;
	std::vector<double> matrix;
};

struct DxIASemantic
{
	std::string name;
	UINT index;
	BOOL isIndexValid;
	DXGI_FORMAT format;
};

struct DxDrawPrimitive
{
	UINT numVertices;
	UINT numIndices;
	BOOL isIndexedDraw;
};

struct DxMeshState
{
	//IALayout needs contiguous data
	std::vector<D3D12_INPUT_ELEMENT_DESC> inputElementDesc;
	BOOL doubleSided;
};

enum DxAppFrameType
{
	DxFrameRTVIndex = 0,
	DxFrameResource = 1,
	DxFrameInvalid  = 2
};

struct DxAppFrameInfo
{
	UINT rtvIndex;
	ID3D12Resource* pFrameResource;
	D3D12_RESOURCE_STATES pResState;
	DxAppFrameType type;
};