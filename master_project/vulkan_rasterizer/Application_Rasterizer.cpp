#include "pch.hpp"
#include "Application_Rasterizer.hpp"


namespace
{
	

}

Application_Rasterizer::Application_Rasterizer()
{
	constexpr int width = 1920 / 2;
	constexpr int height = 1080 / 2;

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

	m_graphcisBackend.Init(m_sdlWindow.get());

	m_camera.SetPerspection( 0.1f, 1000.0f, glm::radians(45.f), glm::vec2(width, height));
	m_camera.m_transform.translation =  glm::vec3(0.f, 0.05f, 3.f) * 20.f;
	m_camera.LookDir( glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
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
	constexpr float movementMultiplicator = 10.f;
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
	
	if (forwardInput != 0.f || rightInput != 0.f || upInput != 0.f)
	{

		const glm::vec3 oldCameraPos = m_camera.m_transform.translation;
		m_camera.UpdateLocation(forwardInput, rightInput, upInput);

		if (true)
		{
			gsl::span<const TriangleMesh> triangleMeshes = m_graphcisBackend.GetTriangleMeshes();

			const PortalManager& portalManager = m_graphcisBackend.GetPortalManager();
			std::optional<PortalManager::RayTraceResult> maybeRtResult =
				portalManager.RayTrace(Ray::FromStartAndEndpoint(oldCameraPos, m_camera.m_transform.translation), triangleMeshes);
			
			if (maybeRtResult.has_value())
			{
				const PortalManager::RayTraceResult& rtResult = *maybeRtResult;

				const glm::vec4 cameraTranslation(m_camera.m_transform.translation, 1.f);

				const glm::mat4& teleportMat = portalManager.GetPortals()[rtResult.portalIndex].toOtherEndpoint[rtResult.endpoint];

				m_camera.m_transform.translation = glm::vec3((teleportMat) * cameraTranslation);
				//m_camera.m_transform.rotation *= glm::quat_cast(*maybeTeleportMat) * m_camera.m_transform.rotation;

			}
		}
	}

#if 0
	if (false && m_inputManager.GetKey(KeyCode::KEY_SPACE).GetNumPressed() > 0)
	{
		const Ray ray = Ray::FromOriginAndDirection(m_camera.m_transform.translation, m_camera.CalcForwardVector());
		gsl::span<const TriangleMesh> triangleMeshes = m_graphcisBackend.GetTriangleMeshes();

		const glm::vec4 colors[] =
		{
			glm::vec4(1.f,0.f,0.f,1.f),
			glm::vec4(0.f,1.f,0.f,1.f)
		};

		float distanceTraveled = 0.f;
		std::vector<Line> lines;

		//while (distanceTraveled < ray.distance)
		{
			float closestDistance = std::numeric_limits<float>::max();
			glm::vec3 worldspaceBegin = ray.origin + ray.direction * distanceTraveled;
			glm::vec3 worldspaceEnd = ray.CalcEndPoint();
			float currentTraveldDistance = std::numeric_limits<float>::max();

			//std::printf("ws begin (%f, %f, %f)\n", worldspaceBegin.x, worldspaceBegin.y, worldspaceBegin.z);

			//m_graphcisBackend.GetPortalManager().FindHitPortalTeleportMatrix(ray, triangleMeshes);
			for (const Portal& portal : m_graphcisBackend.GetPortalManager().GetPortals())
			{

				glm::mat4 modelMats[2] = { portal.a_transform, portal.b_transform };
				const glm::mat4 inverseModelMats[2] = { glm::inverse(modelMats[0]), glm::inverse(modelMats[1]) };
				for (int i = 0; i < std::size(inverseModelMats); ++i)
				{
					const glm::vec3 modelRayOrigin = glm::vec3(inverseModelMats[i] * glm::vec4(worldspaceBegin, 1.f));
					const glm::vec3 modelRayEnd = glm::vec3(inverseModelMats[i] * glm::vec4(ray.CalcEndPoint(), 1.f));

					const Ray modelRay = Ray::FromStartAndEndpoint(modelRayOrigin, modelRayEnd);

					std::optional<float> rt_res = triangleMeshes[portal.meshIndex].RayTrace(modelRay);

					if (rt_res.has_value() && *rt_res < closestDistance)
					{
						closestDistance = *rt_res;
						worldspaceEnd = modelMats[i] * glm::vec4(modelRayOrigin + modelRay.direction * (*rt_res), 1.f);
						currentTraveldDistance = glm::distance(worldspaceBegin, worldspaceEnd);
					}

				}
			}


			distanceTraveled += currentTraveldDistance;
			Line line;
			line.pointA = glm::vec4(worldspaceBegin, 1.f);
			line.pointB = glm::vec4(worldspaceEnd, 1.f);

			line.colorA = line.colorB = colors[lines.size() % std::size(colors)];
			lines.push_back(line);
		}
		m_extraLines.insert(m_extraLines.end(), lines.begin(), lines.end());
	}
#endif


	std::printf("delta Milliseconds: %f \n", DeltaSeconds * 1000.f);


}

