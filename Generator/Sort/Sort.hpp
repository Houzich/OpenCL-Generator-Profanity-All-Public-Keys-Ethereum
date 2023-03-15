#ifndef HPP_SORT
#define HPP_SORT

#include <string>
#include "Config/Config.hpp"
#include "Data/Buffer.h"

int DistributionByFiles(ConfigClass& config, DataClass& Data);
int SortInAscendingOrder(ConfigClass& config, DataClass& Data);
int SortBinToCSV(ConfigClass& config, DataClass& Data);
int Save8BytesKeys(ConfigClass& config, DataClass& Data);
int SortBin8BytesToCSV(ConfigClass& config, DataClass& Data);
#endif /* HPP_SORT */
