#pragma once
#include "Game/Game.h"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Renderer/Camera.h"
#include "Engine/Core/EventSystem.hpp"
#include "Engine/Core/Clock.hpp"

class App
{
public:
	App();
	~App();
	void Startup();
	void Shutdown();
	void RunFrame();

	void RunMainLoop();
	bool IsQuitting() const { return m_isQuitting; }
	static bool HandleQuitRequested(EventArgs& args);

	GameMode GetNextGameMode();
	GameMode GetPreviousGameMode();
	Camera m_screenCamera;
	Camera m_worldCamera;
	Clock*  m_gameClock = nullptr;
	
private:
	void BeginFrame();
	void Update();
	void Render() const;
	void EndFrame();

	void LoadGameConfig(char const* gameConfigXMLFilePath);
	void SubscribeToEvents();
	Game* CreateNewGameForMode(GameMode mode);

private:
	GameMode m_currentGameMode = GAME_MODE_NEAREST_POINT;
	Game* m_game = nullptr;
	float m_timeLastFrameStart = 0.0f;
	bool  m_isQuitting = false;
};