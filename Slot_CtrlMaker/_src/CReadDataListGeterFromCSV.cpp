#include "_header/CReadDataListGeterFromCSV.hpp"
#include "_header/SMainReadFileIndex.h"

bool CReadDataListGeterFromCSV::FileInit(std::string pFileName){
	// [act]リール演出抽選データが格納されたCSVを開く
	// [ret]ファイルオープンに成功したかどうか
	return StartReadFile(pFileName.c_str());
}

bool CReadDataListGeterFromCSV::MakeData(CGameDataManage::ArrayIndex& p_Data){
	StringArr NowGetStr;
	while (!DataEOF()){
		GetStrSplitByComma(NowGetStr);
		if (NowGetStr.at(0) == "#MainFile"){
			SMainReadFileIndex Temp;
			Temp.SoundStreamingFlag = 0;
			// enumに整数値はそのまま代入できないので、キャスト。
			int FileTypeHandle;
			StrToNum(FileTypeHandle, NowGetStr.at(1));
			Temp.FileType = (EMainReadFileType)FileTypeHandle;
			StrToNum(Temp.DataReadSyncFlag, NowGetStr.at(2));
			Temp.FileName = NowGetStr.at(3);
			if (Temp.FileType == Sound)
				StrToNum(Temp.SoundStreamingFlag, NowGetStr.at(4));
			p_Data.push_back(Temp);
		}
		NowGetStr.clear();
	}
	return true;
}
