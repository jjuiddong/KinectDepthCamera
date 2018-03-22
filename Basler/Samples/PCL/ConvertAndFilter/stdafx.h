// stdafx.h: include file for standard system include files
// or project-specific include files that are used frequently but
// changed infrequently
//

#pragma once

//#ifdef _MSC_VER
//#include "targetver.h"
//#include <tchar.h>
//#endif

//#ifdef _MSC_VER
#pragma push_macro( "_SCL_SECURE_NO_WARNINGS" )
#define _SCL_SECURE_NO_WARNINGS
//#endif

#include <pcl/point_types.h>
#include <pcl/point_cloud.h>
#include <pcl/visualization/pcl_visualizer.h> 
#include <pcl/visualization/cloud_viewer.h>
#include <pcl/filters/voxel_grid.h>

#include <stdio.h>

#include <ConsumerImplHelper/ToFCamera.h>
#include <iostream>


//#ifdef _MSC_VER
#pragma pop_macro( "_SCL_SECURE_NO_WARNINGS" )
//#endif

