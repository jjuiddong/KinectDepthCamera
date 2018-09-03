//
// 2018-03-19, jjuiddong
// Input View
//
#pragma once


class cInputView : public framework::cDockWindow
{
public:
	struct sFileInfo {
		vector<__int64> ids; // convert only filename  to number
		vector<common::StrPath> fullFileNames; //  Full Filename (UTF-8 encoding)
		vector<common::StrPath> fileNames; // Only Filename (UTF-8 encoding)
	};

	cInputView(const string &name);
	virtual ~cInputView();

	bool Init(graphic::cRenderer &renderer);
	virtual void OnRender(const float deltaSeconds) override;
	virtual void OnEventProc(const sf::Event &evt);
	void DelayMeasure();
	void DelayMeasure10();
	void CancelDelayMeasure();


protected:
	void UpdateFileList();
	bool OpenFile(const common::StrPath &ansifileName, const size_t camIdx=0);
	void UpdateDelayMeasure(const float deltaSeconds);
	void CalcDelayMeasure();
	void CalcDelayMeasureRefine();
	void RenderFileList();
	__int64 ConvertFileNameToInt64(const common::StrPath &fileName);
	std::pair<int, int> GetOnlyTwoCameraAnimationIndex(const sFileInfo &finfo1, int aniIdx1
		, const sFileInfo &finfo2, int aniIdx2, const bool isSkip=false);

	
public:
	struct eState {
		enum Enum { 
			DELAY_MEASURE1 // 1000 번 계산해서 평균값을 저장한다.
			, DELAY_MEASURE2 // 10 번 계산해서 평균값을 저장한다.
			, NORMAL 
		};
	};
	eState::Enum m_state;
	bool m_isCaptureContinuos;
	float m_captureTime;

	// Animation
	bool m_isFileAnimation;
	int m_aniIndex1;
	int m_aniIndex2;
	int m_aniIndex3;
	float m_aniTime;
	int m_selFileIdx;
	int m_aniCameraCount; // + 1
	int m_explorerFolderIndex; //0, 1, 2 (camera 1,2,3), default:2
	bool m_isAutoSelectFileIndex;

	// Delay Measure (minimum difference error buffer)
	float m_measureTime;
	int m_measureCount;

	// Files
	enum { MAX_FILEPAGE = 100 };
	int m_filePages;
	int m_comboFileIdx;
	common::Str256 m_comboFileStr;
	common::StrPath m_selectPath; // UTF-16

	vector<sFileInfo> m_files;
};
