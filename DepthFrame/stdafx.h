#pragma once


#include "../../Common/Common/common.h"
//using namespace common;
#include "../../Common/Graphic11/graphic11.h"
#include "../../Common/Framework11/framework11.h"
#include "../../Common/Network/network.h"

#include <Shlobj.h>

// Kinect Header files
#include <Kinect.h>

// OpenCV
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/objdetect/objdetect.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv/cvaux.h>
#include <opencv2/calib3d.hpp>



#include "graphbuff.h"
#include "sensorbuffer.h"
#include "plyreader.h"
#include "datreader.h"
#include "contour.h"
#include "volume.h"
#include "dbclient.h"
#include "root.h"
#include "logview.h"
#include "sensor.h"
#include "calibration.h"


extern framework::cGameMain2 * g_application;
extern cRoot g_root;
