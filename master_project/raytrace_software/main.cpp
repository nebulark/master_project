// raytrace_software.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"

#include "Deleters.hpp"


#include "Sphere.hpp"
#include "Camera.hpp"
#include "Intersection.hpp"
#include "Color.hpp"

namespace
{
	Sphere sphere;
	Camera camera;
	glm::vec3 camera_pos;
}

Color normal_as_color(glm::vec3 normal)
{
	// map from -1;+1 to 0;+1
	const glm::vec3 color_vec = (normal + 1.f ) / 2.f;
	return Color(color_vec * 255.f);
}

void Init()
{
	sphere = Sphere{ glm::vec3(0.f,0.f, 10.f), 5.f };
	camera = Camera::create_with_forward_dir(glm::vec3(0.f, 0.f, 1.f));
	camera_pos = glm::vec3(0.f);
}


void Draw(SDL_Renderer* renderer)
{
	assert(renderer);

	SDL_Rect viewport_rect;


	/* Get the Size of drawing surface */
	SDL_RenderGetViewport(renderer, &viewport_rect);

	const glm::ivec2 viewport_i(viewport_rect.w, viewport_rect.h);
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

			std::optional<float> intersection_distance = Intersection::ray_sphere(Ray{ camera_pos, ray_dir }, sphere);
			Color color;
			if (intersection_distance)
			{
				const glm::vec3 intersection_point = camera_pos + ray_dir * (*intersection_distance);
				const glm::vec3 intersection_normal = glm::normalize(intersection_point - sphere.positon);
				color = normal_as_color(intersection_normal);
			}
			else
			{
				color = Color(0, 0, 0);
			}

			SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 0xFF);
			SDL_RenderDrawPoint(renderer, x, y);
		}
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
				640,                               // width, in pixels
				480,                               // height, in pixels
				SDL_WINDOW_OPENGL                  // flags - see below
			) 
		};

		// Check that the window was successfully created
		if (window == nullptr) {
			// In the case that the window could not be made...
			printf("Could not create window: %s\n", SDL_GetError());
			return 1;
		}

		RendererPtr renderer{
			SDL_CreateSoftwareRenderer(SDL_GetWindowSurface(window.get()))
		};

		SDL_SetRenderDrawColor(renderer.get(), 0xFF, 0xFF, 0xFF, 0xFF);
		SDL_RenderClear(renderer.get());

		Init();
		while (true) 
		{
			SDL_Event event;
			while (SDL_PollEvent(&event)) 
			{
				switch (event.type)
				{
					case SDL_WINDOWEVENT:
					{
						if (event.window.event == SDL_WINDOWEVENT_CLOSE)
						{
							goto end_event_loop;
						}
					}
				}
			}
			/* do some other stuff here -- draw your app, etc. */
			Draw(renderer.get());
			SDL_UpdateWindowSurface(window.get());
			
		}
	end_event_loop:;
		

	}
	// Clean up
	SDL_Quit();
	return 0;

}


