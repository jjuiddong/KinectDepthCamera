//
// 2018-05-26, jjuiddong
// Calibration View
//
#pragma once


class cCalibrationView : public framework::cDockWindow
{
public:
	cCalibrationView(const string &name);
	virtual ~cCalibrationView();

	virtual void OnRender(const float deltaSeconds) override;


protected:
	void CalibrationGroundPlane();
	void SingleSensorSubPlaneCalibration();
	void MultiSensorGroundCalibration();
	void HeightDistrubution();

	double CalcBasePlaneStandardDeviation(const size_t camIdx = 0);
	bool CalibrationSubPlane(const int sensorIdx, const common::Vector3 &regionCenter, const common::Vector2 &regionSize);

	common::StrPath OpenFileDialog();


public:
	bool m_isCalc;
};
