
#include "Config.hpp"


void check_advanced_сonfig(ConfigClass* config)
{
		if ((config->num_generated_keys % config->num_files) != 0) {
			std::cerr << "ERROR CHECK config_advanced.cfg FILE: config->number_of_generated_keys % config->num_files != 0" << std::endl;
			throw std::logic_error("error check config_advanced.cfg file");
		}
		if (config->num_generated_keys < config->num_files) {
			std::cerr << "ERROR CHECK config_advanced.cfg FILE: config->number_of_generated_keys < config->num_files" << std::endl;
			throw std::logic_error("error check config_advanced.cfg file");
		}
		if ((config->num_generated_keys / config->num_files) < config->num_keys_gpu_worksize) {
			std::cerr << "ERROR CHECK config_advanced.cfg FILE: ((config->num_generated_keys / config->num_files) < config->num_keys_gpu_worksize)" << std::endl;
			throw std::logic_error("error check config_advanced.cfg file");
		}
}

void parse_config(ConfigClass* config, std::string path)
{
	try {
		const tao::config::value v = tao::config::from_file(path);
		config->folder_save_keys = access(v, tao::config::key("folder_save_keys")).get_string();
		config->folder_sort_keys = access(v, tao::config::key("folder_sort_keys")).get_string();
		config->folder_8_bytes_keys = access(v, tao::config::key("folder_8_bytes_keys")).get_string();
	}
	catch (std::runtime_error& e) {
		std::cerr << "Error parse config.cfg file " << path << " : " << e.what() << '\n';
		throw std::logic_error("error parse config.cfg file");
	}
	catch (...) {
		std::cerr << "Error parse config.cfg file, unknown exception occured" << std::endl;
		throw std::logic_error("error parse config.cfg file");
	}
}


void parse_advanced_config(ConfigClass* config, std::string path)
{
	try {
		const tao::config::value v = tao::config::from_file(path);
		config->gpu_local_size = access(v, tao::config::key("gpu_local_size")).get_unsigned();
		config->init_random_seed = access(v, tao::config::key("init_random_seed")).get_unsigned();
		config->num_keys_gpu_worksize = std::strtoull(access(v, tao::config::key("num_keys_gpu_worksize")).get_string().c_str(), NULL, 16);
		config->num_generated_keys = std::strtoull(access(v, tao::config::key("num_generated_keys")).get_string().c_str(), NULL, 16);
		config->folder_to_save_sort_csv = access(v, tao::config::key("folder_to_save_sort_csv")).get_string();
		config->folder_to_save_sort_8_bytes_csv = access(v, tao::config::key("folder_to_save_sort_8_bytes_csv")).get_string();
		config->num_files = access(v, tao::config::key("num_files")).get_unsigned();
		config->bytes_check = access(v, tao::config::key("bytes_check")).get_unsigned();
		config->num_files_to_csv = access(v, tao::config::key("num_files_to_csv")).get_unsigned();
	}
	catch (std::runtime_error& e) {
		std::cerr << "Error parse config_advanced.cfg file " << path << " : " << e.what() << '\n';
		throw std::logic_error("error parse config_advanced.cfg file");
	}
	catch (...) {
		std::cerr << "Error parse config_advanced.cfg file, unknown exception occured" << std::endl;
		throw std::logic_error("error parse config_advanced.cfg file");
	}
	try {
		check_advanced_сonfig(config);
	}
	catch (...) {
		throw std::logic_error("error check config_advanced.cfg file");
	}
}






