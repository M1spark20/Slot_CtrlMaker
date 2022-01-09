#include "_header/CReadReachCollectionFromCSV.hpp"
#include "_header/SReachCollectionData.hpp"
#include "DxLib.h"

bool CReadReachCollectionFromCSV::FileInit(int pFileID) {
	DxLib::FileRead_seek(pFileID, 0, SEEK_SET);
	while (!DxLib::FileRead_eof(pFileID)) {
		TCHAR str[1024];
		DxLib::FileRead_gets(str, 1024, pFileID);
		m_ReadDataAll.append(str).append("\n");
	}
	return !m_ReadDataAll.empty();
}

bool CReadReachCollectionFromCSV::MakeData(SReachCollectionData& pData) {
	m_ReadDataNowCurrent = 0;

	while (!DataEOF()) {
		StringArr NowGetStr;
		GetStrSplitByComma(NowGetStr);
		if (NowGetStr.empty()) continue;
		if (NowGetStr[0] == "#reelNum") {
			if (NowGetStr.size() < 2) continue;
			StrToNum(pData.reelNum, NowGetStr[1]);
		}
		if (NowGetStr[0] == "#listImg") {
			if (NowGetStr.size() < 2) continue;
			StrToNum(pData.listImgID, NowGetStr[1]);
		}
		if (NowGetStr[0] == "#secretImg") {
			if (NowGetStr.size() < 6) continue;
			StrToNum(pData.secretImgID, NowGetStr[1]);
			StrToNum(pData.secretImgPos.imgX, NowGetStr[2]);
			StrToNum(pData.secretImgPos.imgY, NowGetStr[3]);
			StrToNum(pData.secretImgPos.imgW, NowGetStr[4]);
			StrToNum(pData.secretImgPos.imgH, NowGetStr[5]);
		}
		if (NowGetStr[0] == "#HazureNG") {
			for (auto it = NowGetStr.begin()+1; it != NowGetStr.end(); ++it) {
				if (it->empty()) break;
				int flagID;	StrToNum(flagID, *it);
				pData.hazureNGFlags.push_back(flagID);
			}
		}
		if (NowGetStr[0] == "#colle") {
			if (NowGetStr.size() < 9 + 4*pData.reelNum) continue;
			SReachCollectionElement element;
			bool isOnlyHazure = true;
			StrToNum(element.imgX, NowGetStr[1]);
			StrToNum(element.imgY, NowGetStr[2]);
			StrToNum(element.imgW, NowGetStr[3]);
			StrToNum(element.imgH, NowGetStr[4]);
			element.isLeft1st = (NowGetStr[5] == "T");
			element.isOnlyHazure = (NowGetStr[6] != "T");
			StrToNum(element.betNum, NowGetStr[7]);
			StrToNum(element.gameMode, NowGetStr[8]);
			element.isDefaultShown = (NowGetStr[9] == "T");
			
			const int reelElemNum = COMA_MAX + 1;
			const int begin = 10;
			for (int i = 0; i < pData.reelNum; ++i) {
				int dataVal = 0;
				if (NowGetStr[begin + i * reelElemNum] == "H" || NowGetStr[begin + i * reelElemNum] == "?") {
					dataVal = 0x3FFFFFFF;	// Hazure, ?
				} else if (NowGetStr[begin + i * reelElemNum] == "R") {
					dataVal = 0x40000000;	// Rotating
				} else if (NowGetStr[begin + i * reelElemNum] == "A") {
					dataVal = 0x3FFFFFFF;	// Any
				} else if (NowGetStr[begin + i * reelElemNum] == "*"){
					// comaMask
					for (int coma = 0; coma < COMA_MAX; ++coma) {
						const int index = begin + i * reelElemNum + 1 + coma;
						int comaData = 0;
						if (NowGetStr[index] == "*") { comaData = 0x3FF; }
						else {
							StrToNum(comaData, NowGetStr[index]);
							if (comaData < 0) comaData = ~abs(comaData);
							comaData &= 0x3FF;
						}
						dataVal |= (comaData << ((COMA_MAX - 1 - coma) * 10));
					}
				} else {
					StrToNum(dataVal, NowGetStr[begin + i * reelElemNum]);	// reelPos
					dataVal = -1 * abs(dataVal);
				}
				element.reelElement.push_back(dataVal);
			}
			element.dataID = (int)pData.elem.size();
			pData.elem.push_back(element);
		}
	}
	return true;
}
