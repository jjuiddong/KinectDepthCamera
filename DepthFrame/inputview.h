//
// 2018-03-19, jjuiddong
// Input View
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
	void UpdateDelayMeasure(const float deltaSeconds);
	void CalcDelayMeasure();
	void StoreMinimumDifferenceSensorBuffer();
	void RenderFileList();

	
public:
	struct eState {
		enum Enum { 
			DELAY_MEASURE // 1초간 정보를 받아서, 가장 적은 오차가 있는 정보로 계산한다.
			, NORMAL 
		};
	};
	eState::Enum m_state;
	bool m_isCaptureContinuos;
	float m_captureTime;

	// Animation
	bool m_isFileAnimation;
	int m_aniIndex;
	float m_aniTime;
	int m_selFileIdx;

	// Delay Measure (minimum difference error buffer)
	float m_measureTime;
	float m_minDifference;
	vector<USHORT> m_depthBuff; // intensity
	vector<USHORT> m_depthBuff2; // confidence
	vector<common::Vector3> m_vertices;

	// Files
	enum {
		MAX_FILEPAGE = 100
	};
	int m_filePages;
	int m_comboFileIdx;
	common::Str256 m_comboFileStr;
	common::StrPath m_selectPath; // UTF-16
	vector<common::StrPath> m_files; // UTF-8 encoding
	vector<common::StrPath> m_files2; // UTF-8 encoding (only filename)
};
