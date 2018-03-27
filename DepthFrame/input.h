//
// 2018-03-19, jjuiddong
// Input
//
#pragma once


class cInputView : public framework::cDockWindow
{
public:
	cInputView(const string &name);
	virtual ~cInputView();

	bool Init(graphic::cRenderer &renderer);
	virtual void OnRender(const float deltaSeconds) override;
	virtual void OnEventProc(const sf::Event &evt);
	void DelayMeasure();


protected:
	void UpdateFileList();
	bool OpenFile(const common::StrPath &ansifileName);
	void CalcDelayMeasure();
	void StoreMinimumDifferenceSensorBuffer();



public:
	struct eState {
		enum Enum { 
			DELAY_MEASURE // 1�ʰ� ������ �޾Ƽ�, ���� ���� ������ �ִ� ������ ����Ѵ�.
			, NORMAL 
		};
	};
	eState::Enum m_state;
	common::StrPath m_selectPath; // UTF-16
	vector<common::StrPath> m_files; // UTF-8 encoding
	vector<common::StrPath> m_files2; // UTF-8 encoding (only filename)
	bool m_isCaptureContinuos;
	float m_captureTime;

	// Animation
	bool m_isFileAnimation;
	int m_aniIndex;
	float m_aniTime;
	int m_selFileIdx;

	// Delay Measure
	float m_measureTime;
	float m_minDifference;
	vector<USHORT> m_depthBuff; // intensity
	vector<USHORT> m_depthBuff2; // confidence
	vector<common::Vector3> m_vertices;
};
