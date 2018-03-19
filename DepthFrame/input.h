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


protected:
	void UpdateFileList();
	common::StrPath m_selectPath; // UTF-16
	vector<common::StrPath> m_files; // UTF-8 encoding
};
