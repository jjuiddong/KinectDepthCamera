//
// 2018-04-06, jjuiddong
// Contour
//
#pragma once


class cContour
{
public:
	cContour();
	virtual ~cContour();

	bool Init(const vector<cv::Point> &contour);
	void Draw(cv::Mat &dst, const cv::Scalar &color = cv::Scalar(0, 0, 0), const int thickness = 1) const;
	bool IsContain(const cContour &contour);
	u_int Area();
	inline u_int Size();
	inline cv::Point operator[] (int i);


public:
	vector<cv::Point> m_data;
};



inline u_int cContour::Size() {return m_data.size();}
inline cv::Point cContour::operator[] (int i) {return m_data[i];}
