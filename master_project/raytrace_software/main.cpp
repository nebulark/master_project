// raytrace_software.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"

#include "Deleters.hpp"


#include "Sphere.hpp"
#include "Camera.hpp"
#include "Intersection.hpp"
#include "Color.hpp"
#include "InputManager.hpp"

struct RGB_888_Pixel
{
	uint8_t r;
	uint8_t g;
	uint8_t b;

	uint8_t padding;
};

namespace
{
	Sphere spheres[2];
	Camera camera;
	InputManager inputManager;
	glm::vec3 camera_pos;
}

using Clock = std::chrono::steady_clock;
using FloatSeconds = std::chrono::duration<float>;

Color normal_as_color(glm::vec3 normal)
{
	// map from -1;+1 to 0;+1
	const glm::vec3 color_vec = (normal + 1.f ) / 2.f;
	return Color(color_vec * 255.f);
}

void Init()
{
	for (int i = 0; i < std::size(spheres); ++i)
	{
		spheres[i] = Sphere{ glm::vec3(2*i - 5.f,0.f, 10.f), 5.f };
	}
	
	camera = Camera::create_with_forward_dir(glm::vec3(0.f, 0.f, 1.f));
	camera_pos = glm::vec3(0.f);
}


void Draw(SDL_Surface* surface)
{
	assert(SDL_PIXELFORMAT_RGB888 == surface->format->format);
	assert(sizeof(RGB_888_Pixel) == surface->format->BytesPerPixel);
	assert(nullptr == surface->format->palette);


	RGB_888_Pixel* Pixels = reinterpret_cast<RGB_888_Pixel*>(surface->pixels);


	

	const glm::ivec2 viewport_i(surface->w, surface->h);
	const glm::vec2 viewport(viewport_i);
	const glm::vec2 viewport_inverse = 1.f / viewport;
	const float aspect_ratio_scaling = viewport.x / viewport.y;

	constexpr float field_of_view_deg = 90.f;
	constexpr float field_of_view_rad =  glm::radians(field_of_view_deg);
	const float field_of_view_scaling = std::tan(field_of_view_rad / 2.0f);

	

	for (int y = 0; y < viewport_i.y; ++y)
	{
		for (int x = 0; x < viewport_i.x; ++x)
		{
			glm::vec2 norm{
				viewport_inverse.x * x,
				viewport_inverse.y * y
			};

			glm::vec2 normalised_screen_pixel = (norm * 2.f) - 1.f;
			normalised_screen_pixel.x *= aspect_ratio_scaling;

			const glm::vec3 ray_dir = camera.calc_ray_dir(normalised_screen_pixel, 1.f);

			int sphereIndex = -1;
			std::optional<float> intersection_distance = std::nullopt;
			for (int i = 0; i < std::size(spheres); ++i)
			{
				std::optional<float> new_intersection_distance = Intersection::ray_sphere(Ray{ camera_pos, ray_dir }, spheres[i]);
				if (new_intersection_distance.has_value() && (!intersection_distance.has_value() || *new_intersection_distance < *intersection_distance))
				{
					intersection_distance = new_intersection_distance;
					sphereIndex = i;
				}
			}
			Color color;
			if (intersection_distance)
			{
				const glm::vec3 intersection_point = camera_pos + ray_dir * (*intersection_distance);
				const glm::vec3 intersection_normal = glm::normalize(intersection_point - spheres[sphereIndex].position);
				color = normal_as_color(intersection_normal);
			}
			else
			{
				color = Color(0, 0, 0);
			}

			RGB_888_Pixel& pixel = Pixels[x + y * viewport_i.x];
			pixel.r = color.r;
			pixel.g = color.g;
			pixel.b = color.b;

		}
	}
}

void handle_event(const SDL_Event& event)
{
	switch (event.type)
	{
	case SDL_EventType::SDL_KEYDOWN:
		inputManager.update_key_down(event.key);
		break;
	case SDL_EventType::SDL_KEYUP:
		inputManager.update_key_up(event.key);
		break;
	case SDL_EventType::SDL_MOUSEMOTION:
		inputManager.update_mouse(event.motion);
		break;
	default:
		break;
	}
}

int main(int argc, char* argv[])
{
	SDL_Init(SDL_INIT_EVERYTHING);
	{

		WindowPtr window{
				SDL_CreateWindow(
				"An SDL2 window",                  // window title
				SDL_WINDOWPOS_UNDEFINED,           // initial x position
				SDL_WINDOWPOS_UNDEFINED,           // initial y position
				1920,                               // width, in pixels
				1080,                               // height, in pixels
				SDL_WINDOW_OPENGL                 // flags - see below
			) 
		};

		// Check that the window was successfully created
		if (window == nullptr) {
			// In the case that the window could not be made...
			printf("Could not create window: %s\n", SDL_GetError());
			return 1;
		}

		Init();

		SDL_Surface* surface = SDL_GetWindowSurface(window.get());


		auto previous = Clock::now();
		while (true) 
		{
			auto current = Clock::now();
			FloatSeconds delta = std::chrono::duration_cast<FloatSeconds>(current - previous);
			previous = current;

			SDL_Event event;
			while (SDL_PollEvent(&event)) 
			{
				if (SDL_WINDOWEVENT == event.type && SDL_WINDOWEVENT_CLOSE == event.window.event)
				{
					goto end_event_loop;
				}
				handle_event(event);				
			}
			/* do some other stuff here -- draw your app, etc. */

			if (inputManager.GetKey(KeyCode::KEY_D).IsPressed())
			{
				camera_pos.x += delta.count() * 5.f;
			}
			if (inputManager.GetKey(KeyCode::KEY_A).IsPressed())
			{
				camera_pos.x -= delta.count() * 5.f;
			}

			SDL_LockSurface(surface);

			Draw(surface);

			SDL_UnlockSurface(surface);
			SDL_UpdateWindowSurface(window.get());
			
			printf("FPS %f | Milliseconds: %f\n", 1.f / delta.count(), delta.count() * 1'000.f);
		}
	end_event_loop:;
		

	}
	// Clean up
	SDL_Quit();
	return 0;

}


