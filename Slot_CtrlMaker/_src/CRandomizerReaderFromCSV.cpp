#include "_header/CRandomizerReaderFromCSV.hpp"
#include "_header/SRandomizerData.hpp"
#include "DxLib.h"

bool CRandomizerReaderFromCSV::FileInit(){
// [act]リール演出抽選データが格納されたCSVを開く
// [ret]ファイルオープンに成功したかどうか
	const char* FileName = "data/flug_table.csv";
	return StartReadFile(FileName);
}

bool CRandomizerReaderFromCSV::FileInit(int pFileID){
	// [act]DxLib側で開いたファイルからデータを読み出す
	// [ret]ファイルオープンに成功したかどうか
	while (!DxLib::FileRead_eof(pFileID)){
		TCHAR str[1024];
		DxLib::FileRead_gets(str, 1024, pFileID);
		m_ReadDataAll.append(str).append("\n");
	}
	return !m_ReadDataAll.empty();
}

bool CRandomizerReaderFromCSV::MakeData(SRandomizerData& p_Data){
	typedef unsigned int UINT;
	while( !DataEOF() ){
		StringArr NowGetStr;
		GetStrSplitByComma(NowGetStr);
		if(NowGetStr.at(0)[0] == ';') continue;
		if(NowGetStr.at(0) == "F"){
			std::array<int,eFlagTypeMax> flagData;
			StrToNum(flagData[eFlagID] ,		NowGetStr.at(1));
			StrToNum(flagData[eBonusID],		NowGetStr.at(2));
			StrToNum(flagData[eIsReachCheck],	NowGetStr.at(4));
			p_Data.flagType.push_back(flagData);
			p_Data.flagName.push_back(NowGetStr.at(5));
		} else if(NowGetStr.at(0) == "T") {
			SRandTable newData;
			int flagMax;
			StrToNum(flagMax,			NowGetStr.at(1));
			StrToNum(newData.set,		NowGetStr.at(2));
			StrToNum(newData.bet,		NowGetStr.at(3));
			StrToNum(newData.gameMode,	NowGetStr.at(4));
			StrToNum(newData.rtMode,	NowGetStr.at(5));
			StrToNum(newData.randMax,	NowGetStr.at(6));

			newData.randSeed.resize(flagMax+1);
			int randSum = 0;
			for (unsigned int i = 0; i < flagMax + 1; ++i){
				StrToNum(newData.randSeed[i], NowGetStr.at(7 + i));
				randSum += newData.randSeed[i];
			}
			// 乱数合計チェック
			if (newData.randMax != randSum) return false;
			p_Data.randomizerData.push_back(newData);
		} else if (NowGetStr.at(0) == "R"){
			std::pair<int, int> rtMode;
			StrToNum(rtMode.first,	NowGetStr.at(1));
			StrToNum(rtMode.second,	NowGetStr.at(2));
			p_Data.rtModeAtBonusHit.push_back(rtMode);
		} else if (NowGetStr.at(0) == "B"){
			std::pair<int, int> betData;
			StrToNum(betData.first,	NowGetStr.at(1));
			StrToNum(betData.second,NowGetStr.at(2));
			betData.second &= 0x7;
			p_Data.betAvailable.push_back(betData);
		}
	}
	return true;
}
