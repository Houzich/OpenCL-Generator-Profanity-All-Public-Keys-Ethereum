#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <cstdio>
#include <vector>
#include <map>
#include <set>

#if defined(__APPLE__) || defined(__MACOSX)
#include <OpenCL/cl.h>
#include <OpenCL/cl_ext.h> // Included to get topology to get an actual unique identifier per device
#else
#include <CL/cl.h>
#include <CL/cl_ext.h> // Included to get topology to get an actual unique identifier per device
#endif

#ifdef _DEBUG
#ifdef __x86_64
#define SOURCE_FILE_CODE_BIN_1 "x64/Debug/gen_public_keys.ir"
#define SOURCE_FILE_CODE "gen_public_keys.cl"
#else
#define SOURCE_FILE_CODE_BIN_1 "Debug/gen_public_keys.ir"
#define SOURCE_FILE_CODE "gen_public_keys.cl"
#endif //__x86_64
#define SOURCE_FILE_CODE_BIN_2 "Debug/gen_public_keys.ir"
#else
#ifdef __x86_64
#define SOURCE_FILE_CODE_BIN_1 "x64/Release/gen_public_keys.ir"
#define SOURCE_FILE_CODE "gen_public_keys.cl"
#else
#define SOURCE_FILE_CODE_BIN_1 "Release/gen_public_keys.ir"
#define SOURCE_FILE_CODE "gen_public_keys.cl"
#endif //__x86_64
#define SOURCE_FILE_CODE_BIN_2 "gen_public_keys.ir"
#endif //_DEBUG


#include "Config.hpp"
#include "Dispatcher.hpp"
#include "ArgParser.hpp"
#include "Mode.hpp"
#include "help.hpp"
#include "utils.hpp"
#include <random>




int GenerateAllPublicKeys(cl_device_id* device, ConfigClass& config) {
	try {



		std::string DeviceBinary;
		size_t DeviceBinarySize = 0;
		cl_int errorCode;


		std::ifstream fileIn1(SOURCE_FILE_CODE_BIN_1, std::ios::binary);
		std::ifstream fileIn2(SOURCE_FILE_CODE_BIN_2, std::ios::binary);
		if (fileIn1.is_open()) {
			DeviceBinary = std::string((std::istreambuf_iterator<char>(fileIn1)), std::istreambuf_iterator<char>());
			DeviceBinarySize = DeviceBinary.size();
		}
		else if (fileIn2.is_open()) {
			DeviceBinary = std::string((std::istreambuf_iterator<char>(fileIn2)), std::istreambuf_iterator<char>());
			DeviceBinarySize = DeviceBinary.size();
		}

		std::cout << "Initializing OpenCL..." << std::endl;
		std::cout << "  Creating context..." << std::flush;
		auto clContext = clCreateContext(NULL, (cl_uint)1, device, NULL, NULL, &errorCode);
		if (printResult(clContext, errorCode)) {
			return 1;
		}

		cl_program clProgram;
		if (DeviceBinarySize != 0) {
			// Create program from binaries
			std::cout << "  Loading kernel from binary..." << std::flush;
			const unsigned char** pKernel = new const unsigned char* [1];
			pKernel[0] = reinterpret_cast<const unsigned char*>(DeviceBinary.data());

			cl_int* pStatus = new cl_int[1];

			clProgram = clCreateProgramWithBinary(clContext, (cl_uint)1, device, &DeviceBinarySize, pKernel, pStatus, &errorCode);
			if (printResult(clProgram, errorCode)) {
				return 1;
			}
		}
		else {
			// Create a program from the kernel source
			std::cout << "  Compiling kernel..." << std::flush;
			const std::string strVanity = readFile(SOURCE_FILE_CODE);
			const char* szKernels[] = { strVanity.c_str() };

			clProgram = clCreateProgramWithSource(clContext, sizeof(szKernels) / sizeof(char*), szKernels, NULL, &errorCode);
			if (printResult(clProgram, errorCode)) {
				return 1;
			}
		}

		// Build the program
		std::cout << "  Building program..." << std::flush;
		if (printProgramBuildInfo(clBuildProgram(clProgram, (cl_uint)1, device, NULL, NULL, NULL), clProgram, *device)) {
			return 1;
		}
		std::cout << std::endl;

	
		DispatcherGenerator d(clContext, clProgram, config);

		d.addDevice(*device, config);
		d.run();
		clReleaseContext(clContext);
	
		//readBinaryFile(config.folder_to_save_results + '/' + "00.bin", d.Dev->dataStore.result, d.Dev->dataStore.result_alloc_size * 8);
		//printBinaryResult((uint64_t *)d.Dev->dataStore.result, d.Dev->dataStore.lenght * 8);

		return 0;
	} catch (std::runtime_error & e) {
		std::cout << "std::runtime_error - " << e.what() << std::endl;
	} catch (...) {
		std::cout << "unknown exception occured" << std::endl;
	}
	return 1;
}

