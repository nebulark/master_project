#pragma once
#pragma once
#include <stdint.h>

class Key
{
public:
	bool IsPressed() const { return m_isPressed; }
	int8_t GetNumPressed() const { return m_numPressed; }
	int8_t GetNumReleased() const { return m_numReleased; }

	void SetIsPressed(bool isPressed);
	void StartNewFrame();

private:
	bool m_isPressed;
	int8_t m_numPressed;
	int8_t m_numReleased;
};