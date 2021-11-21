#include "_header/CReelDataReaderFromCSV.hpp"
#include "_header/SReelChaData.hpp"
#include "_header/CGameDataManage.h"
#include "DxLib.h"

bool CReelDataReaderFromCSV::FileInit(){
// [act]リール配列・画像読み込みデータが格納されたCSVを開く
// [ret]ファイルオープンに成功したかどうか
	const char* FileName = "data/flug_table.csv";
	return StartReadFile(FileName);
}
bool CReelDataReaderFromCSV::FileInit(int pFileID){
// [act]DxLib側で開いたファイルからデータを読み出す
// [ret]ファイルオープンに成功したかどうか
	while (!DxLib::FileRead_eof(pFileID)){
		TCHAR str[1024];
		DxLib::FileRead_gets(str, 1024, pFileID);
		m_ReadDataAll.append(str).append("\n");
	}
	return !m_ReadDataAll.empty();
}

bool CReelDataReaderFromCSV::MakeData(std::vector<SReelChaData>& p_Data, const CGameDataManage& pRes){
	typedef unsigned int UINT;
	std::vector<SReelChaUnit> commonUnit;
	while (!DataEOF()){
		StringArr NowGetStr;
		GetStrSplitByComma(NowGetStr);
		int arrayDataHandle, charaNum;
		if (NowGetStr.at(0) == "#O"){
			SReelChaData newData;
			StrToNum(newData.reelID,	NowGetStr.at(1));
			StrToNum(arrayDataHandle,	NowGetStr.at(2));
			StrToNum(charaNum,			NowGetStr.at(3));
			StrToNum(newData.rpm,		NowGetStr.at(4));
			StrToNum(newData.accTime,	NowGetStr.at(5));

			newData.arrayData.resize(charaNum);
			const int dataHandle = pRes.GetDataHandle(arrayDataHandle);
			for (int i = 0; i < newData.arrayData.size(); ++i){
				DxLib::FileRead_seek(dataHandle, sizeof(int)*(3 * i + newData.reelID), SEEK_SET);
				int hoge;
				DxLib::FileRead_read(&hoge, sizeof(int), dataHandle);
				newData.arrayData[i] = hoge;
			}
			p_Data.push_back(newData);
		}
		if (NowGetStr.at(0) == "#C"){
			SReelChaUnit unit;
			StrToNum(unit.charaID	, NowGetStr.at(1));
			StrToNum(unit.imageID	, NowGetStr.at(2));
			StrToNum(unit.x			, NowGetStr.at(3));
			StrToNum(unit.y			, NowGetStr.at(4));
			StrToNum(unit.w			, NowGetStr.at(5));
			StrToNum(unit.h			, NowGetStr.at(6));
			commonUnit.push_back(unit);
		}
	}
	
	for (auto it = p_Data.begin(); it != p_Data.end(); ++it)
		it->reelData = commonUnit;
	return true;
}
