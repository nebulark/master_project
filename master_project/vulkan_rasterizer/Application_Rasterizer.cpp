#include "pch.hpp"
#include "Application_Rasterizer.hpp"
#include <iostream>


namespace
{
	std::string printTestCase(gsl::span<const int> testcase)
	{
		if (testcase.size() == 0)
		{
			return std::string("Testcase - No recursions");
		}

		std::string result("Testcase ");
		result += std::to_string(testcase[0]);
		for (int i = 1; i < testcase.size(); ++i)
		{
			result += std::string("-") + std::to_string(testcase[i]);
		}

		return result;
	}

	double calcMedian(gsl::span<double> range)
	{
		const auto size = range.size();
		auto n = size / 2;
		int remainder = size % 2;

		std::nth_element(range.begin(), range.begin() + n, range.end());
		auto med = range[n];
		if (remainder == 0) {
			auto max_it = std::max_element(range.begin(), range.begin() + n);
			med = (*max_it + med) / 2.0;
		}
		return med;
	}

}

Application_Rasterizer::Application_Rasterizer()
{
	constexpr int width = static_cast<int>(1920);// / 1.5);
	constexpr int height = static_cast<int>(1080);// / 1.5);

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


	testCases = {
		std::vector<int>{},
		std::vector<int>{8,6,4,2},
		std::vector<int>{8,6,4},
		std::vector<int>{8,6},
		std::vector<int>{8},
		std::vector<int>{12,12,12,12},
		std::vector<int>{12,12,12,},
		std::vector<int>{12,12},
		std::vector<int>{12},
		std::vector<int>{4,4,4,4},
		std::vector<int>{4,4,4,},
		std::vector<int>{4,4},
		std::vector<int>{4},
		std::vector<int>{0,0,0,0,0},
		std::vector<int>{0,0,0,0},
		std::vector<int>{0,0,0},
		std::vector<int>{0,0},
		std::vector<int>{0},
	};
	m_graphcisBackend.SetMaxVisiblePortalsForRecursion(testCases[1]);
}

bool Application_Rasterizer::Update()
{
	ClockType::time_point currentTime = ClockType::now();
	const float DeltaSeconds = std::chrono::duration<float>(currentTime - m_lastTime).count();
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

	ClockType::time_point beforeRender = ClockType::now();
	m_graphcisBackend.Render(m_camera, m_drawOptions);
	ClockType::time_point afterRender = ClockType::now();

	m_frameTimeBuckets[m_currentFrameBucketIndex] = DoubleSeconds(afterRender - beforeRender).count();
	m_currentFrameBucketIndex = (m_currentFrameBucketIndex + 1) % frameTimeBuckedSize;



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

void Application_Rasterizer::SetTestCase(int i)
{
	if (i >= 0)
	{
		
		m_graphcisBackend.SetMaxVisiblePortalsForRecursion(testCases[i]);
	}
	currentCase = i;
}

void Application_Rasterizer::GameUpdate(float Seconds)
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

	if (m_inputManager.GetKey(KeyCode::KEY_K).GetNumPressed() > 0)
	{
		m_shouldLockrotation = !m_shouldLockrotation;
	}

	if (!m_inputManager.GetKey(KeyCode::KEY_LEFT_SHIFT).IsPressed() ^ m_shouldLockrotation)
	{
		yawInput += m_inputManager.GetMouseMotion().x * Seconds * yawMultiplicator;
		pitchInput += m_inputManager.GetMouseMotion().y * Seconds * pitchMultiplicator;
		m_camera.UpdateFromMouse(yawInput, pitchInput);
	}

	forwardInput *= Seconds * movementMultiplicator;
	rightInput *= Seconds * movementMultiplicator;
	upInput *= Seconds * movementMultiplicator;

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


	if (m_inputManager.GetKey(KeyCode::KEY_L).GetNumPressed() > 0)
	{
		std::puts("     --- Seperation -----        ");
		std::cout.flush();
	}
	if (false && m_inputManager.GetKey(KeyCode::KEY_1).GetNumPressed() > 0)
	{
		m_savedLocation = m_camera.CalcPosition();
	}


	if (false && m_inputManager.GetKey(KeyCode::KEY_2).GetNumPressed() > 0)
	{
		const glm::vec3 currentLocation = m_camera.CalcPosition();

		const PortalManager& portalmanger = m_graphcisBackend.GetPortalManager();

		m_drawOptions.extraLines.push_back(Line(glm::vec4(m_savedLocation, 1.f), glm::vec4(currentLocation, 1.f)));

		const Ray ray = Ray::FromStartAndEndpoint(m_savedLocation, currentLocation);

		std::optional<PortalManager::RayTraceResult> maybeResult =
			portalmanger.RayTrace(ray, m_graphcisBackend.GetTriangleMeshes());

		if (maybeResult.has_value())
		{
			const glm::vec3 hitPos = maybeResult->hitLocation;
			m_drawOptions.extraLines.push_back(Line(glm::vec4(hitPos.x, -100.f, hitPos.z, 1.f), glm::vec4(hitPos.x, +100.f, hitPos.z, 1.f)));
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
			m_drawOptions.extraLines.push_back(Line(missedA, missedB));
		}

		const bool b = maybeResult.has_value();
	}

	if (m_inputManager.GetKey(KeyCode::KEY_P).GetNumPressed() > 0)
	{
		m_drawOptions.maxRecursion = m_drawOptions.maxRecursion == 0 ? std::numeric_limits<int>::max() : 0;
	}

	if (m_inputManager.GetKey(KeyCode::KEY_F).GetNumPressed() > 0)
	{
		m_showRenderMilliseconds = !m_showRenderMilliseconds;
	}

	if (m_showRenderMilliseconds)
	{
		double avarage = std::accumulate(m_frameTimeBuckets.begin(), m_frameTimeBuckets.end(), 0.0) / frameTimeBuckedSize;
		std::printf("render Milliseconds: %f \n", avarage * 1000.f);

		std::cout.flush();
		m_showRenderMilliseconds = false;
	}

	if (m_inputManager.GetKey(KeyCode::KEY_M).GetNumPressed() > 0)
	{
		SetTestCase(0);
		framesTillSample = sampleWaitFrames;
		std::printf("testCase;average;median;min;max;\n");
	}

	if (m_inputManager.GetKey(KeyCode::KEY_0).GetNumPressed() > 0)
	{
		m_graphcisBackend.SetMaxVisiblePortalsForRecursion(testCases[0]);
	}

	if (currentCase != -1)
	{
		if (--framesTillSample == 0)
		{
			double avarage = std::accumulate(m_frameTimeBuckets.begin(), m_frameTimeBuckets.end(), 0.0) / frameTimeBuckedSize;
			auto [mintIter,maxIter] = std::minmax_element(m_frameTimeBuckets.begin(), m_frameTimeBuckets.end());
			const double minTime = *mintIter;
			const double maxTime = *maxIter;

			const double median = calcMedian(m_frameTimeBuckets);

			std::string testcaseString = printTestCase(testCases[currentCase]);
			std::printf("%s;%f;%f;%f;%f;\n"
				, testcaseString.c_str()
				, avarage * 1000.0
				, median * 1000.0
				, minTime * 1000.0
				, maxTime * 1000.0		
			);

			framesTillSample = sampleWaitFrames;

			int nextCase = currentCase + 1;
			if (nextCase < testCases.size())
			{
				std::cerr << "Next Case: " << nextCase << std::endl;
				SetTestCase(nextCase);
			}
			else
			{
				SetTestCase(-1);
				std::cerr << "Finished" << std::endl;
			}
		
		}
	}

	const glm::mat4 cam = m_camera.CalcMat();

	unsigned int bla = 0;

	if (m_inputManager.GetKey(KeyCode::KEY_C).GetNumPressed() > 0)
	{
		const uint32_t matrixCount = NTree::CalcTotalElements(m_graphcisBackend.GetPortalManager().GetPortalCount(), 6);
		std::vector<glm::mat4> matStorage(matrixCount);

		std::printf("Rescursion count;average;min;max;median;iterations;garbage value \n");
		for(int i = 0; i < 6; ++i)
		{
			const uint32_t iterMatrixCount = NTree::CalcTotalElements(m_graphcisBackend.GetPortalManager().GetPortalCount(), i + 1);
			const int iterations = matrixCount * 2 / iterMatrixCount;
			const double inverseIterations = 1.0 / iterations;
			std::array<double, 12> results;
			for (double& e : results)
			{		
				ClockType::time_point before = ClockType::now();
				for (int k = 0; k < iterations; ++k)
				{
					m_graphcisBackend.GetPortalManager().CreateCameraMats(cam, i, matStorage);
					for (int m = 0; m < iterMatrixCount; ++m)
					{
						matStorage[m] = glm::inverse(matStorage[m]);
					}

					bla += reinterpret_cast<unsigned int&>(matStorage[0][3][3]);
				}
				ClockType::time_point after = ClockType::now();
				e = DoubleSeconds(after - before).count() * inverseIterations;
			}

			double avarage = std::accumulate(results.begin(), results.end(), 0.0) / std::size(results);
			auto [mintIter, maxIter] = std::minmax_element(results.begin(), results.end());
			const double minTime = *mintIter;
			const double maxTime = *maxIter;

			const double median = calcMedian(results);

			std::printf("%d;%f;%f;%f;%f;%d;%d\n"
				, i
				, avarage * 1000'000.0
				, minTime * 1000'000.0
				, maxTime * 1000'000.0
				, median * 1000'000.0
				, iterations
				, bla
			);
/*
			std::printf(
				"\n"
				"Rescursion count : %d"
				"\n"
				"average: %f \n"
				"min: %f \n"
				"max: %f \n"
				"median: %f \n"
				"iterations %d\n"
				"garbage value %d\n"
				, i
				, avarage * 1000'000.0
				, minTime * 1000'000.0
				, maxTime * 1000'000.0
				, median * 1000'000.0
				, iterations
				, bla
			);*/

			std::cout.flush();

		}
	
	}
}

