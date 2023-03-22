/*****************************************************************************
 * Copyright (c) 2013-2016 Intel Corporation
 * All rights reserved.
 *
 * WARRANTY DISCLAIMER
 *
 * THESE MATERIALS ARE PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL INTEL OR ITS
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THESE
 * MATERIALS, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Intel Corporation is the author of the Materials, and requests that all
 * problem reports or change requests be submitted to it directly
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <memory.h>
#include <windows.h>
#include "utils.hpp"

 // Suppress a compiler warning about undefined CL_TARGET_OPENCL_VERSION
 // Khronos ICD supports only latest OpenCL version
#define CL_TARGET_OPENCL_VERSION 220

#include "CL\cl.h"
#include "CL\cl_ext.h"
#include "Config/Config.hpp"




//we want to use POSIX functions
#pragma warning( push )
#pragma warning( disable : 4996 )


void LogInfo(const char* str, ...)
{
	if (str)
	{
		va_list args;
		va_start(args, str);

		vfprintf(stdout, str, args);

		va_end(args);
	}
}

void LogError(const char* str, ...)
{
	if (str)
	{
		va_list args;
		va_start(args, str);

		vfprintf(stderr, str, args);

		va_end(args);
	}
}

// Upload the OpenCL C source code to output argument source
// The memory resource is implicitly allocated in the function
// and should be deallocated by the caller
int ReadSourceFromFile(const char* fileName, char** source, size_t* sourceSize)
{
	int errorCode = CL_SUCCESS;

	FILE* fp = NULL;
	fopen_s(&fp, fileName, "rb");
	if (fp == NULL)
	{
		LogError("Error: Couldn't find program source file '%s'.\n", fileName);
		errorCode = CL_INVALID_VALUE;
	}
	else {
		fseek(fp, 0, SEEK_END);
		*sourceSize = ftell(fp);
		fseek(fp, 0, SEEK_SET);

		*source = new char[*sourceSize];
		if (*source == NULL)
		{
			LogError("Error: Couldn't allocate %d bytes for program source from file '%s'.\n", *sourceSize, fileName);
			errorCode = CL_OUT_OF_HOST_MEMORY;
		}
		else {
			fread(*source, 1, *sourceSize, fp);
		}
	}
	return errorCode;
}

#pragma warning( pop )

std::string readFile(const char* const szFilename)
{
	std::ifstream in(szFilename, std::ios::in | std::ios::binary);
	std::ostringstream contents;
	contents << in.rdbuf();
	return contents.str();
}

std::vector<cl_device_id> getAllDevices(cl_device_type deviceType)
{
	std::vector<cl_device_id> vDevices;

	cl_uint platformIdCount = 0;
	clGetPlatformIDs(0, NULL, &platformIdCount);

	std::vector<cl_platform_id> platformIds(platformIdCount);
	clGetPlatformIDs(platformIdCount, platformIds.data(), NULL);

	for (auto it = platformIds.cbegin(); it != platformIds.cend(); ++it) {
		cl_uint countDevice;
		clGetDeviceIDs(*it, deviceType, 0, NULL, &countDevice);

		std::vector<cl_device_id> deviceIds(countDevice);
		clGetDeviceIDs(*it, deviceType, countDevice, deviceIds.data(), &countDevice);

		std::copy(deviceIds.begin(), deviceIds.end(), std::back_inserter(vDevices));
	}

	return vDevices;
}





template <typename T, typename U, typename V, typename W>
std::vector<T> clGetWrapperVector(U function, V param, W param2) {
	size_t len;
	function(param, param2, 0, NULL, &len);
	len /= sizeof(T);
	std::vector<T> v;
	if (len > 0) {
		T* pArray = new T[len];
		function(param, param2, len * sizeof(T), pArray, NULL);
		for (size_t i = 0; i < len; ++i) {
			v.push_back(pArray[i]);
		}
		delete[] pArray;
	}
	return v;
}

std::vector<std::string> getBinaries(cl_program& clProgram) {
	std::vector<std::string> vReturn;
	std::vector<size_t> vSizes = clGetWrapperVector<size_t>(clGetProgramInfo, clProgram, CL_PROGRAM_BINARY_SIZES);
	if (!vSizes.empty()) {
		unsigned char** pBuffers = new unsigned char* [vSizes.size()];
		for (size_t i = 0; i < vSizes.size(); ++i) {
			pBuffers[i] = new unsigned char[vSizes[i]];
		}

		clGetProgramInfo(clProgram, CL_PROGRAM_BINARIES, vSizes.size() * sizeof(unsigned char*), pBuffers, NULL);
		for (size_t i = 0; i < vSizes.size(); ++i) {
			std::string strData(reinterpret_cast<char*>(pBuffers[i]), vSizes[i]);
			vReturn.push_back(strData);
			delete[] pBuffers[i];
		}

		delete[] pBuffers;
	}

	return vReturn;
}


bool printResult(const cl_int err) {
	std::cout << ((err != CL_SUCCESS) ? TranslateOpenCLError(err) : "OK") << std::endl;
	return err != CL_SUCCESS;
}

bool printProgramBuildInfo(const cl_int err, cl_program program, cl_device_id device) {
	if (err == CL_BUILD_PROGRAM_FAILURE)
	{
		size_t log_size = 0;
		clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);

		std::vector<char> build_log(log_size);
		clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size, &build_log[0], NULL);

		LogError("Error happened during the build of OpenCL program.\nBuild log:%s", &build_log[0]);
	}
	else
	{
		std::cout << ((err != CL_SUCCESS) ? TranslateOpenCLError(err) : "OK") << std::endl;
	}
	return err != CL_SUCCESS;
}


unsigned int getKernelExecutionTimeMicros(cl_event& e) {
	cl_ulong timeStart = 0, timeEnd = 0;
	clWaitForEvents(1, &e);
	clGetEventProfilingInfo(e, CL_PROFILING_COMMAND_START, sizeof(timeStart), &timeStart, NULL);
	clGetEventProfilingInfo(e, CL_PROFILING_COMMAND_END, sizeof(timeEnd), &timeEnd, NULL);
	return (unsigned int)(timeEnd - timeStart) / 1000;
}


bool incUlong4(cl_ulong4* s) {
	if (++s->s[0] != 0) return false;
	if (++s->s[1] != 0) return false;
	if (++s->s[2] != 0) return false;
	if (++s->s[3] != 0) return false;
	return true; //overflow
}

bool incUlong4Profanity(cl_ulong4* s) {
	if (++s->s[3] != 0) return false;
	return true; //overflow
}

bool isUlong4Zero(cl_ulong4 s) {
	if ((s.s[0] == (0xAAFFFF + 1)) && (s.s[1] == 0) && (s.s[2] == 0) && (s.s[3] == 0)) return true;
	return true;
}

void printPublicKey(char* title, public_key* pubKey) {
	printf("%s%.16llX%.16llX%.16llX%.16llX%.16llX%.16llX%.16llX%.16llX\n",
		title,
		pubKey->key.s[7],
		pubKey->key.s[6],
		pubKey->key.s[5],
		pubKey->key.s[4],
		pubKey->key.s[3],
		pubKey->key.s[2],
		pubKey->key.s[1],
		pubKey->key.s[0]);
}

void printPrivateKey(char* title, private_key* privKey) {
	printf("%s%.16llx%.16llx%.16llx%.16llx\n",
		title,
		privKey->key.s[3],
		privKey->key.s[2],
		privKey->key.s[1],
		privKey->key.s[0]);
}

void printPoint(char* title, point* point) {
	printf("%s%.8x%.8x%.8x%.8x%.8x%.8x%.8x%.8x%.8x%.8x%.8x%.8x%.8x%.8x%.8x%.8x\n",
		title,
		point->x.d[7],
		point->x.d[6],
		point->x.d[5],
		point->x.d[4],
		point->x.d[3],
		point->x.d[2],
		point->x.d[1],
		point->x.d[0],
		point->y.d[7],
		point->y.d[6],
		point->y.d[5],
		point->y.d[4],
		point->y.d[3],
		point->y.d[2],
		point->y.d[1],
		point->y.d[0]);
}

void printAddress(char* title, mp_number* num) {
	printf("%s%.8x%.8x%.8x%.8x%.8x\n",
		title,
		num->d[4],
		num->d[3],
		num->d[2],
		num->d[1],
		num->d[0]);
}

void printMpNumber(char* title, mp_number* num) {
	printf("%s%.8x%.8x%.8x%.8x%.8x%.8x%.8x%.8x\n",
		title,
		num->d[0],
		num->d[1],
		num->d[2],
		num->d[3],
		num->d[4],
		num->d[5],
		num->d[6],
		num->d[7]);
}

void printPercent(std::string title, double percent) {
	std::ostringstream ss;
	ss << std::fixed << std::setprecision(3) << percent << "%";
	const auto savePercent = ss.str();

	const std::string strVT100ClearLine = "\33[2K\r";
	// << '\r' << std::flush;
	std::cout << strVT100ClearLine << title << savePercent << std::flush;
}


int clearFile(const char* filename) {
	std::ofstream out;
	out.exceptions(std::ofstream::failbit | std::ofstream::badbit);

	try {
		out.open(filename);
		out.close();
	}
	catch (std::ofstream::failure e) {
		std::cerr << "Error clear file " << filename << " : " << e.what() << '\n';
		throw;
	}
	return 0;
}

void clearFiles(std::string folder) {
	std::ostringstream ss;
	ss << std::hex << std::uppercase;
	for (int i = 0; i < 256; i++) {
		ss.str("");
		ss << folder << '/' << std::setfill('0') << std::setw(2) << i << ".bin";
		clearFile(ss.str().c_str());
	}
}


void openBinaryFilesForWrite(std::ofstream* out, std::string path) {
	std::string filename;
	for (int i = 0; i < 256; i++)
	{
		filename = getFileName(path, i, "bin");
		out[i].exceptions(std::ofstream::failbit | std::ofstream::badbit);
		try {
			out[i].open(filename, std::ios_base::app | std::ios::binary);
		}
		catch (std::ofstream::failure e) {
			std::cerr << "Error write to file " << filename << " : " << e.what() << '\n';
			throw;
		}
	}
}

void openBinaryFilesForRead(std::ifstream* in, std::string path) {
	std::string filename;
	for (int i = 0; i < 256; i++)
	{
		filename = getFileName(path, i, "bin");
		in[i].exceptions(std::ifstream::badbit);
		try {
			in[i].open(filename, std::ios::in | std::ios::binary);
		}
		catch (std::ofstream::failure e) {
			std::cerr << "Error read from file " << filename << " : " << e.what() << '\n';
			throw;
		}
	}
}

void closeBinaryFiles(std::ofstream out[]) {
	for (int i = 0; i < 256; i++)
	{
		try {
			out[i].close();
		}
		catch (std::ofstream::failure e) {
			std::string filename = getFileName("", i, "bin");
			std::cerr << "Error close file " << filename << " : " << e.what() << '\n';
			throw;
		}
	}
}

void writeToStream(std::ofstream& out, void* data, size_t size) {
	try {
		out.write((char*)data, size);
	}
	catch (std::ofstream::failure e) {
		std::cerr << "Error write to stream: " << e.what() << '\n';
		throw;
	}
}



void openStreamToWriteTXT(std::string filename, std::ofstream& out) {
	try {
		out.open(filename, std::ios_base::app);
	}
	catch (std::ofstream::failure e) {
		std::cerr << "Error openStreamToWriteTXT " << filename << " : " << e.what() << '\n';
		throw;
	}
}

void closeStream(std::ofstream& out) {
	try {
		out.close();;
	}
	catch (std::ofstream::failure e) {
		std::cerr << "Error close stream: " << e.what() << '\n';
		throw;
	}
}

void closeInStreams(std::ifstream in[]) {
	for (int i = 0; i < 256; i++)
	{
		try {
			in[i].close();
		}
		catch (std::ofstream::failure e) {
			std::string filename = getFileName("", i, "bin");
			std::cerr << "Error close file " << filename << " : " << e.what() << '\n';
			throw;
		}
	}
}

void putToBinaryFile(std::string filename, void* data, size_t size) {
	std::ofstream out;
	out.exceptions(std::ofstream::failbit | std::ofstream::badbit);
	try {
		out.open(filename, std::ios_base::app | std::ios::binary);
		out.write((char*)data, size);
		out.close();
	}
	catch (std::ofstream::failure e) {
		std::cerr << "Error write to file " << filename << " : " << e.what() << '\n';
		throw;
	}
}

void getFromBinaryFile(std::string filename, void* data, size_t size, size_t position) {
	std::ifstream in;
	in.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	try {
		in.open(filename, std::ios::in | std::ios::binary);
		in.read((char*)data, size);
		in.close();
	}
	catch (std::ifstream::failure e) {
		std::cerr << "Error read file " << filename << " : " << e.what() << '\n';
		throw;
	}
}

std::string getFileName(std::string path, size_t num_file, std::string extension) {
	std::ostringstream ss;
	ss << std::hex << std::uppercase;
	ss << path << '\\' << std::setfill('0') << std::setw(2) << num_file << "." << extension;
	return ss.str();
}



void readBinaryFile(std::string filename, void* data, size_t size) {
	std::ifstream in;
	in.exceptions(std::ifstream::badbit);
	try {
		in.open(filename, std::ios::in | std::ios::binary);
		in.read((char*)data, size);
		in.close();
	}
	catch (std::ifstream::failure e) {
		std::cerr << "Error read file " << filename << " : " << e.what() << '\n';
		throw;
	}
}

size_t getBinaryFileSize(std::string filename) {
	std::ifstream in(filename, std::ios::binary);

	const auto begin = in.tellg();
	in.seekg(0, std::ios::end);
	const auto end = in.tellg();
	const auto fsize = (end - begin);
	return fsize;
}







void printBinaryResult(uint64_t* data, size_t len) {
	for (int i = 0; i < len; i++) {
		public_key* key = (public_key*)&data[8 * i];
		printPublicKey("public_key: ", key);
	}
}


void stringToPublicKey(std::string str, public_key* pubKey) {
	for (int i = 0; i < 8; i++) {
		std::string substr = str.substr(((size_t)i * 16), 16);
		pubKey->key.s[7 - i] = (mp_word)std::stoull(substr, nullptr, 16);
	}
}

void stringToPrivateKey(std::string str, private_key* privKey) {
	for (int i = 0; i < 4; i++) {
		std::string substr = str.substr(((size_t)i * 16), 16);
		privKey->key.s[3 - i] = (cl_ulong)std::stoull(substr, nullptr, 16);
	}
}

void copyPubKey(public_key* pubKey1, const public_key* pubKey2) {
	for (int i = 0; i < 8; i++) {
		pubKey1->key.s[i] = pubKey2->key.s[i];
	}
}

bool isPubKeysEqual(const public_key* pubKey1, const public_key* pubKey2) {
	for (int i = 0; i < 8; i++) {
		if (pubKey1->key.s[i] != pubKey2->key.s[i]) return false;
	}
	return true;
}

private_key calcKeyFromResult(private_key privKey, result_crack r) {

	// Format private key
	cl_ulong carry = 0;
	private_key found;

	found.key.s[0] = privKey.key.s[0] + r.round; carry = found.key.s[0] < r.round;
	found.key.s[1] = privKey.key.s[1] + carry; carry = !found.key.s[1];
	found.key.s[2] = privKey.key.s[2] + carry; carry = !found.key.s[2];
	found.key.s[3] = privKey.key.s[3] + carry + r.foundId;

	return found;
}

int getNumFileFromNum(size_t num, size_t all_num, size_t num_files) {

	if (num == 0) { std::cerr << "ERROR getNumFileFromNum(): num == 0 " << std::endl; return -1; }
	if (num > all_num) { std::cerr << "ERROR getNumFileFromNum(): num > all_num. num = " << num << ", all_num = " << all_num << std::endl; return -1; }
	if (all_num < num_files) { std::cerr << "ERROR getNumFileFromNum(): all_num < num_files. all_num = " << all_num << ", num_files = " << num_files << std::endl; return -1; }

	int num_in_file = (size_t)(all_num / num_files);
	int num_file = ((size_t)num - 1) / num_in_file;

	return num_file;
}


std::string TranslateOpenCLError(cl_int errorCode)
{
	switch (errorCode)
	{
	case CL_SUCCESS:                            return "CL_SUCCESS";
	case CL_DEVICE_NOT_FOUND:                   return "CL_DEVICE_NOT_FOUND";
	case CL_DEVICE_NOT_AVAILABLE:               return "CL_DEVICE_NOT_AVAILABLE";
	case CL_COMPILER_NOT_AVAILABLE:             return "CL_COMPILER_NOT_AVAILABLE";
	case CL_MEM_OBJECT_ALLOCATION_FAILURE:      return "CL_MEM_OBJECT_ALLOCATION_FAILURE";
	case CL_OUT_OF_RESOURCES:                   return "CL_OUT_OF_RESOURCES";
	case CL_OUT_OF_HOST_MEMORY:                 return "CL_OUT_OF_HOST_MEMORY";
	case CL_PROFILING_INFO_NOT_AVAILABLE:       return "CL_PROFILING_INFO_NOT_AVAILABLE";
	case CL_MEM_COPY_OVERLAP:                   return "CL_MEM_COPY_OVERLAP";
	case CL_IMAGE_FORMAT_MISMATCH:              return "CL_IMAGE_FORMAT_MISMATCH";
	case CL_IMAGE_FORMAT_NOT_SUPPORTED:         return "CL_IMAGE_FORMAT_NOT_SUPPORTED";
	case CL_BUILD_PROGRAM_FAILURE:              return "CL_BUILD_PROGRAM_FAILURE";
	case CL_MAP_FAILURE:                        return "CL_MAP_FAILURE";
	case CL_MISALIGNED_SUB_BUFFER_OFFSET:       return "CL_MISALIGNED_SUB_BUFFER_OFFSET";                          //-13
	case CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST:    return "CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST";   //-14
	case CL_COMPILE_PROGRAM_FAILURE:            return "CL_COMPILE_PROGRAM_FAILURE";                               //-15
	case CL_LINKER_NOT_AVAILABLE:               return "CL_LINKER_NOT_AVAILABLE";                                  //-16
	case CL_LINK_PROGRAM_FAILURE:               return "CL_LINK_PROGRAM_FAILURE";                                  //-17
	case CL_DEVICE_PARTITION_FAILED:            return "CL_DEVICE_PARTITION_FAILED";                               //-18
	case CL_KERNEL_ARG_INFO_NOT_AVAILABLE:      return "CL_KERNEL_ARG_INFO_NOT_AVAILABLE";                         //-19
	case CL_INVALID_VALUE:                      return "CL_INVALID_VALUE";
	case CL_INVALID_DEVICE_TYPE:                return "CL_INVALID_DEVICE_TYPE";
	case CL_INVALID_PLATFORM:                   return "CL_INVALID_PLATFORM";
	case CL_INVALID_DEVICE:                     return "CL_INVALID_DEVICE";
	case CL_INVALID_CONTEXT:                    return "CL_INVALID_CONTEXT";
	case CL_INVALID_QUEUE_PROPERTIES:           return "CL_INVALID_QUEUE_PROPERTIES";
	case CL_INVALID_COMMAND_QUEUE:              return "CL_INVALID_COMMAND_QUEUE";
	case CL_INVALID_HOST_PTR:                   return "CL_INVALID_HOST_PTR";
	case CL_INVALID_MEM_OBJECT:                 return "CL_INVALID_MEM_OBJECT";
	case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR:    return "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR";
	case CL_INVALID_IMAGE_SIZE:                 return "CL_INVALID_IMAGE_SIZE";
	case CL_INVALID_SAMPLER:                    return "CL_INVALID_SAMPLER";
	case CL_INVALID_BINARY:                     return "CL_INVALID_BINARY";
	case CL_INVALID_BUILD_OPTIONS:              return "CL_INVALID_BUILD_OPTIONS";
	case CL_INVALID_PROGRAM:                    return "CL_INVALID_PROGRAM";
	case CL_INVALID_PROGRAM_EXECUTABLE:         return "CL_INVALID_PROGRAM_EXECUTABLE";
	case CL_INVALID_KERNEL_NAME:                return "CL_INVALID_KERNEL_NAME";
	case CL_INVALID_KERNEL_DEFINITION:          return "CL_INVALID_KERNEL_DEFINITION";
	case CL_INVALID_KERNEL:                     return "CL_INVALID_KERNEL";
	case CL_INVALID_ARG_INDEX:                  return "CL_INVALID_ARG_INDEX";
	case CL_INVALID_ARG_VALUE:                  return "CL_INVALID_ARG_VALUE";
	case CL_INVALID_ARG_SIZE:                   return "CL_INVALID_ARG_SIZE";
	case CL_INVALID_KERNEL_ARGS:                return "CL_INVALID_KERNEL_ARGS";
	case CL_INVALID_WORK_DIMENSION:             return "CL_INVALID_WORK_DIMENSION";
	case CL_INVALID_WORK_GROUP_SIZE:            return "CL_INVALID_WORK_GROUP_SIZE";
	case CL_INVALID_WORK_ITEM_SIZE:             return "CL_INVALID_WORK_ITEM_SIZE";
	case CL_INVALID_GLOBAL_OFFSET:              return "CL_INVALID_GLOBAL_OFFSET";
	case CL_INVALID_EVENT_WAIT_LIST:            return "CL_INVALID_EVENT_WAIT_LIST";
	case CL_INVALID_EVENT:                      return "CL_INVALID_EVENT";
	case CL_INVALID_OPERATION:                  return "CL_INVALID_OPERATION";
	case CL_INVALID_GL_OBJECT:                  return "CL_INVALID_GL_OBJECT";
	case CL_INVALID_BUFFER_SIZE:                return "CL_INVALID_BUFFER_SIZE";
	case CL_INVALID_MIP_LEVEL:                  return "CL_INVALID_MIP_LEVEL";
	case CL_INVALID_GLOBAL_WORK_SIZE:           return "CL_INVALID_GLOBAL_WORK_SIZE";                           //-63
	case CL_INVALID_PROPERTY:                   return "CL_INVALID_PROPERTY";                                   //-64
	case CL_INVALID_IMAGE_DESCRIPTOR:           return "CL_INVALID_IMAGE_DESCRIPTOR";                           //-65
	case CL_INVALID_COMPILER_OPTIONS:           return "CL_INVALID_COMPILER_OPTIONS";                           //-66
	case CL_INVALID_LINKER_OPTIONS:             return "CL_INVALID_LINKER_OPTIONS";                             //-67
	case CL_INVALID_DEVICE_PARTITION_COUNT:     return "CL_INVALID_DEVICE_PARTITION_COUNT";                     //-68
//    case CL_INVALID_PIPE_SIZE:                  return "CL_INVALID_PIPE_SIZE";                                  //-69
//    case CL_INVALID_DEVICE_QUEUE:               return "CL_INVALID_DEVICE_QUEUE";                               //-70    

	default:
		return "UNKNOWN ERROR CODE";
	}
}