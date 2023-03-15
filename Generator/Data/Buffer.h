#pragma once
#include <stdint.h>
#include <string>
#include <iostream>
#include <chrono>
#include <thread>
#include <fstream>
#include <string>
#include <memory>
#include <sstream>
#include <iomanip>
#include <vector>
#include <map>
#include <omp.h>
#include "types.hpp"
#include "Config/Config.hpp"

class HostBuffersClass
{
private:
	uint8_t* tempBuffer = NULL;
	size_t tempBufferSize = 0;
	point* result = NULL;
	size_t num_result = 0;

	uint64_t memory_size = 0;
public:
	std::ofstream outstream[256];

	HostBuffersClass()
	{
		memory_size = 0;
	}

	std::string FormatWithCommas(uint64_t value)
	{
		std::stringstream ss;
		ss.imbue(std::locale(""));
		ss << std::fixed << value;
		return ss.str();
	}
	int alignedMalloc(uint8_t** point, uint64_t size, uint64_t* all_ram_memory_size, std::string buff_name) {
		*point = (uint8_t*)_aligned_malloc(size, 4096);
		if (NULL == *point) { fprintf(stderr, "_aligned_malloc (%s) failed! Size: %s", buff_name.c_str(), FormatWithCommas(size).data()); return -1; }
		*all_ram_memory_size += size;
		//std::cout << "MALLOC RAM MEMORY SIZE (" << buff_name << "): " << std::to_string((float)size / (1024.0f * 1024.0f)) << " MB\n";
		return 0;
	}

	int MallocResult(size_t num_keys)
	{	
		if (alignedMalloc((uint8_t**)&result, num_keys * sizeof(point), &memory_size, "result") != 0) return -1;
		//std::cout << "MALLOC ALL RAM MEMORY SIZE (ALL): " << std::to_string((float)memory_size / (1024.0f * 1024.0f)) << " MB\n";
		num_result = num_keys;
		return 0;
	}

	int MallocTempBuffer(size_t size)
	{
		int ret = alignedMalloc(&tempBuffer, size, &memory_size, "tempBuffer");
		if (ret == 0) {
			tempBufferSize = size;
			return 0;
		}
		else
		{
			return ret;
		}
	}

	void FreeTempBuffer()
	{
		if (tempBuffer != NULL) _aligned_free(tempBuffer);
	}
	void FreeResultBuffer()
	{
		if (result != NULL) _aligned_free(result);
	}
	point* getResultBuffer()
	{
		return result;
	}
	size_t getSizeResultBuffer()
	{
		return num_result * sizeof(point);
	}
	size_t getNumResult()
	{
		return num_result;
	}
	uint8_t* getTempBuffer()
	{
		return tempBuffer;
	}
	size_t getSizeTempBuffer()
	{
		return tempBufferSize;
	}
	~HostBuffersClass()
	{
		FreeTempBuffer();
		FreeResultBuffer();
	}

};


class DataClass
{
public:
	HostBuffersClass host;
public:
	DataClass()
	{

	}
	DataClass(bool malloc)
	{
		if (malloc) Malloc();
	}
	int Malloc()
	{
		//if (host.Malloc() != 0) return -1;
		return 0;
	}
	~DataClass()
	{

	}
};
