#include "_header/CStopPosDataReaderFromCSV.hpp"
#include "_header/SStopPosData.hpp"
#include "DxLib.h"

bool CStopPosDataReaderFromCSV::FileInit(int pFileID) {
	// [act]DxLib側で開いたファイルからデータを読み出す
	// [ret]ファイルオープンに成功したかどうか
	while (!DxLib::FileRead_eof(pFileID)) {
		TCHAR str[1024];
		DxLib::FileRead_gets(str, 1024, pFileID);
		m_ReadDataAll.append(str).append("\n");
	}
	return !m_ReadDataAll.empty();
}

bool CStopPosDataReaderFromCSV::MakeData(std::vector<SStopPosData>& p_Data, const int pComaNum) {
	typedef unsigned int UINT;
	int nowRead = -1;
	while (!DataEOF()) {
		StringArr NowGetStr;
		GetStrSplitByComma(NowGetStr);
		if (nowRead == -1) {
			if (NowGetStr.at(0)[0] == ';') continue;
			if (NowGetStr.at(0) == "#start") nowRead = 0;
		} else {
			if (nowRead++ % (pComaNum + 2) <= 1) continue;
			if (pComaNum + 1 > NowGetStr.size()) return false;
			for (int comaC = 0; comaC < pComaNum; ++comaC) {
				std::string readStr = NowGetStr.at(1 + comaC);
				SStopPosData nowData; nowData.spotFlag = ""; nowData.reachLevel = 0;
				if (IsCheckExactStr(readStr, "!!")) nowData.reachLevel = 3;
				if (IsCheckExactStr(readStr, "!" )) nowData.reachLevel = 2;
				if (IsCheckExactStr(readStr, "@" )) nowData.reachLevel = 1;
				nowData.spotFlag = readStr;	// IsChechExtractStrでフラグ部分はクリアされる
				p_Data.push_back(nowData);
			}
		}
	}
	return true;
}

bool CStopPosDataReaderFromCSV::IsCheckExactStr(std::string& pStr, std::string pFindFor) {
	const auto findPos = pStr.find(pFindFor.c_str(), 0, pFindFor.size());
	if (findPos == std::string::npos) return false;
	pStr = pStr.substr(0, findPos);
	return true;
}