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
	bool OpenFile(const common::StrPath &ansifileName, const size_t camIdx=0);
	void UpdateDelayMeasure(const float deltaSeconds);
	void CalcDelayMeasure(const size_t camIdx = 0);
	void StoreMinimumDifferenceSensorBuffer(const size_t camIdx=0);
	void RenderFileList();
	__int64 ConvertFileNameToInt64(const common::StrPath &fileName);
	int GetTwoCameraAnimationIndex(int aniIdx1, int aniIdx2);
	std::pair<int, int> GetOnlyTwoCameraAnimationIndex(int aniIdx1, int aniIdx2);

	
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
	float m_triggerDelayTime;

	// Animation
	bool m_isFileAnimation;
	bool m_isReadTwoCamera;
	bool m_isOnlyTwoCameraFile;
	int m_aniIndex;
	int m_aniIndex2;
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
	vector<common::StrPath> m_files; // UTF-8 encoding (fulle filename)
	vector<common::StrPath> m_files2; // UTF-8 encoding (only filename)
	vector<common::StrPath> m_secondFiles; // UTF-8 encoding
	vector<__int64> m_fileIds;
	vector<__int64> m_secondFileIds;
	bool m_isReadCamera2;
};
