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

