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
};