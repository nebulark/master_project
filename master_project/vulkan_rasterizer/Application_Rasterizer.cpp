#include "pch.hpp"
#include "Application_Rasterizer.hpp"


namespace
{
	

}

Application_Rasterizer::Application_Rasterizer()
{
	constexpr int width = static_cast<int>( 1920 / 1.5);
	constexpr int height = static_cast<int>(1080 / 1.5);

	m_sdlWindow = WindowPtr{
		SDL_CreateWindow(
		"An SDL2 window",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		width,
		height,
		SDL_WindowFlags::SDL_WINDOW_VULKAN | SDL_WindowFlags::SDL_WINDOW_RESIZABLE
		)
	};


	m_camera.SetPerspection_InverseZBuffer( 1.f, glm::radians(45.f), glm::vec2(width, height));

	m_camera.SetPosition(glm::vec3(0.f, 0.05f, 3.f) * 20.f);
	m_camera.LookDir(glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	m_graphcisBackend.Init(m_sdlWindow.get(), m_camera);
	m_oldCameraPos = m_camera.CalcPosition();
	m_lastTime = ClockType::now();
}

bool Application_Rasterizer::Update()
{
	ClockType::time_point currentTime = ClockType::now();
	const float DeltaSeconds = FloatSeconds(currentTime - m_lastTime).count();
	m_lastTime = currentTime;

	m_inputManager.StartNewFrame();
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		if (SDL_WINDOWEVENT == event.type && SDL_WINDOWEVENT_CLOSE == event.window.event)
		{
			m_graphcisBackend.WaitIdle();
			return false;
		}
		HandleEvent(event);
	}
	GameUpdate(DeltaSeconds);
	m_graphcisBackend.Render(m_camera, m_extraLines);
	SDL_UpdateWindowSurface(m_sdlWindow.get());
	return true;
}

void Application_Rasterizer::HandleEvent(SDL_Event event)
{
	switch (event.type)
	{
	case SDL_EventType::SDL_KEYDOWN:
		m_inputManager.update_key_down(event.key);
		break;
	case SDL_EventType::SDL_KEYUP:
		m_inputManager.update_key_up(event.key);
		break;
	case SDL_EventType::SDL_MOUSEMOTION:
		m_inputManager.update_mouse(event.motion);
	default:
		break;
	}
}

void Application_Rasterizer::GameUpdate(float DeltaSeconds)
{	
	constexpr float movementMultiplicator = 500.f;
	constexpr float pitchMultiplicator = 2.f;
	constexpr float yawMultiplicator = 2.f;
	float yawInput = 0;
	float pitchInput = 0;
	float rightInput = 0;
	float forwardInput = 0;
	float upInput = 0;

	if (m_inputManager.GetKey(KeyCode::KEY_W).IsPressed())
	{
		forwardInput += 1.f;
	}

	if (m_inputManager.GetKey(KeyCode::KEY_S).IsPressed())
	{
		forwardInput -= 1.f;
	}

	if (m_inputManager.GetKey(KeyCode::KEY_D).IsPressed())
	{
		rightInput += 1.f;
	}

	if (m_inputManager.GetKey(KeyCode::KEY_A).IsPressed())
	{
		rightInput -= 1.f;
	}

	if (m_inputManager.GetKey(KeyCode::KEY_Q).IsPressed())
	{
		upInput += 1.f;
	}

	if (m_inputManager.GetKey(KeyCode::KEY_E).IsPressed())
	{
		upInput -= 1.f;
	}


	if (!m_inputManager.GetKey(KeyCode::KEY_LEFT_SHIFT).IsPressed())
	{
		yawInput += m_inputManager.GetMouseMotion().x * DeltaSeconds * yawMultiplicator;
		pitchInput += m_inputManager.GetMouseMotion().y * DeltaSeconds * pitchMultiplicator;
		m_camera.UpdateFromMouse(yawInput, pitchInput);
	}

	forwardInput *= DeltaSeconds * movementMultiplicator;
	rightInput *= DeltaSeconds * movementMultiplicator;
	upInput *= DeltaSeconds * movementMultiplicator;
	
	// update location and teleport if neccessary
	{

		m_camera.UpdateLocation(forwardInput, rightInput, upInput);

		//const float raytraceBias = 0.1f;

		const glm::vec3 newCameraPos = m_camera.CalcPosition();
			

		gsl::span<const TriangleMesh> triangleMeshes = m_graphcisBackend.GetTriangleMeshes();


		const PortalManager& portalManager = m_graphcisBackend.GetPortalManager();
		const Ray cameraMoveRay = Ray::FromStartAndEndpoint(
			m_oldCameraPos, 
			newCameraPos  /* + m_camera.CalcForwardVector() * raytraceBias*/
		);

		m_oldCameraPos = newCameraPos;

		std::optional<PortalManager::RayTraceResult> maybeRtResult =
			portalManager.RayTrace(cameraMoveRay, triangleMeshes);



		if (maybeRtResult.has_value())
		{
			const PortalManager::RayTraceResult& rtResult = *maybeRtResult;

			const glm::mat4& teleportMat = portalManager.GetPortals()[rtResult.portalIndex].toOtherEndpoint[rtResult.endpoint];

			m_camera.m_coordinateSystem = teleportMat * m_camera.m_coordinateSystem;

			// update old post so it fits in the same coordinate system
			m_oldCameraPos = m_camera.CalcPosition();
			//	m_oldCameraPos += m_camera.CalcForwardVector() * raytraceBias;

		}
	}


	if (m_inputManager.GetKey(KeyCode::KEY_P).GetNumPressed() > 0)
	{
		std::puts("     --- Seperation -----        ");
	}
	if (m_inputManager.GetKey(KeyCode::KEY_1).GetNumPressed() > 0)
	{
		m_savedLocation = m_camera.CalcPosition();
	}


	if (m_inputManager.GetKey(KeyCode::KEY_2).GetNumPressed() > 0)
	{
		const glm::vec3 currentLocation = m_camera.CalcPosition();

		const PortalManager& portalmanger = m_graphcisBackend.GetPortalManager();

		m_extraLines.push_back(Line(glm::vec4(m_savedLocation, 1.f), glm::vec4(currentLocation, 1.f)));

		const Ray ray = Ray::FromStartAndEndpoint(m_savedLocation, currentLocation);

		std::optional<PortalManager::RayTraceResult> maybeResult =
			portalmanger.RayTrace(ray, m_graphcisBackend.GetTriangleMeshes());

		if (maybeResult.has_value())
		{
			const glm::vec3 hitPos = maybeResult->hitLocation;
			m_extraLines.push_back(Line(glm::vec4(hitPos.x, -100.f, hitPos.z, 1.f), glm::vec4(hitPos.x, +100.f, hitPos.z, 1.f)));
		}


	}

	// useful for testing raytracing
	if (m_inputManager.GetKey(KeyCode::KEY_R).GetNumPressed() > 0)
	{
		const PortalManager& portalmanger = m_graphcisBackend.GetPortalManager();

		const glm::vec4 missedA(21.4760914, 12.6429110, 23.1454220, 1.00000000);
		const glm::vec4 missedB(5.09701300, 11.9839067, 27.4304352, 1.00000000);

		const Ray ray = Ray::FromStartAndEndpoint(missedA, missedB);

		std::optional<PortalManager::RayTraceResult> maybeResult =
			portalmanger.RayTrace(ray, m_graphcisBackend.GetTriangleMeshes());

		if (!maybeResult.has_value())
		{
			m_extraLines.push_back(Line(missedA, missedB));
		}

		const bool b = maybeResult.has_value();
	}


	if (m_inputManager.GetKey(KeyCode::KEY_F).GetNumPressed() > 0)
	{
		m_showFrameMilliseconds = !m_showFrameMilliseconds;
	}

	if (m_showFrameMilliseconds)
	{
		std::printf("delta Milliseconds: %f \n", DeltaSeconds * 1000.f);
	}
}

