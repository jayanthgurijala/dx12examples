#include "pch.h"
#include "FileReader.h"
#include <stdexcept>
#include <fstream>
#include <vector>


void FileReader::GetExecutablePath(_Out_writes_(pathSize) CHAR* path, UINT pathSize)
{
	if (path == nullptr)
	{
		throw std::exception();
	}

	DWORD size = GetModuleFileNameA(nullptr, path, pathSize);

	if (size == 0 || size == pathSize)
	{
		throw std::exception();
	}

	//strip the file name
	CHAR* lastSlash = strrchr(path, L'\\');
	if (lastSlash)
	{
		*(lastSlash + 1) = L'\0';
	}

}

FileReader::FileReader()
{
	CHAR executableFilePath[MaxPathSize];
	FileReader::GetExecutablePath(executableFilePath, MaxPathSize);
	m_assetsPath = executableFilePath;
}

std::string FileReader::GetFullAssetFilePath(const std::string& assetName)
{
	return m_assetsPath + assetName;
}

/*
* Loads compiled shader e.g. vso, pso, cso etc.
* To compile and load shader use D3DCompileFromFile
*/
ComPtr<ID3DBlob> FileReader::LoadShaderBlobFromAssets(std::string compiledShaderName)
{
	ComPtr<ID3DBlob> shaderBlob = nullptr;

	std::ifstream shaderFile(compiledShaderName, std::ios::binary | std::ios::ate);
	if (shaderFile.is_open())
	{
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
	}
	return shaderBlob;

}


