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

#ifndef HPP_UTILS
#define HPP_UTILS

#include "CL\cl.h"
//#include <d3d9.h>
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
#include "types.hpp"
#include "Config/Config.hpp"

#pragma once


// Print useful information to the default output. Same usage as with printf
void LogInfo(const char* str, ...);

// Print error notification to the default output. Same usage as with printf
void LogError(const char* str, ...);

// Read OpenCL source code from fileName and store it in source. The number of read bytes returns in sourceSize
int ReadSourceFromFile(const char* fileName, char** source, size_t* sourceSize);

std::vector<cl_device_id> getAllDevices(cl_device_type deviceType = CL_DEVICE_TYPE_GPU);

template <typename T, typename U, typename V, typename W>
T clGetWrapper(U function, V param, W param2) {
	T t;
	function(param, param2, sizeof(t), &t, NULL);
	return t;
}

template <typename U, typename V, typename W>
std::string clGetWrapperString(U function, V param, W param2) {
	size_t len;
	function(param, param2, 0, NULL, &len);
	char* const szString = new char[len];
	function(param, param2, len, szString, NULL);
	std::string r(szString);
	delete[] szString;
	return r;
}

std::string readFile(const char* const szFilename);

template <typename T> bool printResult(const T& t, const cl_int& err) {
	std::cout << ((t == NULL) ? TranslateOpenCLError(err) : "OK") << std::endl;
	return t == NULL;
}

bool printProgramBuildInfo(const cl_int err, cl_program program, cl_device_id device);


bool printResult(const cl_int err);

std::vector<std::string> getBinaries(cl_program& clProgram);
unsigned int getKernelExecutionTimeMicros(cl_event& e);



bool incUlong4(cl_ulong4* s);
bool incUlong4Profanity(cl_ulong4* s);
bool isUlong4Zero(cl_ulong4 s);
void printPublicKey(char* title, public_key* pubKey);
void printPrivateKey(char* title, private_key* privKey);
void printPoint(char* title, point* point);
void printAddress(char* title, mp_number* num);
void printMpNumber(char* title, mp_number* num);
void printPercent(std::string title, double percent);
int clearFile(const char* filename);
void clearFiles(std::string folder);
void putToBinaryFile(std::string  filename, void* data, size_t size);
void getFromBinaryFile(std::string filename, void* data, size_t size, size_t position);
void readBinaryFile(std::string filename, void* data, size_t size);
size_t getBinaryFileSize(std::string filename);
std::string getFileName(std::string path, size_t num_file, std::string extension);
void printBinaryResult(uint64_t* data, size_t len);
void stringToPublicKey(std::string str, public_key* pubKey);
void stringToPrivateKey(std::string str, private_key* privKey);
void copyPubKey(public_key* pubKey1, const public_key* pubKey2);
bool isPubKeysEqual(const public_key* pubKey1, const public_key* pubKey2);
private_key calcKeyFromResult(private_key privKey, result_crack r);
std::string TranslateOpenCLError(cl_int errorCode);

void openBinaryFilesForWrite(std::ofstream *out, std::string path);
void openBinaryFilesForRead(std::ifstream* in, std::string path);
void closeBinaryFiles(std::ofstream out[]);
void writeToStream(std::ofstream& out, void* data, size_t size);
void openStreamToWriteTXT(std::string filename, std::ofstream& out);
void closeStream(std::ofstream& out);
void closeInStreams(std::ifstream in[]);
int getNumFileFromNum(size_t num, size_t all_num, size_t num_files);
#endif /* HPP_UTILS */

