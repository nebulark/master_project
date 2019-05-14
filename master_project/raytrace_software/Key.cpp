#include "pch.h"
#include "Key.hpp"

void Key::SetIsPressed(bool isPressed)
{
	if (m_isPressed == isPressed)
	{
		return;
	}
	if (isPressed)
	{
		assert(m_isPressed == false);
		m_isPressed = true;

		assert(std::numeric_limits<decltype(m_numPressed)>::max() != m_numPressed);
		++m_numPressed;
	}
	else
	{
		assert(m_isPressed == true);
		m_isPressed = false;

		assert(std::numeric_limits<decltype(m_numReleased)>::max() != m_numReleased);
		++m_numReleased;
	}
}

void Key::StartNewFrame()
{
	m_numPressed = 0;
	m_numReleased = 0;
}