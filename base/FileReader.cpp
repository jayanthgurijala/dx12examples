#include "FileReader.h"
#include <stdexcept>
#include <fstream>
#include <vector>



void FileReader::GetExecutablePath(_Out_writes_(pathSize) WCHAR* path, UINT pathSize)
{
	if (path == nullptr)
	{
		throw std::exception();
	}

	DWORD size = GetModuleFileName(nullptr, path, pathSize);

	if (size == 0 || size == pathSize)
	{
		throw std::exception();
	}

	//strip the file name
	WCHAR* lastSlash = wcsrchr(path, L'\\');
	if (lastSlash)
	{
		*(lastSlash + 1) = L'\0';
	}

}

FileReader::FileReader()
{
	WCHAR executableFilePath[MaxPathSize];
	FileReader::GetExecutablePath(executableFilePath, MaxPathSize);
	m_assetsPath = executableFilePath;
}

std::wstring FileReader::GetFullAssetFilePath(LPCWSTR assetName)
{
	return m_assetsPath + assetName;
}

/*
* Loads compiled shader e.g. vso, pso, cso etc.
* To compile and load shader use D3DCompileFromFile
*/
ComPtr<ID3DBlob> FileReader::LoadShaderBlobFromAssets(std::wstring compiledShaderName)
{
	ComPtr<ID3DBlob> shaderBlob;
	std::ifstream shaderFile(compiledShaderName, std::ios::binary | std::ios::ate);
	std::streamsize totalSize = shaderFile.tellg();
	shaderFile.seekg(0, std::ios::beg);

	std::vector<char> buffer(totalSize);
	if (!shaderFile.read(buffer.data(), totalSize))
	{
		throw std::runtime_error("Unable to read shader file");
	}

	HRESULT hr = D3DCreateBlob(totalSize, &shaderBlob);
	if (hr != S_OK)
	{
		throw std::runtime_error("D3DCreateBlob failed");
	}

	memcpy(shaderBlob->GetBufferPointer(), buffer.data(), totalSize);
	return shaderBlob;

}
