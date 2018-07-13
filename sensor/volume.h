//
// 2018-05-28, jjuiddong
// volume global definition
//
#pragma once

#include "rectcontour.h"


// BoxVolume
struct sContourInfo 
{
	bool used; // use internal
	int level; // vertex count
	int loop;
	float lowerH;
	float upperH;
	bool duplicate;
	graphic::cColor color;
	cContour contour;
};


// Box Top Area Information
struct sAreaFloor
{
	int startIdx; // start height
	int endIdx;  // end height
	int maxIdx; // 가장 많이 분포한 높이 인덱스
	float avrHeight; // 높이 평균값
	int areaCnt;
	graphic::cColor color;
	sGraph<100> areaGraph;
	graphic::cVertexBuffer *vtxBuff;
};


// Box Information
struct sBoxInfo 
{
	float minVolume;
	float maxVolume;
	common::Vector3 pos;
	common::Vector3 volume;
	common::Vector3 box3d[13 * 2];
	u_int pointCnt;
	int loopCnt;
	graphic::cColor color;
};


// Box Volume Average
struct sAvrContour 
{
	bool check;
	int count;
	sContourInfo box;
	sBoxInfo result;
	common::Vector2 avrVertices[13]; // 평균 버텍스 위치, 최대 8개 꼭지점
	common::Vector3 vertices3d[13 * 2]; // 꼭지점 평균 버텍스 3D 위치
};


struct sMeasureVolume
{
	int id;
	float horz;
	float vert;
	float height;
	common::Vector3 pos;
	float volume;
	float vw;
	int pointCount;

	sContourInfo contour;
};


struct sMeasureResult
{
	int id; // measure id
	int type; // 1:delay measure, 2:snap measure
	vector<sMeasureVolume> volumes;
};
