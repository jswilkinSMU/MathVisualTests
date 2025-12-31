#include "Game/Game.h"
#include "Game/GameCommon.h"
#include "Game/App.h"

#include "Engine/Input/InputSystem.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Core/Rgba8.h"
#include "Engine/Core/Vertex_PCU.h"
#include "Engine/Core/EngineCommon.h"
#include "Engine/Core/Time.hpp"
#include "Engine/Math/MathUtils.h"
#include "Engine/Core/VertexUtils.h"

void Game::AdjustForPauseAndTimeDistortion(float& ds) 
{
	UNUSED(ds);

	if (g_theInput->IsKeyDown('T'))
	{
		g_theApp->m_gameClock->SetTimeScale(0.1);
	}
	else
	{
		g_theApp->m_gameClock->SetTimeScale(1.0);
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_ESC))
	{
		g_theEventSystem->FireEvent("Quit");
	}
}
