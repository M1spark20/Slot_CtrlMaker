#pragma once
#include <vector>

struct SReelChaUnit{
	int charaID;
	int imageID;
	int x, y, w, h;
};

struct SReelChaData{
	std::vector<SReelChaUnit>	reelData;	// リール切り出し位置(直接描画用)
	int							reelID;		// リールID
	int							rpm;		// リール回転速度
	int							accTime;	// リール加速時間
	std::vector<int>			arrayData;	// リール配列データ
};
