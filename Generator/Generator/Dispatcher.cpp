
#include <stdexcept>
#include <iostream>
#include <thread>
#include <sstream>
#include <iomanip>
#include <random>
#include <thread>
#include <algorithm>

#include "Dispatcher.hpp"
#include "precomp.hpp"
#include "utils.hpp"
#include "Config/Config.hpp"

static std::string toHex(const uint8_t* const s, const size_t len) {
	std::string b("0123456789abcdef");
	std::string r;

	for (size_t i = 0; i < len; ++i) {
		const unsigned char h = s[i] / 16;
		const unsigned char l = s[i] % 16;

		r = r + b.substr(h, 1) + b.substr(l, 1);
	}

	return r;
}


DispatcherGenerator::OpenCLException::OpenCLException(const std::string s, const cl_int res) :
	std::runtime_error(s + " (res = " + toString(res) + ")"),
	m_res(res)
{

}

void DispatcherGenerator::OpenCLException::OpenCLException::throwIfError(const std::string s, const cl_int res) {
	if (res != CL_SUCCESS) {
		throw OpenCLException(s, res);
	}
}

cl_command_queue DispatcherGenerator::Device::createQueue(cl_context& clContext, cl_device_id& clDeviceId) {
	// nVidia CUDA Toolkit 10.1 only supports OpenCL 1.2 so we revert back to older functions for compatability
#ifdef PROFANITY_DEBUG
	//cl_command_queue_properties p = CL_QUEUE_PROFILING_ENABLE;
	cl_command_queue_properties p = NULL;
#else
	cl_command_queue_properties p = NULL;
#endif

#ifdef CL_VERSION_2_0
	const cl_command_queue ret = clCreateCommandQueueWithProperties(clContext, clDeviceId, &p, NULL);
#else
	const cl_command_queue ret = clCreateCommandQueue(clContext, clDeviceId, p, NULL);
#endif
	return ret == NULL ? throw std::runtime_error("failed to create command queue") : ret;
}

cl_kernel DispatcherGenerator::Device::createKernel(cl_program& clProgram, const std::string s) {
	cl_kernel ret = clCreateKernel(clProgram, s.c_str(), NULL);
	return ret == NULL ? throw std::runtime_error("failed to create kernel \"" + s + "\"") : ret;
}


//0xFFFFFFFFFFFFFFFF
//0xFFFFFFFFFFFFFFFE
//0xBAAEDCE6AF48A03B
//0xBFD25E8CD0364140
//(any random 256-bit number from 0x1 to 0xFFFF FFFF FFFF FFFF FFFF FFFF FFFF FFFE BAAE DCE6 AF48 A03B BFD2 5E8C D036 4140)
size_t DispatcherGenerator::Device::genPrivateKeys(private_key* priv_keys, size_t size, size_t init_value) {

	for (size_t i = 0; i < size; i++)
	{
		std::mt19937_64 eng(init_value);
		std::uniform_int_distribution<cl_ulong> distr;

		priv_keys[i].key.s[0] = distr(eng);
		priv_keys[i].key.s[1] = distr(eng);
		priv_keys[i].key.s[2] = distr(eng);
		priv_keys[i].key.s[3] = distr(eng);
		init_value++;
	}

	return init_value;
}


void DispatcherGenerator::Device::copyToResult() {
#pragma omp parallel for
	for (int i = 0; i < config.num_keys_gpu_worksize; i++) {
		for (int x = 0; x < 8; x++) {
			dataStore.result[i].x.d[x] = memResult.data()[i].x.d[x];
			dataStore.result[i].y.d[x] = memResult.data()[i].y.d[x];
		}
		//for (size_t x = 0; x < 4; x++) {
		//	dataStore.result[i].priv_key.key.s[x] = m_memPrivateKey.data()[i].key.s[x];
		//}
	}

}

void DispatcherGenerator::Device::printResult() {
	for (size_t i = 0; i < dataStore.lenght; i++)
	{
		printPoint("Dispather PubKeys ", &dataStore.result[i]);
	}
}

DispatcherGenerator::Device::Device(DispatcherGenerator& parent,
	cl_context& clContext,
	cl_program& clProgram,
	cl_device_id clDeviceId,
	ConfigClass& confg
) :
	parent(parent),
	clDeviceId(clDeviceId),
	config(confg),
	clQueue(createQueue(clContext, clDeviceId)),
	kernelGenKeys(createKernel(clProgram, "gen_public_keys")),
	memPrecomp(clContext, clQueue, CL_MEM_READ_ONLY | CL_MEM_HOST_WRITE_ONLY, sizeof(g_precomp), g_precomp),
	memPrivateKey(clContext, clQueue, CL_MEM_READ_ONLY | CL_MEM_HOST_WRITE_ONLY, config.num_keys_gpu_worksize),
	memResult(clContext, clQueue, CL_MEM_WRITE_ONLY | CL_MEM_HOST_READ_ONLY, config.num_keys_gpu_worksize),
	round(0),
	currSeedRandom(config.init_random_seed),
	speed(SPEED_SAMPLES),
	sizeInitialized(0),
	eventFinished(NULL),
	keysHandled(0)
{
	dataStore.Malloc(config.num_keys_gpu_worksize);
	currSeedRandom = genPrivateKeys(memPrivateKey.data(), config.num_keys_gpu_worksize, currSeedRandom);
}

DispatcherGenerator::Device::~Device() {

}
DispatcherGenerator::DispatcherGenerator(
	cl_context& clContext,
	cl_program& clProgram,
	ConfigClass& config) :
	clContext(clContext),
	clProgram(clProgram),
	config(config),
	keysHandled(0),
	sizeInitTotal(config.num_keys_gpu_worksize),
	sizeInitDone(0),
	eventFinished(NULL),
	quit(false),
	Dev(NULL)
{

}

DispatcherGenerator::~DispatcherGenerator() {

}

void DispatcherGenerator::addDevice(
	cl_device_id clDeviceId,
	ConfigClass& config) {
	Dev = new Device(*this, clContext, clProgram, clDeviceId, config);
}





void DispatcherGenerator::run() {
	eventFinished = clCreateUserEvent(clContext, NULL);
	timeStart = std::chrono::steady_clock::now();

	clearFiles(config.folder_save_keys);

	init();

	const auto timeInitialization = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - timeStart).count();
	std::cout << "Initialization time: " << timeInitialization << " seconds" << std::endl;

	clWaitForEvents(1, &eventFinished);
	clReleaseEvent(eventFinished);
	eventFinished = NULL;
}




void DispatcherGenerator::init() {



	std::wcout << L"Инициализация..." << std::endl;
	std::wcout << L"Может занять несколько минут..." << std::endl;
	sizeInitTotal = config.num_keys_gpu_worksize;
	sizeInitDone = 0;
	

	cl_event const pInitEvents = clCreateUserEvent(clContext, NULL);
	Dev->eventFinished = pInitEvents;
	initBegin();

	clWaitForEvents((cl_uint)1, &pInitEvents);
	Dev->eventFinished = NULL;
	clReleaseEvent(pInitEvents);


	std::cout << std::endl;
}

void DispatcherGenerator::initBegin() {

	// Write precompute table
	Dev->memPrecomp.write(true);
	Dev->memPrivateKey.write(true);

	// Kernel arguments - profanity_begin
	Dev->memPrecomp.setKernelArg(Dev->kernelGenKeys, 0);
	Dev->memPrivateKey.setKernelArg(Dev->kernelGenKeys, 1);
	Dev->memResult.setKernelArg(Dev->kernelGenKeys, 2);

	// Seed device
	initContinue();
}

void DispatcherGenerator::initContinue() {
	size_t sizeLeft = config.num_keys_gpu_worksize - Dev->sizeInitialized;
	size_t sizeInitLimit = config.num_keys_gpu_worksize / SPEED_SAMPLES;
	if (config.num_keys_gpu_worksize < SPEED_SAMPLES) sizeInitLimit = config.num_keys_gpu_worksize;

	// Print progress
	const size_t percentDone = sizeInitDone * 100 / sizeInitTotal;
	std::cout << "  " << percentDone << "%\r" << std::flush;

	if (sizeLeft) {
		cl_event event;
		const size_t sizeRun = std::min(sizeInitLimit, sizeLeft);
		const auto resEnqueue = clEnqueueNDRangeKernel(Dev->clQueue, Dev->kernelGenKeys, 1, &Dev->sizeInitialized, &sizeRun, NULL, 0, NULL, &event);
		OpenCLException::throwIfError("kernel queueing failed during initilization", resEnqueue);

		const cl_int ret = clFlush(Dev->clQueue);
		if (ret != CL_SUCCESS) {
			throw std::runtime_error("initContinue() Error clFlush - " + toString(ret));
		}

		//std::lock_guard<std::mutex> lock(mutex);
		Dev->sizeInitialized += sizeRun;
		sizeInitDone += sizeRun;

		const auto resCallback = clSetEventCallback(event, CL_COMPLETE, initCallback, Dev);
		OpenCLException::throwIfError("failed to set custom callback during initialization", resCallback);
	}
	else {
		// Printing one whole string at once helps in avoiding garbled output when executed in parallell
		const std::string strOutput = "  Generator programm initialized";
		std::cout << strOutput << std::endl;
		clSetUserEventStatus(Dev->eventFinished, CL_COMPLETE);

		cl_event event;
		Dev->memResult.read(false, &event);
		clFlush(Dev->clQueue);
		const auto res = clSetEventCallback(event, CL_COMPLETE, staticCallback, Dev);
		OpenCLException::throwIfError("failed to set custom callback", res);

	}
}

void DispatcherGenerator::enqueueKernel(cl_command_queue& clQueue, cl_kernel& clKernel, size_t worksizeGlobal, const size_t worksizeLocal, cl_event* pEvent = NULL) {
	const size_t worksizeMax = config.num_keys_gpu_worksize;
	size_t worksizeOffset = 0;
	while (worksizeGlobal) {
		const size_t worksizeRun = std::min(worksizeGlobal, worksizeMax);
		const size_t* const pWorksizeLocal = (worksizeLocal == 0 ? NULL : &worksizeLocal);
		const auto res = clEnqueueNDRangeKernel(clQueue, clKernel, 1, &worksizeOffset, &worksizeRun, pWorksizeLocal, 0, NULL, pEvent);
		OpenCLException::throwIfError("kernel queueing failed", res);

		worksizeGlobal -= worksizeRun;
		worksizeOffset += worksizeRun;
	}
}

void DispatcherGenerator::enqueueKernelDevice(cl_kernel& clKernel, size_t worksizeGlobal, cl_event* pEvent = NULL) {
	try {
		enqueueKernel(Dev->clQueue, clKernel, worksizeGlobal, Dev->config.gpu_local_size, pEvent);
	}
	catch (OpenCLException& e) {
		// If local work size is invalid, abandon it and let implementation decide
		if ((e.m_res == CL_INVALID_WORK_GROUP_SIZE || e.m_res == CL_INVALID_WORK_ITEM_SIZE) && Dev->config.gpu_local_size != 0) {
			std::cout << std::endl << "warning: local work size abandoned on GPU" << std::endl;
			Dev->config.gpu_local_size = 0;
			enqueueKernel(Dev->clQueue, clKernel, worksizeGlobal, Dev->config.gpu_local_size, pEvent);
		}
		else {
			throw;
		}
	}
}

void DispatcherGenerator::dispatch() {
	cl_event event;
	Dev->memPrivateKey.write(false);
	enqueueKernelDevice(Dev->kernelGenKeys, config.num_keys_gpu_worksize);
	Dev->memResult.read(false, &event);
	clFlush(Dev->clQueue);
	const auto res = clSetEventCallback(event, CL_COMPLETE, staticCallback, Dev);
	OpenCLException::throwIfError("failed to set custom callback", res);
}


bool DispatcherGenerator::isEnd() {
	if (keysHandled >= config.num_generated_keys)
	{
		return true;
	}
	return false;
}


void DispatcherGenerator::writeResultToFile() {			
	int num_file = getNumFileFromNum(Dev->keysHandled, config.num_generated_keys, config.num_files);
	if (num_file == -1) return;
	std::string filename = getFileName(config.folder_save_keys, num_file, "bin");
	putToBinaryFile(filename, Dev->dataStore.result, Dev->dataStore.result_alloc_size);
}

int DispatcherGenerator::saveResult() {
	writeResultToFile();
	return 0;
}


int DispatcherGenerator::handleResult() {

	Dev->copyToResult();
	Dev->keysHandled += Dev->config.num_keys_gpu_worksize;
	keysHandled += Dev->config.num_keys_gpu_worksize;
	return 0;
}

void DispatcherGenerator::onEvent(cl_event event, cl_int status) {
	if (status != CL_COMPLETE) {
		std::cout << "Dispatcher::onEvent - Got bad status: " << status << std::endl;
	}
	else {
		std::lock_guard<std::mutex> lock(mtx);
		++Dev->round;
		handleResult();
		//Dev->printResult();

		Dev->speed.sample((double)config.num_keys_gpu_worksize);
		printSpeed();

		//if overflow
		if (isEnd())  //overflow
		{
			quit = true;		
		}
		else {
			Dev->currSeedRandom = Dev->genPrivateKeys(Dev->memPrivateKey.data(), config.num_keys_gpu_worksize, Dev->currSeedRandom);
			dispatch();
		}
		saveResult();
		if (quit)
		{
			clSetUserEventStatus(eventFinished, CL_COMPLETE);
		}
	}
}
void DispatcherGenerator::onEventInit(cl_event event, cl_int status, Device& d) {
	if (status != CL_COMPLETE) {
		std::cout << "DispatcherGenerator::onEventInit - Got bad status: " << status << std::endl;
	}
	else if (d.eventFinished != NULL) {
		initContinue();
	}

}


void CL_CALLBACK DispatcherGenerator::staticCallback(cl_event event, cl_int event_command_exec_status, void* user_data) {
	Device* const pDevice = static_cast<Device*>(user_data);
	pDevice->parent.onEvent(event, event_command_exec_status);
	clReleaseEvent(event);
}
void CL_CALLBACK DispatcherGenerator::initCallback(cl_event event, cl_int event_command_exec_status, void* user_data) {
	Device* const pDevice = static_cast<Device*>(user_data);
	pDevice->parent.onEventInit(event, event_command_exec_status, *pDevice);
	clReleaseEvent(event);
}

// This is run when m_mutex is held.
void DispatcherGenerator::printSpeed() {

	std::string strGPUs;
	unsigned int i = 0;

	const auto saveResult = Dev->speed.getSpeed();
	std::ostringstream ss;
	ss << std::fixed << std::setprecision(3) << ((double)keysHandled/ (double)config.num_generated_keys)*100.0 << "%";
	const auto savePercent = ss.str();

	const std::string strVT100ClearLine = "\33[2K\r";
	std::cerr << strVT100ClearLine << "Speed: " << formatSpeed(saveResult) << ". " << savePercent << '\r' << std::flush;

}

std::string DispatcherGenerator::formatSpeed(double f) {
	const std::string S = " KMGT";

	unsigned int index = 0;
	while (f > 1000.0f && index < S.size()) {
		f /= 1000.0f;
		++index;
	}

	std::ostringstream ss;
	ss << std::fixed << std::setprecision(3) << (double)f << " " << S[index] << "H/s";
	return ss.str();
}
