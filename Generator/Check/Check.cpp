
#include "Check.hpp"
#include "utils.hpp"
#include "Config/Config.hpp"
#include "profanity_cpu.hpp"
#include <random>
#include <precomp.hpp>
#include "Data/Buffer.h"

size_t genPrivateKeys(private_key* priv_keys, size_t size, size_t init_value) {

	for (size_t i = 0; i < size; i++)
	{
		std::mt19937_64 eng(init_value);
		std::uniform_int_distribution<cl_ulong> distr;

		priv_keys[i].key.s[0] = distr(eng);
		priv_keys[i].key.s[1] = distr(eng);
		priv_keys[i].key.s[2] = distr(eng);
		priv_keys[i].key.s[3] = distr(eng);

		//printPrivateKey(L"Dispather PrivateKeys ", &priv_keys[i]);
		init_value++;
	}

	return init_value;
}

int Check(ConfigClass& config, DataClass& Data)
{
	int ret = 0;
	size_t num_file = 0;
	std::string filename = getFileName(config.folder_save_keys, num_file, "bin");
	size_t filesize = getBinaryFileSize(filename);
	if (filesize % 64 != 0) {
		std::cout << "Error file (" << filename << ") size (" << filesize << ")" << std::endl;
		return -1;
	}
	ret = Data.host.MallocResult(filesize/64);
	if (ret) {
		std::cout << "Error MallocResult size (" << filesize << ")" << std::endl;
		return ret;
	}
	getFromBinaryFile(filename, Data.host.getResultBuffer(), Data.host.getSizeResultBuffer(), 0);
	
	for (size_t i = 0; i < Data.host.getNumResult(); i++)
	{
		private_key privateKey;
		point publicKey;
		genPrivateKeys(&privateKey, 1, i);
		gen_public_key(g_precomp, &privateKey, &publicKey);
		point publicKeyFromFile = Data.host.getResultBuffer()[i];
		for (size_t ii = 0; ii < 8; ii++)
		{
			if (publicKey.x.d[ii] != publicKeyFromFile.x.d[ii]) {
				std::cout << "Error Check Public Keys in File (" << filename << ")" << std::endl;
				printPoint("publicKey :", &publicKey);
				printPoint("publicKeyFromFile :", &publicKeyFromFile);
				return -1;
			}
			if (publicKey.y.d[ii] != publicKeyFromFile.y.d[ii]) {
				std::cout << "Error Check Public Keys in File (" << filename << ")" << std::endl;
				printPoint("publicKey :", &publicKey);
				printPoint("publicKeyFromFile :", &publicKeyFromFile);
				return -1;
			}
		}


	}



	

	//public_key publicKeyFromBin;
	//memcpy(&publicKeyFromBin, data, 64);
	//printPublicKey("public key from file:", &publicKeyFromBin);


	//private_key privateKey;
	//public_key publicKey;
	//genPrivateKeys(&privateKey, 1, 0);

	//printPrivateKey("private key from gen :", &privateKey);
	//gen_public_key(g_precomp, &privateKey, &publicKey);
	//printPublicKey("public key from gen :", &publicKey);
	std::cout << "End Check";
	return 0;
}



int CheckTest(ConfigClass& config)
{


	private_key privateKey;
	point publicKey;
	genPrivateKeys(&privateKey, 1, config.init_random_seed);

	//printPrivateKey("private key from gen :", &privateKey);
	gen_public_key(g_precomp, &privateKey, &publicKey);
	//printPublicKey("public key from gen :", &publicKey);
	std::cout << "End Check";
	return 0;
}
