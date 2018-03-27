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


protected:
	void UpdateFileList();
	bool OpenFile(const common::StrPath &ansifileName);


public:
	common::StrPath m_selectPath; // UTF-16
	vector<common::StrPath> m_files; // UTF-8 encoding
	bool m_isCaptureContinuos;
	float m_captureTime;

	// Animation
	bool m_isFileAnimation;
	int m_aniIndex;
	float m_aniTime;
	int m_selFileIdx;
};
