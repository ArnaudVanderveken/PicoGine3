#include "pch.h"
#include "GUI.h"

GUI::GUI() :
	m_IsInitialized{ false }
{
}

GUI::~GUI()
{

}

void GUI::Initialize()
{


	m_IsInitialized = true;
}

bool GUI::IsInitialized() const
{
	return m_IsInitialized;
}

void GUI::BeginFrame()
{
}

void GUI::EndFrame()
{
}
