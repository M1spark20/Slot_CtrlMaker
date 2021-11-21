#pragma once

struct SReelDrawData{
//[act] リール描画用構造体
	unsigned int reelID;
	float x;
	float y;
	float comaW;
	float comaH;
	float offsetUpper;
	float offsetLower;
	unsigned int comaNum;
};

struct SReelDrawDataFromCSV {
	struct { int x, y, w, h; } srcPos, dstPos;
	int a;
	int extendModeID, blendModeID;
	int destScr, preDrawScr;		// csvデータからDxLib側のスクリーンハンドルに変換したものを入力
	int blew, blewTime;
	int reelID, comaNum, originComa, comaBegin, comaIndexMax;
};
