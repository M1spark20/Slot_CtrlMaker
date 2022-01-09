#pragma once
#include "IReadcsv.h"
#include <vector>
struct SStopPosData;

class CStopPosDataReaderFromCSV : IReadCSVBase {
	bool IsCheckExactStr(std::string& pStr, std::string pFindFor);
public:
	bool FileInit(int pFileID);
	bool MakeData(std::vector<SStopPosData>& p_Data, const int pComaNum);
};