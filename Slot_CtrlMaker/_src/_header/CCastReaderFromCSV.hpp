#pragma once
#include "IReadCSV.h"
#include "SCastData.hpp"
#include <vector>

class CCastReaderFromCSV : IReadCSVBase {
public:
	bool FileInit();
	bool FileInit(int pFileID);
	bool MakeData(SCastData& p_Data);
};
