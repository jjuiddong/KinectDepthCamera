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
	double CalcBasePlaneStandardDeviation(const size_t camIdx = 0);


public:
	bool m_isCalc;
};
