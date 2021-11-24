#pragma once
#include <vector>

// ����T��`�f�[�^
struct SControlAvailableDef {
	/* 3bit -> 1bit:�t���O(0:���~T,1:�D��T), 2bit:�V�t�g(00:��, 01:��1�R�}, 10: ��1�R�}, 11:���]) */
	unsigned char						tableFlag;
	unsigned char						availableID;		// ����T_ID(0-255)
};

// 2nd��~�ʒu(Stop)�X�x��(Slip)T �f�[�^�^
struct SControlDataSS {
	unsigned int						activeFlag;			// ��~��(0:���~/1:��~, max32bit)
	std::vector<unsigned char>			activeSlipID;		// 2nd�����ʒu(Push)�X�x��(Slip)T 8bit:SlipT_ID (��~�\�ȏꏊ�̂ݒ�`)
};

// 2nd(/3rd)��~�ʒu(Stop)����(Available)T �f�[�^�^
struct SControlDataSA {
	unsigned int						activeFlag;			// ��~��(0:���~/1:��~, max32bit)
	std::vector<SControlAvailableDef>	availableData;		// ����T(2����`����)
};

struct SControlMakeData2nd {
	// �ȉ��͎g�p���镨�݂̂Ƀf�[�^������
	std::vector<unsigned char>			controlData2ndPS;	// 2nd�����ʒu(Push)�X�x��(Slip)T 8bit:SlipT_ID * reelComa
	std::vector<SControlDataSS>			controlData2ndSS;	// 2nd��~�ʒu�X�x��(Slip)T
	std::vector<SControlDataSA>			controlData2ndSA;	// 2nd��~�ʒu(Stop)����(Available)T
	std::vector<SControlDataSA>			controlDataComSA;	// 2nd/3rd��~�ʒu(Stop)����(Available)T (3rd�g���܂킵)
};

struct SControlMakeData3rd {
	unsigned int						activeFlag1st;		// ��~��1st(0:���~/1:��~, max32bit)
	std::vector<unsigned int>			activeFlag2nd;		// ��~��1st(0:���~/1:��~, max32bit) 1st���݂���ꏊ�̂�
	std::vector<SControlAvailableDef>	availableData;		// ����T(1����`����)
};

struct SControlMakeDataWrapper {
	unsigned char						controlData1st;		// 8bit:SlipT_ID
	SControlMakeData2nd					controlData2nd;		// 2nd��~�f�[�^(3rd���ʎ���3rd���g��)
	SControlMakeData3rd					controlData3rd;		// 3rd��~�f�[�^
};

struct SControlMakeData {
	unsigned char						dataID;				// use 8bit
	unsigned char						controlStyle;		// use 6bit
	SControlMakeDataWrapper				controlData;		// reelNum����`����
};