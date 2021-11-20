#pragma once
#include <vector>

struct STouchInput {
	int x;
	int y;
	int ID;
	int dev;
};
typedef std::vector<STouchInput> STouchInputList;
