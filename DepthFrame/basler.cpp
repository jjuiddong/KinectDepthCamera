
#include "stdafx.h"
#include "basler.h"


using namespace graphic;
using namespace framework;

cBaslerView::cBaslerView(const string &name)
	: framework::cDockWindow(name)
{
}

cBaslerView::~cBaslerView()
{
}


bool cBaslerView::Init(graphic::cRenderer &renderer)
{

	return true;
}


void cBaslerView::OnRender(const float deltaSeconds)
{

}

