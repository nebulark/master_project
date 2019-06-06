#pragma once

#include "Key.hpp"
#include "vec2.hpp"

#include <array>

struct SDL_KeyboardEvent;
struct SDL_MouseMotionEvent;

enum class KeyCode
{
	KEY_W,
	KEY_S,
	KEY_D,
	KEY_A,
	KEY_C,
	KEY_L,
	KEY_K,
	KEY_E,
	KEY_F,
	KEY_G,
	KEY_H,
	KEY_N,
	KEY_M,
	KEY_V,
	KEY_B,
	KEY_R,
	KEY_U,
	KEY_I,
	KEY_O,
	KEY_P,
	KEY_Q,
	KEY_T,
	KEY_X,
	KEY_Y,
	KEY_LEFT_SHIFT,
	KEY_1,
	KEY_2,
	KEY_3,
	KEY_4,
	KEY_5,
	KEY_6,
	KEY_7,
	KEY_8,
	KEY_9,
	KEY_0,
	KEY_SPACE,
	PLUS,
	MINUS,
	KEY_COMMA,
	ENUM_SIZE
};


class InputManager
{
public:
	void update_key_down(const SDL_KeyboardEvent& keyboardEvent);
	void update_key_up(const SDL_KeyboardEvent& keyboardEvent);
	void update_mouse(const SDL_MouseMotionEvent& mouseEvent);
	void StartNewFrame();

	const Key& GetKey(KeyCode keycode) const;
	const glm::vec2& GetMouseMotion() const { return m_mouseMotion; }
	const glm::vec2& GetMousePosition() const { return m_mousePosition; }
private:
	Key& AccessKey(KeyCode keycode);

	std::array<Key, static_cast<int>(KeyCode::ENUM_SIZE)> m_keys;
	glm::vec2 m_mouseMotion;
	glm::vec2 m_mousePosition;

	Key m_unusedKey;
};