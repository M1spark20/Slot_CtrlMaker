#pragma once
#include <vector>

struct SControlPositionData {
	int currentFlagID;			// ���ݎQ�ƒ��t���OID
	int selectReel;				// �I�𒆃��[��(������ID�Ǘ�)
	int currentOrder;			// ���݂̉����ʒu(0:1st, 1:2nd, 2:3rd, ...)
	int stop1st;				// ���݂̑���~
	std::vector<int> cursorComa;// ���݂̎Q�ƒ��R�}(1st,2nd,3rd)
	int selectAvailID;			// �Q�ƈ���T�ԍ�
	bool isWatchLeft;			// ��2��~�ȍ~�I�����Q�Ɛ悪������
	
	// 20220408�ǉ� ����m�F�p�T�u�\���ǉ�
	std::vector<int> subFlagList;	// �ʃt���O��`�\�ʒu�ꗗ(=����Q�l�f�[�^�\���t���OID)
	int subFlagPos;					// ��`���T�u�\������subFlagList�̗v�f�ʒu
};