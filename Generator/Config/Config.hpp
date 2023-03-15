#ifndef HPP_CONFIG
#define HPP_CONFIG

#include <string>
#include <tao/config.hpp>


struct ConfigClass
{
public:
	std::string folder_save_keys = "";
	std::string folder_sort_keys = "";
	std::string folder_8_bytes_keys = "";

	std::string folder_to_save_sort_csv = "";
	std::string folder_to_save_sort_8_bytes_csv = "";
	size_t gpu_local_size = 64;
	size_t init_random_seed = 0;
	size_t num_keys_gpu_worksize = 0x400000; //0x400000
	size_t num_generated_keys = 0x100000000; //0x100000000
	size_t num_files = 256;
	size_t bytes_check = 8;
	size_t num_files_to_csv = 2;
public:
	ConfigClass()
	{
	}
	~ConfigClass()
	{
	}
};


void parse_config(ConfigClass* config, std::string path);
void parse_advanced_config(ConfigClass* config, std::string path);
#endif /* HPP_CONFIG */
