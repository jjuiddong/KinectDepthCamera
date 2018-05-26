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
	bool m_isCalc;
	cCalibration::sResult m_result;
};
