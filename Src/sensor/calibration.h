//
// 2018-05-26, jjuiddong
// ground plane calibration
//
#pragma once


class cCalibration
{
public:
	struct sResult {
		double curSD;
		double minSD;
		common::Vector3 pos[3];
		common::Plane plane;
	};

	struct sRegion {
		int sensorId;
		common::Vector3 center;
		common::Vector2 size;
		vector<common::StrPath> files;
	};


	cCalibration();
	virtual ~cCalibration();

	sResult CalibrationBasePlane(const common::Vector3 &center0, const common::Vector2 &size
		, const cSensor *sensor);
	bool CalibrationBasePlane(const char *scriptFileName, OUT sResult &out);
	bool CalibrationBasePlane(const vector<sRegion> &regions, OUT sResult &out);
	float CalcHeightDistribute(const common::Vector3 &center0, const common::Vector2 &size
		, const cSensor *sensor);

	void Clear();


protected:
	double CalcHeightStandardDeviation(const vector<common::Vector3> &vertices
		, const common::Vector3 &center, const common::sRectf &rect, const common::Matrix44 &tm);
	double CalcHeightStandardDeviation(const vector<common::Vector3> &vertices, const common::Matrix44 &tm);


public:
	enum {
		MAX_GROUNDPLANE_CALIBRATION = 1000
	};

	sResult m_result;
	common::Plane m_avrPlane;
	vector<common::Plane> m_planes;
};
