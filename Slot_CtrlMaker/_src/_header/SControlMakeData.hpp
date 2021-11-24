#pragma once
#include <vector>

// 引込T定義データ
struct SControlAvailableDef {
	/* 3bit -> 1bit:フラグ(0:非停止T,1:優先T), 2bit:シフト(00:無, 01:下1コマ, 10: 上1コマ, 11:反転) */
	unsigned char						tableFlag;
	unsigned char						availableID;		// 引込T_ID(0-255)
};

// 2nd停止位置(Stop)スベリ(Slip)T データ型
struct SControlDataSS {
	unsigned int						activeFlag;			// 停止可否(0:非停止/1:停止, max32bit)
	std::vector<unsigned char>			activeSlipID;		// 2nd押し位置(Push)スベリ(Slip)T 8bit:SlipT_ID (停止可能な場所のみ定義)
};

// 2nd(/3rd)停止位置(Stop)引込(Available)T データ型
struct SControlDataSA {
	unsigned int						activeFlag;			// 停止可否(0:非停止/1:停止, max32bit)
	std::vector<SControlAvailableDef>	availableData;		// 引込T(2個ずつ定義する)
};

struct SControlMakeData2nd {
	// 以下は使用する物のみにデータを入れる
	std::vector<unsigned char>			controlData2ndPS;	// 2nd押し位置(Push)スベリ(Slip)T 8bit:SlipT_ID * reelComa
	std::vector<SControlDataSS>			controlData2ndSS;	// 2nd停止位置スベリ(Slip)T
	std::vector<SControlDataSA>			controlData2ndSA;	// 2nd停止位置(Stop)引込(Available)T
	std::vector<SControlDataSA>			controlDataComSA;	// 2nd/3rd停止位置(Stop)引込(Available)T (3rd使いまわし)
};

struct SControlMakeData3rd {
	unsigned int						activeFlag1st;		// 停止可否1st(0:非停止/1:停止, max32bit)
	std::vector<unsigned int>			activeFlag2nd;		// 停止可否1st(0:非停止/1:停止, max32bit) 1st存在する場所のみ
	std::vector<SControlAvailableDef>	availableData;		// 引込T(1個ずつ定義する)
};

struct SControlMakeDataWrapper {
	unsigned char						controlData1st;		// 8bit:SlipT_ID
	SControlMakeData2nd					controlData2nd;		// 2nd停止データ(3rd共通時は3rdも使う)
	SControlMakeData3rd					controlData3rd;		// 3rd停止データ
};

struct SControlMakeData {
	unsigned char						dataID;				// use 8bit
	unsigned char						controlStyle;		// use 6bit
	SControlMakeDataWrapper				controlData;		// reelNum分定義する
};