#pragma once
#include <vector>

// スベリT, 引込Tデータ
struct SControlTable {
	unsigned long long	data;		// 64bit変数 スベリTは49bit, 引込Tは21bit使用 [20,19,18,...,0]LSB
	unsigned int		activePos;	// (内部データ,slipTのみ使用)停止可能位置メモ
	int					refNum;		// 参照数指定、データの追加・削除に使用
};

// 引込T定義データ
struct SControlAvailableDef {
	/* 4bit -> 1bit:反転フラグ(0:なし,1:あり), 1bit:フラグ(0:非停止T,1:優先T), 2bit:シフト(00:無, 01:下1コマ, 10: 上1コマ, 11:特定1ヶ所) */
	unsigned char						tableFlag;
	unsigned char						availableID;		// 引込T_ID(0-255) or 特定箇所(1-21)
};

struct SControlDataSA {
	std::vector<SControlAvailableDef>	data;				// 引込T指定 最大4つ*2(LR)まで定義できる
};

struct SControlMakeData2nd {
	// 以下は位置毎に管理し、書き出すときに使用するもののみ適用
	unsigned int						activeFlag;			// 停止可否(0:非停止/1:停止, max32bit)
	/* 00:2nd押し位置スベリT(コード上未使用), 01:2nd停止位置スベリT, 10:2nd停止位置引込T, 11:2nd/3rd共通引込T */
	std::vector<unsigned char>			controlStyle2nd;	// use 2bit*m_comaMax 1stがPushの時は無視され00扱いになる
	std::vector<unsigned char>			controlData2ndPS;	// 2nd押し位置(Push)スベリ(Slip)T 8bit:SlipT_ID * reelComa*2
	std::vector<unsigned char>			controlData2ndSS;	// 2nd停止位置スベリ(Slip)T 8bit:SlipT_ID * reelComa(非停止位置も定義)
	std::vector<SControlDataSA>			controlData2ndSA;	// 2nd停止位置(Stop)引込(Available)T 8bit:AvailT_ID * reelComa(非停止位置も定義)
	std::vector<SControlDataSA>			controlDataComSA;	// 2nd/3rd停止位置(Stop)引込(Available)T 8bit:AvailT_ID * reelComa(3rd使いまわし)
};

struct SControlMakeData3rd {
	unsigned int						activeFlag1st;		// 停止可否1st(0:非停止/1:停止, max32bit)
	std::vector<unsigned long long>		activeFlag2nd;		// 停止可否2nd(0:非停止/1:停止, max64bit 1st左→右) 1st存在しない場所含む
	std::vector<SControlDataSA>			availableData;		// 引込T(1個ずつ定義する) order: (1st,2nd,LR) = 00L, 00R, 01L ... 0KR, 10L, 10R, ...
};

struct SControlMakeDataWrapper {
	unsigned char						controlData1st;		// 8bit:SlipT_ID
	SControlMakeData2nd					controlData2nd;		// 2nd停止データ(3rd共通時は3rdも使う)
	SControlMakeData3rd					controlData3rd;		// 3rd停止データ
};

struct SControlMakeData {
	unsigned char						dataID;				// use 8bit
	unsigned char						controlStyle;		// use 3bit(LCR, 0:Push, 1:Stop)
	std::vector<SControlMakeDataWrapper>controlData;		// 制御データ(LCR)
};