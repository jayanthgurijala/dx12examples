#pragma once

#include "stdafx.h"

#define MaxPathSize 256

using namespace Microsoft::WRL;


class FileReader
{
public:
	FileReader();
	static void GetExecutablePath(_Out_writes_(pathSize) WCHAR* path, UINT pathSize);
	static std::string ToNarrowString(const std::wstring& wideString);
	static std::wstring ToWideString(const std::string& narrowString);

	std::wstring GetFullAssetFilePath(LPCWSTR assetName);
	ComPtr<ID3DBlob> LoadShaderBlobFromAssets(std::wstring compiledShaderName);

protected:

private:
	std::wstring m_assetsPath;

};

