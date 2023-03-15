
#include "Sort.hpp"
#include "utils.hpp"
#include "Config/Config.hpp"

#include <random>
#include <precomp.hpp>
#include <Data/Buffer.h>



int DistributionByFiles(ConfigClass& config, DataClass& Data)
{
	clearFiles(config.folder_sort_keys);
	int ret = 0;
	openBinaryFilesForWrite(Data.host.outstream, config.folder_sort_keys);
	for (size_t num_file = 0; num_file < config.num_files; num_file++) {
		double percent = (num_file == 0) ? 0 : (100.0 / ((double)config.num_files / (double)num_file));
		printPercent("PROCESS: ", percent);
		std::string filename = getFileName(config.folder_save_keys, num_file, "bin");
		size_t filesize = getBinaryFileSize(filename);
		if (filesize % 64 != 0) {
			std::cout << "Error file (" << filename << ") size (" << filesize << ")" << std::endl;
			return -1;
		}
		
		ret = Data.host.MallocTempBuffer(filesize);
		if (ret) {
			std::cout << "Error MallocTempBuffer size (" << filesize << ")" << std::endl;
			return ret;
		}
		getFromBinaryFile(filename, Data.host.getTempBuffer(), Data.host.getSizeTempBuffer(), 0);
		size_t numKeys = filesize / 64;
		uint8_t* data = Data.host.getTempBuffer();

		for (uint32_t i = 0; i < numKeys; i++)
		{
			uint8_t firstByte = data[64 * i + 31];
			uint8_t line[64 + 4];
			memcpy(line, &data[64 * i], 64);
			uint32_t seed = (config.num_generated_keys / config.num_files) * num_file + i;
			memcpy(&line[64], &seed, 4);
			writeToStream(Data.host.outstream[firstByte], line, 64 + 4);
		}
		Data.host.FreeTempBuffer();
	}
	closeBinaryFiles(Data.host.outstream);
	printPercent("PROCESS: ", 100.0);

	//std::cout << "\nEnd DistributionByFiles" << std::endl;
	return 0;
}


#define KEY_LEN 64
#define SEED_RAND_LEN 4
#define LINE_LEN (KEY_LEN + SEED_RAND_LEN)


struct KeyAndSeed {
	uint64_t key[KEY_LEN / 8];
	uint32_t seed;
};

bool compareKeyAndSeed(KeyAndSeed line1, KeyAndSeed line2)
{
	if (line1.key[3] < line2.key[3]) return true;
	else if (line1.key[3] > line2.key[3]) return false;

	if (line1.key[2] < line2.key[2]) return true;
	else if (line1.key[2] > line2.key[2]) return false;

	if (line1.key[1] < line2.key[1]) return true;
	else if (line1.key[1] > line2.key[1]) return false;

	if (line1.key[0] < line2.key[0]) return true;
	else if (line1.key[0] > line2.key[0]) return false;

	if (line1.key[7] < line2.key[7]) return true;
	else if (line1.key[7] > line2.key[7]) return false;

	if (line1.key[6] < line2.key[6]) return true;
	else if (line1.key[6] > line2.key[6]) return false;

	if (line1.key[5] < line2.key[5]) return true;
	else if (line1.key[5] > line2.key[5]) return false;

	if (line1.key[4] < line2.key[4]) return true;
	else if (line1.key[4] > line2.key[4]) return false;

	std::cout << "Found identical keys!!!!"<< std::endl;

	return true;
}


int SortInAscendingOrder(ConfigClass& config, DataClass& Data)
{
	int ret = 0;
	for (size_t num_file = 0; num_file < config.num_files; num_file++) {
		double percent = (num_file == 0) ? 0 : (100.0 / ((double)config.num_files / (double)num_file));
		printPercent("PROCESS: ", percent);

		std::string filename = getFileName(config.folder_sort_keys, num_file, "bin");
		size_t filesize = getBinaryFileSize(filename);
		if (filesize % LINE_LEN != 0) {
			std::cout << "Error file (" << filename << ") size (" << filesize << ")" << std::endl;
			return -1;
		}

		ret = Data.host.MallocTempBuffer(filesize);
		if (ret) {
			std::cout << "Error MallocTempBuffer size (" << filesize << ")" << std::endl;
			return ret;
		}
		getFromBinaryFile(filename, Data.host.getTempBuffer(), Data.host.getSizeTempBuffer(), 0);
		size_t numKeys = filesize / LINE_LEN;
		uint8_t* data = Data.host.getTempBuffer();
		std::vector<KeyAndSeed> lines;
		for (uint32_t i = 0; i < numKeys; i++)
		{
			KeyAndSeed line;
			memcpy(&line.key, &data[LINE_LEN * i], KEY_LEN);
			memcpy(&line.seed, &data[LINE_LEN * i + KEY_LEN], SEED_RAND_LEN);
			lines.push_back(line);
		}

		std::sort(lines.begin(), lines.end(), compareKeyAndSeed);
		if (lines.size() != numKeys) {
			std::cout << "Error Sort lines.size() != numKeys. lines.size() = " << lines.size() << ", numKeys = " << numKeys << std::endl;
			return -1;
		}

		for (uint32_t i = 0; i < numKeys; i++)
		{
			memcpy(&data[LINE_LEN * i], &lines[i].key, KEY_LEN);
			memcpy(&data[LINE_LEN * i + KEY_LEN], &lines[i].seed, SEED_RAND_LEN);
		}

		clearFile(filename.c_str());
		putToBinaryFile(filename, data, filesize);

		Data.host.FreeTempBuffer();
	}
	printPercent("PROCESS: ", 100.0);
	return 0;
}

int SortBinToCSV(ConfigClass& config, DataClass& Data)
{
	int ret = 0;
	if (config.num_files_to_csv == 0) {
		std::cout << "Skip SortBinToCSV" << std::endl;
		return 0;
	}
	for (size_t num_file = 0; num_file < config.num_files_to_csv; num_file++) {
		double percent = (num_file == 0) ? 0 : (100.0 / ((double)config.num_files_to_csv / (double)num_file));
		printPercent("PROCESS: ", percent);

		std::string filename = getFileName(config.folder_sort_keys, num_file, "bin");
		size_t filesize = getBinaryFileSize(filename);
		if (filesize % LINE_LEN != 0) {
			std::cout << "Error file (" << filename << ") size (" << filesize << ")" << std::endl;
			return -1;
		}

		ret = Data.host.MallocTempBuffer(filesize);
		if (ret) {
			std::cout << "Error MallocTempBuffer size (" << filesize << ")" << std::endl;
			return ret;
		}
		getFromBinaryFile(filename, Data.host.getTempBuffer(), Data.host.getSizeTempBuffer(), 0);
		size_t numKeys = filesize / LINE_LEN;
		uint8_t* data = Data.host.getTempBuffer();
		std::ofstream out;
		out.exceptions(std::ofstream::failbit | std::ofstream::badbit);
		std::string filename_csv = getFileName(config.folder_to_save_sort_csv, num_file, "csv");
		clearFile(filename_csv.c_str());
		openStreamToWriteTXT(filename_csv, out);
		for (uint32_t i = 0; i < numKeys; i++)
		{
			uint64_t* key = (uint64_t*)&data[LINE_LEN * i];
			uint32_t* seed = (uint32_t*)&data[LINE_LEN * i + KEY_LEN];
			std::ostringstream ss;
			ss << std::hex << std::setfill('0');
			ss <<
				std::setw(16) << key[7] <<
				std::setw(16) << key[6] <<
				std::setw(16) << key[5] <<
				std::setw(16) << key[4] <<
				std::setw(16) << key[3] <<
				std::setw(16) << key[2] <<
				std::setw(16) << key[1] <<
				std::setw(16) << key[0] << "," <<
				std::setw(8) << *seed << std::endl;
			const std::string line = ss.str();
			writeToStream(out, (void*)line.c_str(), line.length());
		}
		closeStream(out);
		Data.host.FreeTempBuffer();
	}

	std::cout << "End SortBinToCSV" << std::endl;
	return 0;
}


int Save8BytesKeys(ConfigClass& config, DataClass& Data)
{
	clearFiles(config.folder_8_bytes_keys);
	int ret = 0;
	openBinaryFilesForWrite(Data.host.outstream, config.folder_8_bytes_keys);
	for (size_t num_file = 0; num_file < config.num_files; num_file++) {
		double percent = (num_file == 0) ? 0 : (100.0 / ((double)config.num_files / (double)num_file));
		printPercent("PROCESS: ", percent);
		std::string filename = getFileName(config.folder_sort_keys, num_file, "bin");
		size_t filesize = getBinaryFileSize(filename);
		if (filesize % LINE_LEN != 0) {
			std::cout << "Error file (" << filename << ") size (" << filesize << ")" << std::endl;
			return -1;
		}

		ret = Data.host.MallocTempBuffer(filesize);
		if (ret) {
			std::cout << "Error MallocTempBuffer size (" << filesize << ")" << std::endl;
			return ret;
		}
		getFromBinaryFile(filename, Data.host.getTempBuffer(), Data.host.getSizeTempBuffer(), 0);
		size_t numKeys = filesize / LINE_LEN;
		uint8_t* data = Data.host.getTempBuffer();

		for (uint32_t i = 0; i < numKeys; i++)
		{
			uint8_t firstByte = data[LINE_LEN * i + 31];
			uint8_t line[8];
			memcpy(line, &data[LINE_LEN * i + (32 - 8)], 8);
			writeToStream(Data.host.outstream[firstByte], line, 8);
		}
		Data.host.FreeTempBuffer();
	}
	closeBinaryFiles(Data.host.outstream);
	printPercent("PROCESS: ", 100.0);

	//public_key publicKeyFromBin;
	//memcpy(&publicKeyFromBin, Data.host.getTempBuffer(), 64);
	//printPublicKey("public key from file:", &publicKeyFromBin);

	//std::cout << "End SaveBin8Bytes" << std::endl;
	return 0;
}

int SortBin8BytesToCSV(ConfigClass& config, DataClass& Data)
{
	int ret = 0;
	if (config.num_files_to_csv == 0) {
		std::cout << "Skip SortBin8BytesToCSV" << std::endl;
		return 0;
	}
	for (size_t num_file = 0; num_file < config.num_files_to_csv; num_file++) {
		double percent = (num_file == 0) ? 0 : (100.0 / ((double)config.num_files_to_csv / (double)num_file));
		printPercent("PROCESS: ", percent);

		std::string filename = getFileName(config.folder_8_bytes_keys, num_file, "bin");
		size_t filesize = getBinaryFileSize(filename);
		if (filesize % 8 != 0) {
			std::cout << "Error file (" << filename << ") size (" << filesize << ")" << std::endl;
			return -1;
		}

		ret = Data.host.MallocTempBuffer(filesize);
		if (ret) {
			std::cout << "Error MallocTempBuffer size (" << filesize << ")" << std::endl;
			return ret;
		}
		getFromBinaryFile(filename, Data.host.getTempBuffer(), Data.host.getSizeTempBuffer(), 0);
		size_t numKeys = filesize / 8;
		uint8_t* data = Data.host.getTempBuffer();
		std::ofstream out;
		out.exceptions(std::ofstream::failbit | std::ofstream::badbit);
		std::string filename_csv = getFileName(config.folder_to_save_sort_8_bytes_csv, num_file, "csv");
		clearFile(filename_csv.c_str());
		openStreamToWriteTXT(filename_csv, out);
		for (uint32_t i = 0; i < numKeys; i++)
		{
			uint64_t* key = (uint64_t*)&data[8 * i];
			std::ostringstream ss;
			ss << std::hex << std::setfill('0');
			ss <<
				std::setw(16) << key[0] << std::endl;
			const std::string line = ss.str();
			writeToStream(out, (void*)line.c_str(), line.length());
		}
		closeStream(out);
		Data.host.FreeTempBuffer();
	}

	std::cout << "End SortBin8BytesToCSV" << std::endl;
	return 0;
}

