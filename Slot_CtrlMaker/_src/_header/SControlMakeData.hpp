#pragma once
#include <vector>

// �X�x��T, ����T�f�[�^
struct SControlTable {
	unsigned long long	data;		// 64bit�ϐ� �X�x��T��49bit, ����T��21bit�g�p [20,19,18,...,0]LSB
	unsigned int		activePos;	// (�����f�[�^,slipT�̂ݎg�p)��~�\�ʒu����
	int					refNum;		// �Q�Ɛ��w��A�f�[�^�̒ǉ��E�폜�Ɏg�p
};

// ����T��`�f�[�^
struct SControlAvailableDef {
	/* 4bit -> 1bit:���]�t���O(0:�Ȃ�,1:����), 1bit:�t���O(0:���~T,1:�D��T), 2bit:�V�t�g(00:��, 01:��1�R�}, 10: ��1�R�}, 11:����1����) */
	unsigned char						tableFlag;
	unsigned char						availableID;		// ����T_ID(0-255) or ����ӏ�(1-21)
};

struct SControlDataSA {
	std::vector<SControlAvailableDef>	data;				// ����T�w�� �ő�4��*2(LR)�܂Œ�`�ł���
};

struct SControlMakeData2nd {
	// �ȉ��͈ʒu���ɊǗ����A�����o���Ƃ��Ɏg�p������̂̂ݓK�p
	unsigned int						activeFlag;			// ��~��(0:���~/1:��~, max32bit)
	/* 00:2nd�����ʒu�X�x��T(�R�[�h�㖢�g�p), 01:2nd��~�ʒu�X�x��T, 10:2nd��~�ʒu����T, 11:2nd/3rd���ʈ���T */
	std::vector<unsigned char>			controlStyle2nd;	// use 2bit*m_comaMax 1st��Push�̎��͖�������00�����ɂȂ�
	std::vector<unsigned char>			controlData2ndPS;	// 2nd�����ʒu(Push)�X�x��(Slip)T 8bit:SlipT_ID * reelComa*2
	std::vector<unsigned char>			controlData2ndSS;	// 2nd��~�ʒu�X�x��(Slip)T 8bit:SlipT_ID * reelComa(���~�ʒu����`)
	std::vector<SControlDataSA>			controlData2ndSA;	// 2nd��~�ʒu(Stop)����(Available)T 8bit:AvailT_ID * reelComa(���~�ʒu����`)
	std::vector<SControlDataSA>			controlDataComSA;	// 2nd/3rd��~�ʒu(Stop)����(Available)T 8bit:AvailT_ID * reelComa(3rd�g���܂킵)
};

struct SControlMakeData3rd {
	unsigned int						activeFlag1st;		// ��~��1st(0:���~/1:��~, max32bit)
	std::vector<unsigned long long>		activeFlag2nd;		// ��~��2nd(0:���~/1:��~, max64bit 1st�����E) 1st���݂��Ȃ��ꏊ�܂�
	std::vector<SControlDataSA>			availableData;		// ����T(1����`����) order: (1st,2nd,LR) = 00L, 00R, 01L ... 0KR, 10L, 10R, ...
};

struct SControlMakeDataWrapper {
	unsigned char						controlData1st;		// 8bit:SlipT_ID
	SControlMakeData2nd					controlData2nd;		// 2nd��~�f�[�^(3rd���ʎ���3rd���g��)
	SControlMakeData3rd					controlData3rd;		// 3rd��~�f�[�^
};

struct SControlMakeData {
	unsigned char						dataID;				// use 8bit
	unsigned char						controlStyle;		// use 3bit(LCR, 0:Push, 1:Stop)
	std::vector<SControlMakeDataWrapper>controlData;		// ����f�[�^(LCR)
};