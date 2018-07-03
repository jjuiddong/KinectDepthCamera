//
// 2018-03-13, jjuiddong
// Box View
//
#pragma once


class cBoxView : public framework::cDockWindow
{
public:
	cBoxView(const string &name);
	virtual ~cBoxView();

	bool Init(graphic::cRenderer &renderer);
	virtual void OnUpdate(const float deltaSeconds) override;
	virtual void OnRender(const float deltaSeconds) override;
	virtual void OnPreRender(const float deltaSeconds) override;
	virtual void OnResizeEnd(const framework::eDockResize::Enum type, const common::sRectf &rect) override;
	virtual void OnEventProc(const sf::Event &evt) override;
	virtual void OnResetDevice() override;


protected:
	void RenderBoxVolume3D(graphic::cRenderer &renderer);

	void UpdateLookAt();
	void OnWheelMove(const float delta, const POINT mousePt);
	void OnMouseMove(const POINT mousePt);
	void OnMouseDown(const sf::Mouse::Button &button, const POINT mousePt);
	void OnMouseUp(const sf::Mouse::Button &button, const POINT mousePt);


public:
	bool m_showGround;
	bool m_showPointCloud;

	graphic::cGridLine m_ground;
	graphic::cLine m_boxLine;
	graphic::cRenderTarget m_renderTarget;

	// MouseMove Variable
	POINT m_viewPos;
	common::sRectf m_viewRect; // detect mouse event area
	POINT m_mousePos; // window 2d mouse pos
	common::Vector3 m_mousePickPos; // mouse cursor pos in ground picking
	bool m_mouseDown[3]; // Left, Right, Middle
	float m_rotateLen;
	common::Plane m_groundPlane1;
	common::Plane m_groundPlane2;
};

