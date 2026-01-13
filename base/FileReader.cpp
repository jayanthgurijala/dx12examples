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


#include <Windows.h>

// UTF-8 -> UTF-16
std::wstring FileReader::ToWideString(const std::string& s) {
	int size = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
	std::wstring ws(size, 0);
	MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &ws[0], size);
	ws.pop_back(); // remove null terminator
	return ws;
}

// UTF-16 -> UTF-8
std::string FileReader::ToNarrowString(const std::wstring& ws) {
	int size = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, nullptr, 0, nullptr, nullptr);
	std::string s(size, 0);
	WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, &s[0], size, nullptr, nullptr);
	s.pop_back();
	return s;
}

