#pragma once
#include "IReadcsv.h"
#include <vector>
struct SReelChaData;
class CGameDataManage;

class CReelDataReaderFromCSV : IReadCSVBase {
public:
	bool FileInit();
	bool FileInit(int pFileID);
	bool MakeData(std::vector<SReelChaData>& p_Data, const CGameDataManage& pRes);
};
