#pragma once
#include <vector>
#include <algorithm>
#include <string>

enum EGameModeTransfer{
	eGameModeDest, eGameModeMaxGame, eGameModeMaxGet, eGameModeMaxPay, eGameModeInterfaceMax
};

enum ERTModeTransfer{
	eRTModeInDest, eRTModeInMaxGame, eRTModeOutDest, eRTModeOutMaxGame, eRTModeInterfaceMax
};

struct SPayData {
	// BGMセットを後で別に定義
	int					gamemode;
	int					conbination;	// **[L]9876543210[C]9876543210[R]9876543210
	int					payout;			// each 2 digits(8bit): ********/3bet/2bet/1bet
	int					reachSoundID;
	std::string			name;
	int					gamemodeChange[eGameModeInterfaceMax];
	int					rtmodeChange[eRTModeInterfaceMax];
	int					effectID;
};

struct SPayLineData{
	int								betCount;
	int								maxPayNum;
	std::vector<std::string>		checkPosName;
	std::vector<std::vector<int>>	checkPos;
};

struct SCastData {
	std::vector<SPayData>		payData;
	std::vector<SPayLineData>	lineData;
};
