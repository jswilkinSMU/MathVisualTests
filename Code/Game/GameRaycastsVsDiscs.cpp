#include "GameRaycastsVsDiscs.hpp"
#include "Game/App.h"
#include "Engine/Core/Rgba8.h"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Core/Vertex_PCU.h"
#include "Engine/Core/VertexUtils.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Input/InputSystem.h"
#include "Engine/Math/RaycastUtils.hpp"

GameRaycastVsDiscs::GameRaycastVsDiscs(App* owner)
	:m_theApp(owner)
{
	m_font = g_theRenderer->CreateOrGetBitmapFont("Data/Fonts/SquirrelFixedFont");


	RandomizeDiscs();
	m_gameSceneCoords = AABB2(Vec2::ZERO, Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y));
}

void GameRaycastVsDiscs::Update(float deltaSeconds)
{
	m_rayCastStart = Vec2(SCREEN_CENTER_X, SCREEN_CENTER_Y);
	m_rayCastEnd = Vec2(900.f, 300.f);
	AdjustForPauseAndTimeDistortion(deltaSeconds);
	// Disc randomizing
	if (g_theInput->WasKeyJustPressed(KEYCODE_F8))
	{
		RandomizeDiscs();
	}
	ArrowMovement();
}

void GameRaycastVsDiscs::Render() const
{
	g_theRenderer->BeginCamera(g_theApp->m_screenCamera);
	DrawDiscs();
	DrawRaycast();
	GameModeAndControlsText();
}

void GameRaycastVsDiscs::ArrowMovement()
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

void GameRaycastVsDiscs::DrawDiscs() const
{
	for (int discIndex = 0; discIndex < (int)m_discs.size(); ++discIndex)
	{
		Disc const& disc = m_discs[discIndex];
		std::vector<Vertex_PCU> discVerts;

		AddVertsForDisc2D(discVerts, disc.m_discCenter, disc.m_discRadius, Rgba8::SAPPHIRE);
		g_theRenderer->BindTexture(nullptr);
		g_theRenderer->DrawVertexArray(discVerts);
	}
}

void GameRaycastVsDiscs::DrawRaycast() const
{
	Vec2 startToEnd = m_rayCastEnd - m_rayCastStart;
	Vec2 rayCastDirection = startToEnd.GetNormalized();
	float maxDist = startToEnd.GetLength();

	std::vector<Vertex_PCU> arrowVerts;
	bool didRayHitDisc = false;
	RaycastResult2D nearestImpact;
	int nearestDisc = 0;

	for (int discIndex = 0; discIndex < (int)m_discs.size(); ++discIndex)
	{
		RaycastResult2D rayCastResult = RaycastVsDisc2D(m_rayCastStart, rayCastDirection, maxDist, m_discs[discIndex].m_discCenter, m_discs[discIndex].m_discRadius);

		if (rayCastResult.m_didImpact)
		{
			if (didRayHitDisc == false || rayCastResult.m_impactDist < nearestImpact.m_impactDist)
			{
				nearestImpact = rayCastResult;
				nearestDisc = discIndex;
				didRayHitDisc = true;
			}
		}
	}

	if (didRayHitDisc)
	{
		std::vector<Vertex_PCU> impactedDiscVerts;

		AddVertsForArrow2D(arrowVerts, m_rayCastStart, m_rayCastEnd, 20.f, 1.f, Rgba8::DARKGRAY);
		AddVertsForArrow2D(arrowVerts, m_rayCastStart, nearestImpact.m_impactPos, 20.f, 1.f, Rgba8::ORANGE);
		AddVertsForArrow2D(arrowVerts, nearestImpact.m_impactPos, nearestImpact.m_impactPos + nearestImpact.m_impactNormal * 100.f, 20.f, 1.f, Rgba8::CYAN);
		AddVertsForDisc2D(arrowVerts, nearestImpact.m_impactPos, 4.f, Rgba8::WHITE);

		AddVertsForDisc2D(impactedDiscVerts, m_discs[nearestDisc].m_discCenter, m_discs[nearestDisc].m_discRadius, Rgba8::LIGHTBLUE);
		g_theRenderer->BindTexture(nullptr);
		g_theRenderer->DrawVertexArray(impactedDiscVerts);
	}
	else
	{
		AddVertsForArrow2D(arrowVerts, m_rayCastStart, m_rayCastEnd, 20.f, 3.f, Rgba8::WHITE);
	}
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->DrawVertexArray(arrowVerts);
}

void GameRaycastVsDiscs::GameModeAndControlsText() const
{
	std::vector<Vertex_PCU> textVerts;
	m_font->AddVertsForTextInBox2D(textVerts, "Mode (F6/F7 for Prev/Next): Raycast vs. Discs (2D)", m_gameSceneCoords, 15.f, Rgba8::GOLD, 0.8f, Vec2(0.f, 0.97f));
	m_font->AddVertsForTextInBox2D(textVerts, "F8 to Randomize; LMB/RMB set ray start/end; ESDF move start; IJKL move end; Arrows move ray; Hold T for slow", m_gameSceneCoords, 15.f, Rgba8::ALICEBLUE, 0.8f, Vec2(0.f, 0.945f));
	g_theRenderer->BindTexture(&m_font->GetTexture());
	g_theRenderer->DrawVertexArray(textVerts);
}

void GameRaycastVsDiscs::RandomizeDiscs()
{
	m_discs.clear();
	for (int discsIndex = 0; discsIndex < NUM_DISCS; ++discsIndex)
	{
		Disc newDiscs;
		newDiscs.m_discCenter = Vec2(g_rng->RollRandomFloatInRange(0.f, SCREEN_SIZE_X), g_rng->RollRandomFloatInRange(0.f, SCREEN_SIZE_Y));
		newDiscs.m_discRadius = g_rng->RollRandomFloatInRange(10.f, 170.f);
		m_discs.push_back(newDiscs);
	}
}
