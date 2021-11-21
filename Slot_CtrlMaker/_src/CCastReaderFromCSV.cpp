#include "_header/CCastReaderFromCSV.hpp"
#include "_header/SCastData.hpp"
#include "DxLib.h"

bool CCastReaderFromCSV::FileInit(){
// [act]配当データが格納されたCSVを開く
// [ret]ファイルオープンに成功したかどうか
	const char* FileName = "data/odds.csv";
	return StartReadFile(FileName);
}

bool CCastReaderFromCSV::FileInit(int pFileID){
// [act]DxLib側で開いたファイルからデータを読み出す
// [ret]ファイルオープンに成功したかどうか
	while (!DxLib::FileRead_eof(pFileID)){
		TCHAR str[1024];
		DxLib::FileRead_gets(str, 1024, pFileID);
		m_ReadDataAll.append(str).append("\n");
	}
	return !m_ReadDataAll.empty();
}

bool CCastReaderFromCSV::MakeData(SCastData& p_Data){
	typedef unsigned int UINT;
	while( !DataEOF() ){
		StringArr NowGetStr;
		GetStrSplitByComma(NowGetStr);
		if(NowGetStr.at(0) == "L") {
			SPayLineData nowData;
			StrToNum(nowData.betCount,	NowGetStr.at(1));
			StrToNum(nowData.maxPayNum,	NowGetStr.at(2));
			for(UINT i=3; ; ++i){
				std::string line;
				StrToNum(line, NowGetStr.at(i));
				if(line=="-") break;
				std::vector<int> offsetList;
				for (auto it = line.cbegin(); it != line.cend(); ++it)
					offsetList.push_back(*it - '0');
				nowData.checkPosName.push_back(line);
				nowData.checkPos.push_back(offsetList);
			}
			p_Data.lineData.push_back(nowData);
		} else if(NowGetStr.at(0) == "O") {
			int temp, conb = 0, payoutVal = 0;
			SPayData nowData;
			for(size_t i=0; i<3; ++i){
				std::string conbChar = NowGetStr.at(i+2);
				conb <<= 10;
				for(unsigned j=0; j<conbChar.size(); ++j){
					const char mark = conbChar[j] - '0';
					if(mark<0 || mark >= 10) continue; // invalid data
					conb |= (1 << (mark%10));
				}
				StrToNum(temp, NowGetStr.at(i+5));
				payoutVal |= (abs(temp) & 0xFF) << (8 * i);
				if (temp < 0) payoutVal |= (1 << (7 + 8*i ));
			}
			nowData.conbination = conb;
			nowData.payout = payoutVal;
			StrToNum(nowData.gamemode,							NowGetStr.at( 1));
			StrToNum(nowData.reachSoundID,						NowGetStr.at( 8));
			StrToNum(nowData.gamemodeChange[eGameModeDest],		NowGetStr.at( 9));

			// 必ず書き込みが入るとは限らない分先に初期化しておく
			nowData.gamemodeChange[eGameModeMaxGame ] = -1;
			nowData.gamemodeChange[eGameModeMaxGet  ] = -1;
			nowData.gamemodeChange[eGameModeMaxPay  ] = -1;
			nowData.rtmodeChange  [eRTModeOutDest   ] = -1;
			nowData.rtmodeChange  [eRTModeOutMaxGame] = -1;

			if (nowData.gamemodeChange[eGameModeDest] >= 0){
				if (NowGetStr.at(10).empty()){
					if (NowGetStr.at(11).empty()) return false;
					StrToNum(nowData.gamemodeChange[eGameModeMaxPay],	NowGetStr.at(11));
				} else {
					StrToNum(nowData.gamemodeChange[eGameModeMaxGame],	NowGetStr.at(10));
					StrToNum(nowData.gamemodeChange[eGameModeMaxGet],	NowGetStr.at(11));
				}
				StrToNum(nowData.rtmodeChange[eRTModeOutDest],		NowGetStr.at(14));
				StrToNum(nowData.rtmodeChange[eRTModeOutMaxGame],	NowGetStr.at(15));
			}

			StrToNum(nowData.rtmodeChange[eRTModeInDest],		NowGetStr.at(12));
			StrToNum(nowData.rtmodeChange[eRTModeInMaxGame],	NowGetStr.at(13));
			StrToNum(nowData.effectID,							NowGetStr.at(16));
			nowData.name =										NowGetStr.at(17);
			p_Data.payData.push_back(nowData);
		}
	}
	return true;
}
