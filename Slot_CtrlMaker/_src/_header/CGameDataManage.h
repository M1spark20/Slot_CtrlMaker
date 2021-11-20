#pragma once
#include "SMainReadFileIndex.h"
#include <vector>
class CGameDataManage{
// [act]ゲーム本体の画像管理
public:
	typedef std::vector<SMainReadFileIndex> ArrayIndex;

private:
	ArrayIndex m_MainData;
	bool DeleteData(unsigned int DeletePos,EMainReadFileType DataType);

public:

	bool StartReadFile(ArrayIndex &InputData);
	bool CheckReadFile();
	int GetDataHandle(unsigned int DataID) const;
	const SMainReadFileIndex* GetDataIndex(unsigned int DataID) const;
	~CGameDataManage();
};
