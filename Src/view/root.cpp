
#include "stdafx.h"
#include "root.h"
#include <ConsumerImplHelper/ToFCamera.h>
#include <iostream>
#include <iomanip> 
#include "3dview.h"
#include "depthview.h"
#include "depthview2.h"
#include "infraredview.h"
#include "filterview.h"

using namespace common;

cRoot::cRoot()
	: m_input(eInputType::BASLER)
	, m_isAutoSaveCapture(false)
	, m_isTryConnectBasler(true)
	, m_isAutoMeasure(false)
	, m_isPalete(false)
	, m_isConnectKinect(false)
	, m_baslerCam(true)
	, m_isGrabLog(false)
	, m_isGrabErrLog(false)
	, m_isRangeCulling(false)
	, m_cullRangeMin(-200, -20, -200)
	, m_cullRangeMax(200, 200, 200)
	, m_isCalcHorz(false)
	, m_groundPlane(Vector3(0, 1, 0), 0)
	, m_volumeCenter(0, 0, 0)
	, m_regionSize(50, 50)
	, m_isContinuousCalibrationPlane(false)
	, m_planeStandardDeviation(0)
	, m_configFileName("config_depthframe.txt")
	, m_isShowBox(true)
	, m_isShowBoxCandidate(true)
	, m_isShowBoxVertex(true)
	, m_isSave2DMat(false)
	, m_isShowBeforeContours(false)
	, m_masterSensor(2)
	, m_measureType(cMeasure::INTEGRAL)
	, m_hdistribSize(0,0)
{
	// Camera Offset Setting, Korean Air Cargo
	//m_cameraOffset[0].pos = Vector3(-59.740f, 4.170f, -75.420f);
	//m_cameraOffset[0].rot.SetRotationY(-0.02f);
	//m_cameraOffset[1].pos = Vector3(55.030f, -2.200f, 78.090f);	 // 30cm
	//m_cameraOffset[2].pos = Vector3(0, 0, 0);

	//m_planeSub[0] = Plane(Vector3(0,1,0), 0);
	//m_planeSub[1] = Plane(Vector3(0.010194f, 0.999914f, 0.008305f), 0);
	//m_planeSub[2] = Plane(Vector3(0.01611f, 0.999722f, 0.014008f), 0);

	// Test Room Setting
	//m_planeSub[0] = Plane(Vector3(-0.006403f, 0.999976f, 0.002532f), -0.598581f);
	//m_planeSub[1] = Plane(Vector3(0, 1, 0), 0);
	//m_planeSub[2] = Plane(Vector3(0, 1, 0), 0);

	for (int i = 0; i < MAX_CAMERA; ++i)
		m_devChannel[i] = -1;

	m_cullRect[0] = common::sRectf::Rect(0, -200, 200, 200); // Camera3
	m_cullRect[1] = common::sRectf::Rect(0, 0, 200, 200); // Camera2
	m_cullRect[2] = common::sRectf::Rect(-50, -50, 100, 100); // Camera5
	m_cullRect[3] = common::sRectf::Rect(-200, -200, 200, 200); // Camera4
	m_cullRect[4] = common::sRectf::Rect(-200, 0, 200, 200); // Camera1

	m_extraCullRect[0] = m_cullRect[2]; // Camera3
	m_extraCullRect[1] = m_cullRect[2]; // Camera2
	m_extraCullRect[2] = common::sRectf::Rect(0,0,0,0); // Camera5
	m_extraCullRect[3] = m_cullRect[2]; // Camera4
	m_extraCullRect[4] = m_cullRect[2]; // Camera1
}

cRoot::~cRoot()
{
	Clear();
}


bool cRoot::Create()
{
	if (m_config.Read(m_configFileName.c_str()))
	{
		m_isConnectKinect = m_config.GetBool("kinect_connect", true);
		m_isTryConnectBasler = m_config.GetBool("basler_connect", true);
		m_inputFilePath = m_config.GetString("inputfilepath", "../Media/Depth");

		m_regionCenter.x = m_config.GetFloat("calib-center-x");
		m_regionCenter.y = m_config.GetFloat("calib-center-y");
		m_regionCenter.z = m_config.GetFloat("calib-center-z");
		m_regionSize.x = m_config.GetFloat("calib-minmax-x", 50);
		m_regionSize.y = m_config.GetFloat("calib-minmax-y", 50);

		// Parse Device Channel 
		StrId id;
		for (int i = 0; i < MAX_CAMERA; ++i)
		{
			id.Format("camera-option-channel%d-x", i);
			m_devChannel[i] = m_config.GetInt(id.c_str(), -1);
		}
	}
	else
	{
		m_inputFilePath = "../Media/Depth";
	}

	m_3dEyePos = common::Vector3(0.f, 380.f, -300.f);
	m_3dLookAt = common::Vector3(0, 0, 0);

	m_timer.Create();

	return true;
}


bool cRoot::InitSensor()
{
	//if (m_isConnectKinect)
	//	m_kinect.Init();

	if (m_isTryConnectBasler)
	{
		m_baslerCam.Init();
	}
	else
	{
		// for BaslerCamSync::IsReadyCapture() return true
		m_baslerCam.m_state = cBaslerCameraSync::eThreadState::CONNECT_FAIL;
	}

	m_dbClient.Create();

	return true;// m_baslerCam.IsConnect();
}


// 센서를 종료한다.
bool cRoot::DisconnectSensor()
{
	//m_kinect.Clear();
	m_baslerCam.Clear();
	return true;
}


bool cRoot::KinectCapture()
{
	// nothing~ now
	return true;
}


// camIdx 카메라가 인식한 영상으로 볼륨 측정한다.
void cRoot::MeasureVolume(
	const bool isForceMeasure // = false
)
{
	// 순서 중요!!
	g_root.m_3dView->Capture3D();
	g_root.m_measure.MeasureVolume(m_measureType, isForceMeasure);
	g_root.m_filterView->Process();
	g_root.m_infraredView->Process(0);
	//((cViewer*)g_application)->m_depthView->Process();
	//((cViewer*)g_application)->m_depthView2->Process();
}


void cRoot::Update(const float deltaSeconds)
{
}


// Load Global Ground Plane
// Calibration에 관련된 변수들을 파일에 읽어온다.
bool cRoot::LoadPlane()
{
	if (!m_config.Read(m_configFileName.c_str()))
		return false;

	if (m_config.GetString("plane-x", "none") != "none")
	{
		m_groundPlane.N.x = m_config.GetFloat("plane-x");
		m_groundPlane.N.y = m_config.GetFloat("plane-y");
		m_groundPlane.N.z = m_config.GetFloat("plane-z");
		m_groundPlane.D = m_config.GetFloat("plane-d");
	}

	if (g_root.m_config.GetString("center-x", "none") != "none")
	{
		m_volumeCenter.x = m_config.GetFloat("center-x");
		m_volumeCenter.y = m_config.GetFloat("center-y");
		m_volumeCenter.z = m_config.GetFloat("center-z");
	}

	// Load PlaneSub
	for (int i = 0; i < MAX_CAMERA; ++i)
	{
		StrId id;

		id.Format("camera-offset%d-x", i);
		m_cameraOffset[i].pos.x = m_config.GetFloat(id.c_str(), 0);
		id.Format("camera-offset%d-y", i);
		m_cameraOffset[i].pos.y = m_config.GetFloat(id.c_str(), 0);
		id.Format("camera-offset%d-z", i);
		m_cameraOffset[i].pos.z = m_config.GetFloat(id.c_str(), 0);
		id.Format("camera-offset%d-angle", i);
		m_cameraOffsetYAngle[i] = m_config.GetFloat(id.c_str(), 0);
		m_cameraOffset[i].rot.SetRotationY( m_cameraOffsetYAngle[i] );

		id.Format("planesub%d-x", i);
		m_planeSub[i].N.x = m_config.GetFloat(id.c_str(), 0);
		id.Format("planesub%d-y", i);
		m_planeSub[i].N.y = m_config.GetFloat(id.c_str(), 1);
		id.Format("planesub%d-z", i);
		m_planeSub[i].N.z = m_config.GetFloat(id.c_str(), 0);
		id.Format("planesub%d-d", i);
		m_planeSub[i].D = m_config.GetFloat(id.c_str(), 0);
	}

	const bool isCullingRect = g_root.m_config.GetInt("culling-rect-enable", 0) > 0;

	// Parsing Culling Rect
	for (int i = 0; i < MAX_CAMERA; ++i)
	{
		StrId id;
		id.Format("culling-rect%d", i);
		{
			string tok = g_root.m_config.GetString(id.c_str());
			vector<string> out;
			common::tokenizer(tok, ",", "", out);
			if (!isCullingRect || (out.size() < 4))
			{
				m_cullRect[i] = common::sRectf::Rect(0, 0, 0, 0);
			}
			else
			{
				const common::sRectf rect = common::sRectf::Rect(
					(float)atof(out[0].c_str())
					, (float)atof(out[1].c_str())
					, (float)atof(out[2].c_str())
					, (float)atof(out[3].c_str()));
				m_cullRect[i] = rect;
			}
		}

		// Parsing extra-culling-rect
		id.Format("extra-culling-rect%d", i);
		{
			string tok = g_root.m_config.GetString(id.c_str());
			vector<string> out;
			common::tokenizer(tok, ",", "", out);
			if (!isCullingRect || (out.size() < 4))
			{
				m_extraCullRect[i] = common::sRectf::Rect(0, 0, 0, 0);
			}
			else
			{
				const common::sRectf rect = common::sRectf::Rect(
					(float)atof(out[0].c_str())
					, (float)atof(out[1].c_str())
					, (float)atof(out[2].c_str())
					, (float)atof(out[3].c_str()));
				m_extraCullRect[i] = rect;
			}
		}
	}


	// Parsing camera-mapping
	// ex) Camera1:0,Camera2:1
	string cfgCameraOffsetmap = g_root.m_config.GetString("camera-mapping");
	vector<string> tokCameraOffsetmap;
	common::tokenizer(cfgCameraOffsetmap, ",", "", tokCameraOffsetmap);

	vector<std::pair<string, int>> cameraOffsetmap;
	if (!tokCameraOffsetmap.empty())
	{
		for (const auto &tok : tokCameraOffsetmap)
		{
			if (tok.empty())
				continue;

			vector<string> out;
			common::tokenizer(tok, ":", "", out);
			if (out.size() < 2)
			{
				assert(0); // error occur
				continue;
			}

			cameraOffsetmap.push_back(std::make_pair<string, int>(out[0].c_str(), atoi(out[1].c_str())));
		}
	}

	for (u_int i = 0; i < m_baslerCam.m_sensors.size(); ++i)
	{
		auto &sensor = m_baslerCam.m_sensors[i];

		// Setting Offset Parameter
		if (cameraOffsetmap.empty())
		{
			sensor->m_buffer.m_offset = m_cameraOffset[i];
			sensor->m_buffer.m_planeSub = m_planeSub[i];
		}
		else
		{
			int parameterIdx = -1;
			for (u_int i = 0; i < cameraOffsetmap.size(); ++i)
			{
				const auto &val = cameraOffsetmap[i];
				if (val.first == sensor->m_info.strDisplayName)
				{
					common::dbg::Logp("%s Camera Parameter Idx = %d\n", sensor->m_info.strDisplayName.c_str(), val.second);
					parameterIdx = val.second;
					break;
				}
			}
			if ((parameterIdx < 0) || (parameterIdx >= MAX_CAMERA))
			{
				//assert(0);
				parameterIdx = (int)i;
			}

			sensor->m_buffer.m_offset = m_cameraOffset[parameterIdx];
			sensor->m_buffer.m_planeSub = m_planeSub[parameterIdx];
			sensor->m_buffer.m_cullRect = m_cullRect[parameterIdx];
			sensor->m_buffer.m_cullExtraRect = m_extraCullRect[parameterIdx];
		}
	}

	return true;
}


// Calibration에 관련된 변수들을 파일에 저장한다.
bool cRoot::SavePlane()
{
	if (m_groundPlane.N != Vector3(0,1,0))
	{
		m_config.SetValue("plane-x", m_groundPlane.N.x);
		m_config.SetValue("plane-y", m_groundPlane.N.y);
		m_config.SetValue("plane-z", m_groundPlane.N.z);
		m_config.SetValue("plane-d", m_groundPlane.D);
	}

	if (!m_volumeCenter.IsEmpty())
	{
		m_config.SetValue("center-x", m_volumeCenter.x);
		m_config.SetValue("center-y", m_volumeCenter.y);
		m_config.SetValue("center-z", m_volumeCenter.z);
	}

	// Save PlaneSub
	for (int i = 0; i < MAX_CAMERA; ++i)
	{
		StrId id;

		if (!m_cameraOffset[i].pos.IsEmpty())
		{
			id.Format("camera-offset%d-x", i);
			m_config.SetValue(id.c_str(), m_cameraOffset[i].pos.x);
			id.Format("camera-offset%d-y", i);
			m_config.SetValue(id.c_str(), m_cameraOffset[i].pos.y);
			id.Format("camera-offset%d-z", i);
			m_config.SetValue(id.c_str(), m_cameraOffset[i].pos.z);
			id.Format("camera-offset%d-angle", i);
			m_config.SetValue(id.c_str(), m_cameraOffsetYAngle[i]);
		}

		if (!m_planeSub[i].N.IsEmpty())
		{
			id.Format("planesub%d-x", i);
			m_config.SetValue(id.c_str(), m_planeSub[i].N.x);
			id.Format("planesub%d-y", i);
			m_config.SetValue(id.c_str(), m_planeSub[i].N.y);
			id.Format("planesub%d-z", i);
			m_config.SetValue(id.c_str(), m_planeSub[i].N.z);
			id.Format("planesub%d-d", i);
			m_config.SetValue(id.c_str(), m_planeSub[i].D);
		}
	}

	return m_config.Write(m_configFileName.c_str());
}


void cRoot::GeneratePlane(common::Vector3 pos[3])
{
	if (m_groundPlane.N == Vector3(0,1,0))
	{
		Plane plane(pos[0], pos[1], pos[2]);
		m_groundPlane = plane;
	}
	else
	{
		// change original vertex space
		Matrix44 tm;
		{
			Quaternion q;
			q.SetRotationArc(Vector3(0, 1, 0), m_groundPlane.N);
			tm *= q.GetMatrix();
			Matrix44 T;
			T.SetPosition(m_groundPlane.N * -m_groundPlane.D);
			tm *= T;
		}

		common::Vector3 p[3];
		p[0] = pos[0] * tm;
		p[1] = pos[1] * tm;
		p[2] = pos[2] * tm;
		Plane plane(p[0], p[1], p[2]);


		// new plane
		//{
		//	Quaternion q;
		//	q.SetRotationArc(plane.N, Vector3(0, 1, 0));
		//	tm *= q.GetMatrix();
		//	//Matrix44 T;
		//	//T.SetPosition(Vector3(0, plane.D, 0));
		//	//tm *= T;
		//}
		//Vector3 N = Vector3(0, 1, 0) * tm.Inverse();
		//Plane p(N.Normal(), 0);
		m_groundPlane = plane;
	}
}


// 화면에 보이는 센서 중, 첫번째 센서를 선택한다.
cSensor* cRoot::GetFirstVisibleSensor()
{
	for (auto sensor : m_baslerCam.m_sensors)
		if (sensor->m_isEnable && sensor->m_buffer.m_isLoaded && sensor->m_isShow)
			return sensor;
	return NULL;
}


void cRoot::Clear()
{
	m_measure.Clear();

	//m_kinect.Clear();
	m_baslerCam.Clear();
	
	m_config.SetValue("kinect_connect", m_isConnectKinect);
	m_config.SetValue("basler_connect", m_isTryConnectBasler);
	m_config.SetValue("inputfilepath", m_inputFilePath.c_str());
	m_config.Write(m_configFileName.c_str());

	m_pcdWriteThread.Clear();
}
