#include "_header/CGetSysDataFromCSV.hpp"
#include "DxLib.h"

bool CGetSysDataFromCSV::FileInit(int pFileID){
	DxLib::FileRead_seek(pFileID, 0, SEEK_SET);
	while (!DxLib::FileRead_eof(pFileID)){
		TCHAR str[1024];
		DxLib::FileRead_gets(str, 1024, pFileID);
		m_ReadDataAll.append(str).append("\n");
	}
	return !m_ReadDataAll.empty();
}

int CGetSysDataFromCSV::GetSysDataID(std::string pIdentifier){
	m_ReadDataNowCurrent = 0;
	while (!DataEOF()){
		StringArr NowGetStr;
		GetStrSplitByComma(NowGetStr);
		if (NowGetStr.size() < 3) continue;
		if (NowGetStr[0] != "#sysData") continue;
		if (NowGetStr[1] == pIdentifier){
			int ans;
			StrToNum(ans, NowGetStr[2]);
			return ans;
		}
	}
	return -1;
}
