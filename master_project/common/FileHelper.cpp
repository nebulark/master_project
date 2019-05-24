#include "pch.h"
#include "FileHelper.hpp"
#include <fstream>



std::vector<char> FileHelper::LoadFileContent(const char* filename)
{
	std::ifstream file;
	file.open(filename, std::ios::ate | std::ios::binary);
	if (!file.is_open())
	{
		assert(false);
		return {};
	}

	const size_t fileSize = file.tellg();
	std::vector<char> buffer;
	buffer.resize(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);
	file.close();
	
	return buffer;
}
