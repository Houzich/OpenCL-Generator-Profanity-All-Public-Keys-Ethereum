/*****************************************************************************
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <memory.h>
#include <windows.h>
#include <thread>
#include <vector>
#include <array>

#include "CL\cl.h"
#include "CL\cl_ext.h"

#include "utils.hpp"
#include "Generator/Generator.hpp"
#include <tao/config.hpp>
#include "Config/Config.hpp"
#include "Check/Check.hpp"
#include "Sort/Sort.hpp"
#include "Data/Buffer.h"

int main(int argc, char** argv) {
	setlocale(LC_ALL, "Russian");
	system("chcp 1251");

	ConfigClass config;
	try {
		parse_config(&config, "config.cfg");
		parse_advanced_config(&config, "config_advanced.cfg");
	}
	catch (...) {
		for (;;)
			std::this_thread::sleep_for(std::chrono::seconds(30));
	}

	std::vector<cl_device_id> vFoundDevices = getAllDevices();
	cl_device_id Device = NULL;

	std::cout << "Found video cards:" << std::endl;
	for (size_t i = 0; i < vFoundDevices.size(); ++i) {

		if (vFoundDevices[i] == NULL) continue;
		cl_device_id& deviceId = vFoundDevices[i];

		const auto strName = clGetWrapperString(clGetDeviceInfo, deviceId, CL_DEVICE_NAME);
		const auto computeUnits = clGetWrapper<cl_uint>(clGetDeviceInfo, deviceId, CL_DEVICE_MAX_COMPUTE_UNITS);
		const auto globalMemSize = clGetWrapper<cl_ulong>(clGetDeviceInfo, deviceId, CL_DEVICE_GLOBAL_MEM_SIZE);

		std::cout << "  [" << i << "]" << " GPU" << ": " << strName << ", " << globalMemSize << " bytes available" << std::endl;

	}
	std::cout << std::endl;
	size_t number_device = 0;
	std::cout << "Enter the number of the used video card: ";
	std::cin >> number_device;
	if (number_device >= vFoundDevices.size()) {
		std::cout << "ERROR: invalid device number: " << "  [" << number_device << "]\n";
		std::cout << "Total devices found: " << vFoundDevices.size() << "\n";
		return 1;
	}

	Device = vFoundDevices[number_device];
	if (Device == NULL) {
		return 1;
	}

	int mode = -1;
	while (mode == -1)
	{
		std::cout << "Enter mode [0..4]: ";
		std::cin >> mode;
		if ((mode < 0) || (mode > 4))
		{
			std::cout << "Invalid mode! Please select mode [0..4]\n";
			mode = -1;
		}
	}

	DataClass* Data = new DataClass(true);
	if (mode == 0)
	{
		std::cout << std::endl;
		std::cout << "\n*************** Generator Public Keys START! ********************\n";
		if (GenerateAllPublicKeys(&Device, config)) {
			std::cout << "!!!!!ERROR!!!!! GenerateAllPublicKeys!!!!!\n";
			std::cout << "!!!!!ERROR!!!!! GenerateAllPublicKeys!!!!!\n";
			std::cout << "!!!!!ERROR!!!!! GenerateAllPublicKeys!!!!!\n";
			goto exit;
		}
		//std::cout << "\n*************** Generator Public Keys END! **********************\n" << std::endl;
		std::cout << "\n\n*************** Distribution Keys By Files START! ********************\n";
		if (DistributionByFiles(config, *Data)) {
			std::cout << "!!!!!ERROR!!!!! DistributionByFiles!!!!!\n";
			std::cout << "!!!!!ERROR!!!!! DistributionByFiles!!!!!\n";
			std::cout << "!!!!!ERROR!!!!! DistributionByFiles!!!!!\n";
			goto exit;
		}
		//std::cout << "\n*************** Distribution Keys By Files END! **********************\n" << std::endl;
		std::cout << "\n\n*************** Sort Keys In Ascending Order START! ********************\n";
		if (SortInAscendingOrder(config, *Data)) {
			std::cout << "!!!!!ERROR!!!!! SortInAscendingOrder!!!!!\n";
			std::cout << "!!!!!ERROR!!!!! SortInAscendingOrder!!!!!\n";
			std::cout << "!!!!!ERROR!!!!! SortInAscendingOrder!!!!!\n";
			goto exit;
		}
		//std::cout << "\n*************** Sort Keys In Ascending Order END! ********************\n" << std::endl;
		std::cout << "\n\n*************** Save 8 Bytes From Keys START! ********************\n";
		if (Save8BytesKeys(config, *Data)) {
			std::cout << "!!!!!ERROR!!!!! Save8BytesKeys!!!!!\n";
			std::cout << "!!!!!ERROR!!!!! Save8BytesKeys!!!!!\n";
			std::cout << "!!!!!ERROR!!!!! Save8BytesKeys!!!!!\n";
			goto exit;
		}
		//std::cout << "\n*************** Save 8 Bytes From Keys END! ********************\n" << std::endl;
	}
	else if (mode == 1)
	{
		std::cout << "\n*************** Generator Public Keys START! ********************\n" << std::endl;
		GenerateAllPublicKeys(&Device, config);
		std::cout << "\n*************** Generator Public Keys END! **********************\n" << std::endl;
	}
	else if (mode == 2)
	{
		std::cout << "\n*************** Distribution Keys By Files START! ********************\n" << std::endl;
		DistributionByFiles(config, *Data);
		std::cout << "\n*************** Distribution Keys By Files END! **********************\n" << std::endl;
	}
	else if (mode == 3)
	{
		std::cout << "\n*************** Sort Keys In Ascending Order START! ********************\n" << std::endl;
		SortInAscendingOrder(config, *Data);
		std::cout << "\n*************** Sort Keys In Ascending Order END! ********************\n" << std::endl;
	}
	else if (mode == 4)
	{
		std::cout << "\n*************** Save 8 Bytes From Keys START! ********************\n" << std::endl;
		Save8BytesKeys(config, *Data);
		std::cout << "\n*************** Save 8 Bytes From Keys END! ********************\n" << std::endl;
	}
	else if (mode == 5)
	{
		Check(config, *Data);
	}
	else if (mode == 6)
	{
		SortBinToCSV(config, *Data);
	}
	else if (mode == 7)
	{
		SortBin8BytesToCSV(config, *Data);
	}

exit:
	std::cout << "\n\n";
	std::cout << "FINISH!!!!!\n";
	std::cout << "FINISH!!!!!\n";
	std::cout << "FINISH!!!!!\n";
	//test(argc, argv);
	for (;;)
		std::this_thread::sleep_for(std::chrono::seconds(30));
}

