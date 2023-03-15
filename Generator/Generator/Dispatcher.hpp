#ifndef HPP_DISPATCHER_GENERATOR
#define HPP_DISPATCHER_GENERATOR

#include <stdexcept>
#include <fstream>
#include <string>
#include <vector>
#include <mutex>

#if defined(__APPLE__) || defined(__MACOSX)
#include <OpenCL/cl.h>
#define clCreateCommandQueueWithProperties clCreateCommandQueue
#else
#include <CL/cl.h>
#endif

#include "SpeedSample.hpp"
#include "CLMemory.hpp"
#include "types.hpp"
#include "Mode.hpp"
#include "Config/Config.hpp"

class DispatcherGenerator {
public:
	struct HostBuffersClass
	{
	public:
		point* result = NULL;
		size_t result_alloc_size = 0;
		size_t all_alloc_size = 0;
		size_t lenght = 0;
	public:
		HostBuffersClass()
		{
		}
		HostBuffersClass(size_t size, bool malloc = false)
		{
			if (malloc) Malloc(size);
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
			if (NULL == *point) { fprintf(stderr, "_aligned_malloc (%s) failed! Size: %s", buff_name.c_str(), FormatWithCommas(size).data()); return 1; }
			*all_ram_memory_size += size;
			std::cout << "malloc ram memory size (" << buff_name << "): " << std::to_string((float)size / (1024.0f * 1024.0f)) << " MB\n";
			return 0;
		}

		int Malloc(size_t len)
		{
			all_alloc_size = 0;
			result_alloc_size = len * sizeof(point);
			if (alignedMalloc((uint8_t**)&result, len * sizeof(point), &all_alloc_size, "result") != 0) return -1;
			//std::cout << "MALLOC ALL RAM MEMORY SIZE (ALL): " << std::to_string((float)all_alloc_size / (1024.0f * 1024.0f)) << " MB\n";
			lenght = len;
			return 0;
		}

		~HostBuffersClass()
		{
			_aligned_free(result);
		}

	};


private:
	class OpenCLException : public std::runtime_error {
	public:
		OpenCLException(const std::string s, const cl_int res);

		static void throwIfError(const std::string s, const cl_int res);

		const cl_int m_res;
	};
	struct Device {
		Device(DispatcherGenerator& parent,
			cl_context& clContext,
			cl_program& clProgram,
			cl_device_id clDeviceId,
			ConfigClass& config);
		~Device();

		DispatcherGenerator& parent;
		ConfigClass& config;

		cl_device_id clDeviceId;
		cl_command_queue clQueue;
		cl_kernel kernelGenKeys;


		CLMemory<point> memPrecomp;
		CLMemory<private_key> memPrivateKey;
		CLMemory<point> memResult;

		size_t currSeedRandom;
		size_t keysHandled;

		// Seed and round information
		cl_ulong round;

		// Speed sampling
		SpeedSample speed;

		// Initialization
		size_t sizeInitialized;
		cl_event eventFinished;

		static cl_command_queue createQueue(cl_context& clContext, cl_device_id& clDeviceId);
		static cl_kernel createKernel(cl_program& clProgram, const std::string s);
		size_t genPrivateKeys(private_key* priv_keys, size_t size, size_t init_value);
		void copyToResult();
		void printResult();

	public:
		HostBuffersClass dataStore;
	};

public:
	DispatcherGenerator(cl_context& clContext, cl_program& clProgram, ConfigClass& config);
	~DispatcherGenerator();

	void addDevice(cl_device_id clDeviceId, ConfigClass& config);
	void run();

private:
	void init();
	void initBegin();
	void initContinue();

	void dispatch();
	void enqueueKernel(cl_command_queue& clQueue, cl_kernel& clKernel, size_t worksizeGlobal, const size_t worksizeLocal, cl_event* pEvent);
	void enqueueKernelDevice(cl_kernel& clKernel, size_t worksizeGlobal, cl_event* pEvent);

	bool isEnd();
	int handleResult();
	void writeResultToFile();
	int saveResult();

	void onEvent(cl_event event, cl_int status);
	void onEventInit(cl_event event, cl_int status, Device& d);
	void printSpeed();

private:
	static void CL_CALLBACK staticCallback(cl_event event, cl_int event_command_exec_status, void* user_data);
	static void CL_CALLBACK initCallback(cl_event event, cl_int event_command_exec_status, void* user_data);
	static std::string formatSpeed(double s);

public:
	Device* Dev;

private: /* Instance variables */
	cl_context& clContext;
	cl_program& clProgram;
	ConfigClass& config;

	size_t keysHandled;

	cl_event eventFinished;
	// Run information
	std::mutex mtx;
	std::chrono::time_point<std::chrono::steady_clock> timeStart;
	size_t sizeInitTotal;
	size_t sizeInitDone;
	bool quit;
};

#endif /* HPP_DISPATCHER_GENERATOR */
