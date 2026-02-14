/*
* Copyright (C) 2026 by Jayanth Gurijala
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/
#pragma once

#include "stdafx.h"

#define MaxPathSize 256

using namespace Microsoft::WRL;


class FileReader
{
public:
	FileReader();
	static void GetExecutablePath(_Out_writes_(pathSize) CHAR* path, UINT pathSize);

	std::string GetFullAssetFilePath(const std::string& assetName);
	ComPtr<ID3DBlob> LoadShaderBlobFromAssets(std::string compiledShaderName);

protected:

private:
	std::string m_assetsPath;

};


