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
	bool IsContain(const cContour &contour) const;
	u_int Area() const;
	cv::Point Center() const;
	inline u_int Size() const;
	inline cv::Point operator[] (int i) const;


public:
	double m_minCos; // 가장 작은 각
	double m_maxCos; // 가장 큰 각
	vector<cv::Point> m_data;
};



inline u_int cContour::Size() const {return m_data.size();}
inline cv::Point cContour::operator[] (int i) const {return m_data[i];}
