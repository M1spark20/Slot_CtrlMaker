#pragma once
#include "IReadcsv.h"
#include "CGameDataManage.h"
struct SReachCollectionData;

class CReadReachCollectionFromCSV : IReadCSVBase {
	static const int COMA_MAX = 3;
public:
	bool FileInit(int pFileID);
	bool MakeData(SReachCollectionData& pData);
};
