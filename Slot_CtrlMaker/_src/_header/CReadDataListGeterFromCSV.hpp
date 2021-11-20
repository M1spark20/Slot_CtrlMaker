#pragma once
#include "IReadcsv.h"
#include "CGameDataManage.h"
struct SMainReadFileIndex;

class CReadDataListGeterFromCSV : IReadCSVBase{
public:
	bool FileInit(std::string pFileName);
	bool MakeData(CGameDataManage::ArrayIndex& p_Data);
};
