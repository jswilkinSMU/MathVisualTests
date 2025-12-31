#include "Game/App.h"
#include "Game/GameNearestPoint.hpp"
#include "Game/GameRaycastsVsDiscs.hpp"
#include "Game/GameRaycastVsLineSegments.hpp"
#include "Game/GameRaycastVsAABB2s.hpp"
#include "Game/Game3DTestShapes.hpp"
#include "Game/Game2DCurves.hpp"
#include "Game/Game2DPachinko.hpp"

#include "Engine/Core/Vertex_PCU.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Renderer/Camera.h"
#include "Engine/Input/InputSystem.h"
#include "Engine/Core/Time.hpp"
#include "Engine/Window/Window.hpp"
#include "Engine/Core/EventSystem.hpp"
#include "Engine/Core/EngineCommon.h"

RandomNumberGenerator* g_rng = nullptr; // Created and owned by the App
App* g_theApp = nullptr;				// Created and owned by Main_Windows.cpp
Renderer* g_theRenderer = nullptr;		// Created and owned by the App
Window* g_theWindow = nullptr;			// Created and owned by the App
Game* m_theGame;						// Owns the Game instance

App::App()
{
	m_gameClock = new Clock(Clock::GetSystemClock());
}

App::~App()
{
}

void App::Startup()
{
	// Loading game config
	LoadGameConfig("Data/GameConfig.xml");

	// Create all Engine Subsystems
	EventSystemConfig eventSystemConfig;
	g_theEventSystem = new EventSystem(eventSystemConfig);

	InputSystemConfig inputConfig;
	g_theInput = new InputSystem(inputConfig);

	WindowConfig windowConfig;
	windowConfig.m_aspectRatio = 2.f;
	windowConfig.m_inputSystem = g_theInput;
	windowConfig.m_windowTitle = "MathVisualTests MP2-A7: Planes and OBBs";
	g_theWindow = new Window(windowConfig);

	RendererConfig rendererConfig;
	rendererConfig.m_window = g_theWindow;
	g_theRenderer = new Renderer(rendererConfig);

	g_theEventSystem->Startup();
	g_theInput->Startup();
	g_theWindow->Startup();
	g_theRenderer->Startup();

	m_gameClock = new Clock(Clock::GetSystemClock());

	SubscribeToEvents();
	m_screenCamera.SetOrthoView(Vec2::ZERO, Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y));
	m_theGame = new GameNearestPoint(this);
}

void App::Shutdown()
{
	g_theRenderer->Shutdown();
	g_theWindow->Shutdown();
	g_theInput->Shutdown();
	g_theEventSystem->Shutdown();

	delete g_theRenderer;
	delete g_theWindow;
	delete g_theInput;
	delete g_theEventSystem;

	g_theRenderer = nullptr;
	g_theWindow = nullptr;
	g_theInput = nullptr;
	g_theEventSystem = nullptr;
}

void App::BeginFrame()
{
	Clock::GetSystemClock().TickSystemClock();
	g_theRenderer->BeginFrame();
	g_theWindow->BeginFrame();
	g_theInput->BeginFrame();
	g_theEventSystem->BeginFrame();
}

void App::Render() const
{
	g_theRenderer->ClearScreen(Rgba8(0, 0, 0));
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
	m_theGame->Render();
}

void App::Update()
{
	double deltaSeconds = m_gameClock->GetDeltaSeconds();
	m_theGame->Update(static_cast<float>(deltaSeconds));

	if (m_currentGameMode == GAME_MODE_3D_SHAPES_AND_QUERIES)
	{
		g_theInput->SetCursorMode(CursorMode::FPS);
	}
	else
	{
		g_theInput->SetCursorMode(CursorMode::POINTER);
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_F7))
	{
		m_currentGameMode = GetNextGameMode();
		m_theGame = CreateNewGameForMode(m_currentGameMode);
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_F6))
	{
		m_currentGameMode = GetPreviousGameMode();
		m_theGame = CreateNewGameForMode(m_currentGameMode);
	}
}

void App::EndFrame()
{
	g_theEventSystem->EndFrame();
	g_theInput->EndFrame();
	g_theWindow->EndFrame();
	g_theRenderer->EndFrame();
}

void App::LoadGameConfig(char const* gameConfigXMLFilePath)
{
	XmlDocument gameConfigXml;
	XmlError result = gameConfigXml.LoadFile(gameConfigXMLFilePath);
	if (result == tinyxml2::XML_SUCCESS)
	{
		XmlElement* rootElement = gameConfigXml.RootElement();
		if (rootElement)
		{
			g_gameConfigBlackboard.PopulateFromXmlElementAttributes(*rootElement);
		}
		else
		{
			DebuggerPrintf("WARNING: Game config from file \"%s\" was invalid (missing root element)\n", gameConfigXMLFilePath);
		}
	}
	else
	{
		DebuggerPrintf("WARNING: Failed to load game config from file \"%s\"\n", gameConfigXMLFilePath);
	}
}

void App::SubscribeToEvents()
{
	g_theEventSystem->SubscribeEventCallbackFunction("Quit", HandleQuitRequested);
}

Game* App::CreateNewGameForMode(GameMode mode)
{
	m_currentGameMode = mode;

	if (mode == GAME_MODE_NEAREST_POINT)
	{
		m_screenCamera.SetOrthoView(Vec2::ZERO, Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y));
		return new GameNearestPoint(this);
	}
	if (mode == GAME_MODE_RAYCAST_VS_DISCS)
	{
		m_screenCamera.SetOrthoView(Vec2::ZERO, Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y));
		return new GameRaycastVsDiscs(this);
	}
	if (mode == GAME_MODE_RAYCAST_VS_LINESEGMENTS)
	{
		m_screenCamera.SetOrthoView(Vec2::ZERO, Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y));
		return new GameRaycastVsLinesegments(this);
	}
	if (mode == GAME_MODE_RAYCAST_VS_AABB2S)
	{
		m_screenCamera.SetOrthoView(Vec2::ZERO, Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y));
		return new GameRaycastVsAABB2s(this);
	}
	if (mode == GAME_MODE_3D_SHAPES_AND_QUERIES)
	{
		m_screenCamera.SetOrthoView(Vec2::ZERO, Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y));
		m_worldCamera.SetPerspectiveView(2.f, 60.f, 0.1f, 100.f);
		return new Game3DTestShapes(this);
	}
	if (mode == GAME_MODE_2D_CURVES)
	{
		m_screenCamera.SetOrthoView(Vec2::ZERO, Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y));
		m_gameClock->SetTimeScale(0.5);
		return new Game2DCurves(this);
	}
	if (mode == GAME_MODE_2D_PACHINKO)
	{
		m_screenCamera.SetOrthoView(Vec2::ZERO, Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y));
		m_gameClock->SetTimeScale(2.0);
		return new Game2DPachinko(this);
	}
	else
	{
		return nullptr;
	}
}

void App::RunFrame()
{
	BeginFrame();
	Update();	
	Render();	
	EndFrame();	
}

void App::RunMainLoop()
{
	while (!IsQuitting())
	{
		RunFrame();
	}
}

bool App::HandleQuitRequested(EventArgs& args)
{
	UNUSED(args);
	g_theApp->m_isQuitting = true;
	return true;
}

GameMode App::GetNextGameMode()
{
	return static_cast<GameMode>((m_currentGameMode + 1) % GAME_MODE_COUNT);
}

GameMode App::GetPreviousGameMode()
{
	return static_cast<GameMode>((m_currentGameMode - 1 + GAME_MODE_COUNT) % GAME_MODE_COUNT);
}
