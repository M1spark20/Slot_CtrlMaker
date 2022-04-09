#pragma once
#include <vector>

struct SControlPositionData {
	int currentFlagID;			// 現在参照中フラグID
	int selectReel;				// 選択中リール(押し順ID管理)
	int currentOrder;			// 現在の押し位置(0:1st, 1:2nd, 2:3rd, ...)
	int stop1st;				// 現在の第一停止
	std::vector<int> cursorComa;// 現在の参照中コマ(1st,2nd,3rd)
	int selectAvailID;			// 参照引込T番号
	bool isWatchLeft;			// 第2停止以降選択時参照先が左側か
	
	// 20220408追加 制御確認用サブ表示追加
	std::vector<int> subFlagList;	// 別フラグ定義可能位置一覧(=制御参考データ表示フラグID)
	int subFlagPos;					// 定義をサブ表示するsubFlagListの要素位置
};