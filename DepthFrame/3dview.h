//
// 2018-03-13, jjuiddong
// 3D View
//
#pragma once


class c3DView : public framework::cDockWindow
{
public:
	c3DView(const string &name);
	virtual ~c3DView();

	bool Init(graphic::cRenderer &renderer);
	virtual void OnUpdate(const float deltaSeconds) override;
	virtual void OnRender(const float deltaSeconds) override;
	virtual void OnPreRender(const float deltaSeconds) override;
	virtual void OnResizeEnd(const framework::eDockResize::Enum type, const common::sRectf &rect) override;
	virtual void OnEventProc(const sf::Event &evt) override;
	virtual void OnResetDevice() override;


protected:
	void UpdateLookAt();
	void OnWheelMove(const float delta, const POINT mousePt);
	void OnMouseMove(const POINT mousePt);
	void OnMouseDown(const sf::Mouse::Button &button, const POINT mousePt);
	void OnMouseUp(const sf::Mouse::Button &button, const POINT mousePt);


public:
	common::Vector3 m_offset;
	graphic::cGridLine m_ground;
	graphic::cRenderTarget m_renderTarget;
	bool m_showGround;
	bool m_showWireframe;
	bool m_showSensorPlane;
	bool m_showPointCloud;
	bool m_showBoxAreaPointCloud;

	struct eState {
		enum Enum {
			NORMAL, PLANE, PICKPOS, VCENTER
		};
	};

	eState::Enum m_state;
	//bool m_isAutoProcess;
	bool m_isGenPlane;
	bool m_isGenVolumeCenter;
	int m_genPlane;
	common::Vector3 m_planePos[3];

	graphic::cDbgSphere m_sphere;
	graphic::cGridLine m_planeGrid;
	graphic::cDbgLine m_volumeCenterLine;
	common::Vector3 m_pickPos;

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

