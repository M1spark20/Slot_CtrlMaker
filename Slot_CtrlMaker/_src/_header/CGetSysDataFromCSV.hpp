#pragma once
#include "IReadcsv.h"

class CGetSysDataFromCSV : IReadCSVBase{
public:
	bool FileInit(int pFileID);
	int GetSysDataID(std::string pIdentifier);
};
