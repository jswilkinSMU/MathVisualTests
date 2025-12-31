#include "Game/GameRaycastVsAABB2s.hpp"
#include "Game/App.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Input/InputSystem.h"
#include "Engine/Core/EngineCommon.h"
#include "Engine/Math/RaycastUtils.hpp"

GameRaycastVsAABB2s::GameRaycastVsAABB2s(App* owner)
	:m_theApp(owner)
{
	m_font = g_theRenderer->CreateOrGetBitmapFont("Data/Fonts/SquirrelFixedFont");
	m_rayCastStart = Vec2(SCREEN_CENTER_X, SCREEN_CENTER_Y);
	m_rayCastEnd = Vec2(900.f, 300.f);
	RandomizeAABB2s();
	m_gameSceneCoords = AABB2(Vec2::ZERO, Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y));
}

void GameRaycastVsAABB2s::Update(float deltaSeconds)
{
	AdjustForPauseAndTimeDistortion(deltaSeconds);
	// AABB2 randomizing
	if (g_theInput->WasKeyJustPressed(KEYCODE_F8))
	{
		RandomizeAABB2s();
	}
	ArrowMovement();
}

void GameRaycastVsAABB2s::Render() const
{
	g_theRenderer->BeginCamera(g_theApp->m_screenCamera);
	DrawAABB2s();
	DrawRaycast();
	GameModeAndControlsText();
}

void GameRaycastVsAABB2s::RandomizeAABB2s()
{
	m_AABB2s.clear();
	for (int aabb2Index = 0; aabb2Index < NUM_AABB2S; ++aabb2Index)
	{
		AABB2 newAABB2s;
		newAABB2s.m_mins = Vec2(g_rng->RollRandomFloatInRange(0.f, SCREEN_SIZE_X - AABB2_MAX_SIZE), g_rng->RollRandomFloatInRange(0.f, SCREEN_SIZE_Y - AABB2_MAX_SIZE));
		newAABB2s.m_maxs = newAABB2s.m_mins + Vec2(g_rng->RollRandomFloatInRange(AABB2_MIN_SIZE, AABB2_MAX_SIZE), g_rng->RollRandomFloatInRange(AABB2_MIN_SIZE, AABB2_MAX_SIZE));

		m_AABB2s.push_back(newAABB2s);
	}
}

void GameRaycastVsAABB2s::ArrowMovement()
{
	// Ray start movement
	if (g_theInput->IsKeyDown('E'))
	{
		m_rayCastStart += Vec2(0.f, 1.f);
	}
	if (g_theInput->IsKeyDown('S'))
	{
		m_rayCastStart += Vec2(-1.f, 0.f);
	}
	if (g_theInput->IsKeyDown('D'))
	{
		m_rayCastStart += Vec2(0.f, -1.f);
	}
	if (g_theInput->IsKeyDown('F'))
	{
		m_rayCastStart += Vec2(1.f, 0.f);
	}

	// Ray end movement
	if (g_theInput->IsKeyDown('I'))
	{
		m_rayCastEnd += Vec2(0.f, 1.f);
	}
	if (g_theInput->IsKeyDown('J'))
	{
		m_rayCastEnd += Vec2(-1.f, 0.f);
	}
	if (g_theInput->IsKeyDown('K'))
	{
		m_rayCastEnd += Vec2(0.f, -1.f);
	}
	if (g_theInput->IsKeyDown('L'))
	{
		m_rayCastEnd += Vec2(1.f, 0.f);
	}

	// Full ray movement
	if (g_theInput->IsKeyDown(KEYCODE_UPARROW))
	{
		m_rayCastStart += Vec2(0.f, 1.f);
		m_rayCastEnd += Vec2(0.f, 1.f);
	}
	if (g_theInput->IsKeyDown(KEYCODE_LEFTARROW))
	{
		m_rayCastStart += Vec2(-1.f, 0.f);
		m_rayCastEnd += Vec2(-1.f, 0.f);
	}
	if (g_theInput->IsKeyDown(KEYCODE_DOWNARROW))
	{
		m_rayCastStart += Vec2(0.f, -1.f);
		m_rayCastEnd += Vec2(0.f, -1.f);
	}
	if (g_theInput->IsKeyDown(KEYCODE_RIGHTARROW))
	{
		m_rayCastStart += Vec2(1.f, 0.f);
		m_rayCastEnd += Vec2(1.f, 0.f);
	}

	if (g_theInput->WasKeyJustPressed('V'))
	{
		m_rayCastEnd.x = m_rayCastStart.x;
	}
	if (g_theInput->WasKeyJustPressed('H'))
	{
		m_rayCastEnd.y = m_rayCastStart.y;
	}

	if (g_theInput->IsKeyDown(KEYCODE_LEFT_MOUSE))
	{
		Vec2 normalizedMouseUV = g_theInput->GetCursorNormalizedPosition();
		m_rayCastStart = m_gameSceneCoords.GetPointAtUV(normalizedMouseUV);
	}

	if (g_theInput->IsKeyDown(KEYCODE_RIGHT_MOUSE))
	{
		Vec2 normalizedMouseUV = g_theInput->GetCursorNormalizedPosition();
		m_rayCastEnd = m_gameSceneCoords.GetPointAtUV(normalizedMouseUV);
	}
}

void GameRaycastVsAABB2s::DrawAABB2s() const
{
	for (int aabb2Index = 0; aabb2Index < (int)m_AABB2s.size(); ++aabb2Index)
	{
		AABB2 const& aabb2 = m_AABB2s[aabb2Index];
		std::vector<Vertex_PCU> aabb2Verts;

		AddVertsForAABB2D(aabb2Verts, AABB2(aabb2.m_mins, aabb2.m_maxs), Rgba8::SAPPHIRE);
		g_theRenderer->BindTexture(nullptr);
		g_theRenderer->DrawVertexArray(aabb2Verts);
	}
}

void GameRaycastVsAABB2s::DrawRaycast() const
{
	Vec2 startToEnd = m_rayCastEnd - m_rayCastStart;
	Vec2 rayCastDirection = startToEnd.GetNormalized();
	float maxDist = startToEnd.GetLength();

	std::vector<Vertex_PCU> arrowVerts;
	bool didRaycasthitAABB2 = false;
	RaycastResult2D nearestImpact;
	int nearestAABB2 = 0;

	for (int aabb2Index = 0; aabb2Index < (int)m_AABB2s.size(); ++aabb2Index)
	{
		RaycastResult2D raycastResult = RaycastVsAABB2D(m_rayCastStart, rayCastDirection, maxDist, AABB2(m_AABB2s[aabb2Index].m_mins, m_AABB2s[aabb2Index].m_maxs));

		if (raycastResult.m_didImpact)
		{
			if (didRaycasthitAABB2 == false || raycastResult.m_impactDist < nearestImpact.m_impactDist)
			{
				nearestImpact = raycastResult;
				nearestAABB2 = aabb2Index;
				didRaycasthitAABB2 = true;
			}
		}
	}

	if (didRaycasthitAABB2)
	{
		std::vector<Vertex_PCU> impactedAABB2Verts;

		AddVertsForArrow2D(arrowVerts, m_rayCastStart, m_rayCastEnd, 20.f, 1.f, Rgba8::DARKGRAY);
		AddVertsForArrow2D(arrowVerts, m_rayCastStart, nearestImpact.m_impactPos, 20.f, 1.f, Rgba8::ORANGE);
		AddVertsForArrow2D(arrowVerts, nearestImpact.m_impactPos, nearestImpact.m_impactPos + nearestImpact.m_impactNormal * 80.f, 20.f, 1.f, Rgba8::CYAN);
		AddVertsForDisc2D(arrowVerts, nearestImpact.m_impactPos, 4.f, Rgba8::WHITE);

		AddVertsForAABB2D(impactedAABB2Verts, AABB2(m_AABB2s[nearestAABB2].m_mins, m_AABB2s[nearestAABB2].m_maxs), Rgba8::LIGHTBLUE);
		g_theRenderer->BindTexture(nullptr);
		g_theRenderer->DrawVertexArray(impactedAABB2Verts);
	}
	else
	{
		AddVertsForArrow2D(arrowVerts, m_rayCastStart, m_rayCastEnd, 20.f, 3.f, Rgba8::WHITE);
	}
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->DrawVertexArray(arrowVerts);
}

void GameRaycastVsAABB2s::GameModeAndControlsText() const
{
	std::vector<Vertex_PCU> textVerts;
	m_font->AddVertsForTextInBox2D(textVerts, "Mode (F6/F7 for Prev/Next): Raycast vs. AABB2s (2D)", m_gameSceneCoords, 15.f, Rgba8::GOLD, 0.8f, Vec2(0.f, 0.97f));
	m_font->AddVertsForTextInBox2D(textVerts, "F8 to Randomize; LMB/RMB set ray start/end; ESDF move start; IJKL move end; Arrows move ray; Hold T for slow; Press V to snap vertically; Press H to snap Horizontally", m_gameSceneCoords, 15.f, Rgba8::ALICEBLUE, 0.8f, Vec2(0.f, 0.945f));
	g_theRenderer->BindTexture(&m_font->GetTexture());
	g_theRenderer->DrawVertexArray(textVerts);
}
