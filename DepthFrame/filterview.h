//
// 2018-03-24, jjuiddong
// Filter View
//
#pragma once


class cFilterView : public framework::cDockWindow
{
public:
	cFilterView(const string &name);
	virtual ~cFilterView();

	bool Init(graphic::cRenderer &renderer);
	virtual void OnRender(const float deltaSeconds) override;
	void Process();
	void CalcBoxVolumeAverage();
	void ClearBoxVolumeAverage();


public:
	void ProcessDepth();
	void UpdateTexture();
	bool FindBox(cv::Mat &img, const u_int vtxCnt, OUT vector<cContour> &out);
	void RemoveDuplicateContour(vector<sContourInfo> &contours);
	cRoot::sBoxInfo CalcBoxInfo(const sContourInfo &info);


public:
	cv::Mat m_binImg;
	cv::Mat m_dstImg;
	graphic::cTexture m_depthTexture;
	vector<sContourInfo> m_contours; // ���� �νĵ� �ڽ� ����
	vector<sContourInfo> m_removeRects;

	// Box Volume Average
	struct sAvrContour {
		bool check;
		int count;
		sContourInfo box;
		cRoot::sBoxInfo result;
		common::Vector2 avrVertices[13]; // ��� ���ؽ� ��ġ, �ִ� 8�� ������
		common::Vector3 vertices3d[13*2]; // ������ ��� ���ؽ� 3D ��ġ
	};
	vector<sAvrContour> m_avrContours; // ������� ���� �ڽ� ����
	int m_calcAverageCount;
	common::CriticalSection m_cs;
};
