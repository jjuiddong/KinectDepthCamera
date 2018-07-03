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



#include "../sensor/graphbuff.h"
#include "../sensor/sensorbuffer.h"
#include "../sensor/plyreader.h"
#include "../sensor/datreader.h"
#include "../sensor/contour.h"
#include "../sensor/volume.h"
#include "../sensor/dbclient.h"
#include "../sensor/sensor.h"
#include "../sensor/calibration.h"
#include "../sensor/baslersync.h"
#include "../sensor/kinectv2.h"
#include "../sensor/cvutil.h"
#include "../view/logview.h"
#include "../view/root.h"

extern framework::cGameMain2 * g_application;
extern cRoot g_root;
