#include "pch.hpp"
#include "InputManager.hpp"

namespace
{
	KeyCode SdlKeyCodeToMyKeyCode(SDL_Keycode KeyCode);
}


void InputManager::update_key_down(const SDL_KeyboardEvent& keyboardEvent)
{
	assert(SDL_EventType::SDL_KEYDOWN == keyboardEvent.type);

	Key & key = AccessKey(SdlKeyCodeToMyKeyCode(keyboardEvent.keysym.sym));
	key.SetIsPressed(true);
}

void InputManager::update_key_up(const SDL_KeyboardEvent & keyboardEvent)
{
	assert(SDL_EventType::SDL_KEYUP == keyboardEvent.type);

	Key & key = AccessKey(SdlKeyCodeToMyKeyCode(keyboardEvent.keysym.sym));
	key.SetIsPressed(false);
}

void InputManager::update_mouse(const SDL_MouseMotionEvent & mouseEvent)
{
	assert(SDL_EventType::SDL_MOUSEMOTION == mouseEvent.type);
	m_mouseMotion.x += mouseEvent.xrel;
	m_mouseMotion.y += mouseEvent.yrel;

	m_mousePosition = { mouseEvent.x, mouseEvent.y };
}

void InputManager::StartNewFrame()
{
	m_mouseMotion = { 0.f,0.f };
	for (auto& key : m_keys)
	{
		key.StartNewFrame();
	}
}

const Key& InputManager::GetKey(KeyCode keycode) const
{
	const Key& result = const_cast<InputManager*>(this)->AccessKey(keycode);
	assert(&result != &m_unusedKey);
	return result;
}

Key& InputManager::AccessKey(KeyCode keycode)
{
	if (keycode == KeyCode::ENUM_SIZE)
	{
		return m_unusedKey;
	}
	return m_keys[static_cast<int>(keycode)];
}

namespace
{
	KeyCode SdlKeyCodeToMyKeyCode(SDL_Keycode KeyCode)
	{
		switch (KeyCode)
		{
		case SDLK_w:
			return KeyCode::KEY_W;
		case SDLK_s:
			return KeyCode::KEY_S;
		case SDLK_d:
			return KeyCode::KEY_D;
		case SDLK_a:
			return KeyCode::KEY_A;
		case SDLK_c:
			return KeyCode::KEY_C;
		case SDLK_e:
			return KeyCode::KEY_E;
		case SDLK_f:
			return KeyCode::KEY_F;
		case SDLK_g:
			return KeyCode::KEY_G;
		case SDLK_h:
			return KeyCode::KEY_H;
		case SDLK_l:
			return KeyCode::KEY_L;
		case SDLK_k:
			return KeyCode::KEY_K;
		case SDLK_n:
			return KeyCode::KEY_N;
		case SDLK_m:
			return KeyCode::KEY_M;
		case SDLK_v:
			return KeyCode::KEY_V;
		case SDLK_b:
			return KeyCode::KEY_B;
		case SDLK_r:
			return KeyCode::KEY_R;
		case SDLK_u:
			return KeyCode::KEY_U;
		case SDLK_o:
			return KeyCode::KEY_O;
		case SDLK_q:
			return KeyCode::KEY_Q;
		case SDLK_p:
			return KeyCode::KEY_P;
		case SDLK_t:
			return KeyCode::KEY_T;
		case SDLK_x:
			return KeyCode::KEY_X;
		case SDLK_y:
			return KeyCode::KEY_Y;
		case SDLK_i:
			return KeyCode::KEY_I;
		case SDLK_1:
			return KeyCode::KEY_1;
		case SDLK_2:
			return KeyCode::KEY_2;
		case SDLK_3:
			return KeyCode::KEY_3;
		case SDLK_4:
			return KeyCode::KEY_4;
		case SDLK_5:
			return KeyCode::KEY_5;
		case SDLK_6:
			return KeyCode::KEY_6;
		case SDLK_7:
			return KeyCode::KEY_7;
		case SDLK_8:
			return KeyCode::KEY_8;
		case SDLK_9:
			return KeyCode::KEY_9;
		case SDLK_0:
			return KeyCode::KEY_0;
		case SDLK_SPACE:
			return KeyCode::KEY_SPACE;
		case SDLK_LSHIFT:
			return KeyCode::KEY_LEFT_SHIFT;
		case SDLK_COMMA:
			return KeyCode::KEY_COMMA;

		case SDLK_PLUS:
			return KeyCode::PLUS;
		case SDLK_MINUS:
			return KeyCode::MINUS;


		default:
			return KeyCode::ENUM_SIZE;
		}
	}
}