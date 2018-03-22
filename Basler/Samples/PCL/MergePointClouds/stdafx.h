// stdafx.h : include file for standard system include files,
// or project-specific include files that are used frequently, but
// are changed infrequently
//

#pragma once
#ifdef _MSCVER
#  include <Windows.h>
#  include "targetver.h"
#endif

#pragma push_macro( "_SCL_SECURE_NO_WARNINGS" )
#define _SCL_SECURE_NO_WARNINGS
#include <pcl/visualization/cloud_viewer.h>
#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <pcl/common/transforms.h>

#include <ConsumerImplHelper/ToFCamera.h>

#include <vector>
#include <memory>
#include <iostream>
#include <algorithm>



#pragma pop_macro( "_SCL_SECURE_NO_WARNINGS" )
