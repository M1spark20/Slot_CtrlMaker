#pragma once
#include "IReadCSV.h"
struct SRandomizerData;

class CRandomizerReaderFromCSV : IReadCSVBase {
public:
	bool FileInit();
	bool FileInit(int pFileID);
	bool MakeData(SRandomizerData& p_Data);
};
