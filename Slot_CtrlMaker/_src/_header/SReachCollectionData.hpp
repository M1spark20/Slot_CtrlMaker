#pragma once
#include <vector>
#include <string>

struct SReachCollectionElement {
	int imgX, imgY, imgW, imgH;
	bool isLeft1st;
	bool isOnlyHazure;
	int betNum;
	int gameMode;
	bool isDefaultShown;
	int dataID;

	// 0x3FFFFFFF: Any/Hazure/?, 0x40000000: Rotating
	// +:30[isRotating], 20-29[U-ComaMask], 10-19[C-ComaMask], 0-9[D-ComaMask] 
	// 0,-:reelPos
	std::vector<int> reelElement;

	int compCount;
	std::string firstComp;
	std::string lastComp;

	SReachCollectionElement() {
		imgX = -1; imgY = -1; imgW = -1; imgH = -1;
		isLeft1st = true;
		isOnlyHazure = true;
		compCount = 0;
		betNum = 0;
		gameMode = 0;
		isDefaultShown = false;
		dataID = 0;
	}
};

struct SReachCollectionData {
	std::vector<SReachCollectionElement> elem;
	SReachCollectionElement secretImgPos;
	std::vector<int> hazureNGFlags;
	int reelNum;
	int listImgID, secretImgID;

	SReachCollectionData() { reelNum = -1; listImgID = -1; secretImgID = -1; }
};
