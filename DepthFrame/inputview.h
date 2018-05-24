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


protected:
	void UpdateFileList();
	bool OpenFile(const common::StrPath &ansifileName, const size_t camIdx=0);
	void UpdateDelayMeasure(const float deltaSeconds);
	void CalcDelayMeasure(const size_t camIdx = 0);
	void RenderFileList();
	__int64 ConvertFileNameToInt64(const common::StrPath &fileName);
	//int GetTwoCameraAnimationIndex(int aniIdx1, int aniIdx2);
	std::pair<int, int> GetOnlyTwoCameraAnimationIndex(const sFileInfo &finfo1, int aniIdx1
		, const sFileInfo &finfo2, int aniIdx2, const bool isSkip=false);

	
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
	//bool m_isReadTwoCamera;
	//bool m_isOnlyTwoCameraFile;
	int m_aniIndex1;
	int m_aniIndex2;
	int m_aniIndex3;
	float m_aniTime;
	int m_selFileIdx;
	int m_aniCameraCount; // + 1

	// Delay Measure (minimum difference error buffer)
	float m_measureTime;

	// Files
	enum { MAX_FILEPAGE = 100 };
	int m_filePages;
	int m_comboFileIdx;
	common::Str256 m_comboFileStr;
	common::StrPath m_selectPath; // UTF-16

	vector<sFileInfo> m_files;

	//vector<common::StrPath> m_files; //  Full Filename (UTF-8 encoding)
	//vector<common::StrPath> m_files2; // Only Filename (UTF-8 encoding)
	//vector<common::StrPath> m_secondFiles; // Full Filename subfiles 1/... (UTF-8 encoding) 
	//vector<common::StrPath> m_thirdFiles; // Full Filename (subfiles 2/... (UTF-8 encoding) 

	//vector<__int64> m_fileIds;
	//vector<__int64> m_secondFileIds;
	//bool m_isReadCamera2;
};
