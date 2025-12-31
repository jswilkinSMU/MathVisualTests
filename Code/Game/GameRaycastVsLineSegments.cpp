#include "Game/GameRaycastVsLineSegments.hpp"
#include "Game/App.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Input/InputSystem.h"
#include "Engine/Core/EngineCommon.h"
#include "Engine/Math/RaycastUtils.hpp"

GameRaycastVsLinesegments::GameRaycastVsLinesegments(App* owner)
	:m_theApp(owner)
{
	m_font = g_theRenderer->CreateOrGetBitmapFont("Data/Fonts/SquirrelFixedFont");
	m_rayCastStart = Vec2(SCREEN_CENTER_X, SCREEN_CENTER_Y);
	m_rayCastEnd = Vec2(900.f, 300.f);
	RandomizeLineSegments();
	m_gameSceneCoords = AABB2(Vec2::ZERO, Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y));
}

void GameRaycastVsLinesegments::Update(float deltaSeconds)
{
	AdjustForPauseAndTimeDistortion(deltaSeconds);
	if (g_theInput->WasKeyJustPressed(KEYCODE_F8))
	{
		RandomizeLineSegments();
	}
	ArrowMovement();
}

void GameRaycastVsLinesegments::Render() const
{
	g_theRenderer->BeginCamera(g_theApp->m_screenCamera);
	DrawLineSegments();
	DrawRaycast();
	GameModeAndControlsText();
}

void GameRaycastVsLinesegments::RandomizeLineSegments()
{
	m_lineSegments.clear();

	for (int lineSegmentIndex = 0; lineSegmentIndex < NUM_LINES; ++lineSegmentIndex)
	{
		LineSegment newLineSegments;
		newLineSegments.m_lineStart = Vec2(g_rng->RollRandomFloatInRange(0.f, SCREEN_SIZE_X), g_rng->RollRandomFloatInRange(0.f, SCREEN_SIZE_Y));
		newLineSegments.m_lineEnd = Vec2(g_rng->RollRandomFloatInRange(0.f, SCREEN_SIZE_X), g_rng->RollRandomFloatInRange(0.f, SCREEN_SIZE_Y));

		Vec2 lineDirection = newLineSegments.m_lineEnd - newLineSegments.m_lineStart;
		float lineLength = lineDirection.GetLength();

		if (lineLength > LINE_MAX_LENGTH)
		{
			lineDirection.Normalize();
			lineDirection *= LINE_MAX_LENGTH;
			newLineSegments.m_lineEnd = newLineSegments.m_lineStart + lineDirection;
		}
		m_lineSegments.push_back(newLineSegments);
	}
}

void GameRaycastVsLinesegments::GameModeAndControlsText() const
{
	std::vector<Vertex_PCU> textVerts;
	m_font->AddVertsForTextInBox2D(textVerts, "Mode (F6/F7 for Prev/Next): Raycast vs. Line Segments (2D)", m_gameSceneCoords, 15.f, Rgba8::GOLD, 0.8f, Vec2(0.f, 0.97f));
	m_font->AddVertsForTextInBox2D(textVerts, "F8 to Randomize; LMB/RMB set ray start/end; ESDF move start; IJKL move end; Arrows move ray; Hold T for slow", m_gameSceneCoords, 15.f, Rgba8::ALICEBLUE, 0.8f, Vec2(0.f, 0.945f));
	g_theRenderer->BindTexture(&m_font->GetTexture());
	g_theRenderer->DrawVertexArray(textVerts);
}

void GameRaycastVsLinesegments::DrawLineSegments() const
{
	for (int lineSegmentIndex = 0; lineSegmentIndex < (int)m_lineSegments.size(); ++lineSegmentIndex)
	{
		LineSegment const& lineSegment = m_lineSegments[lineSegmentIndex];
		std::vector<Vertex_PCU> lineSegmentVerts;

		AddVertsForLineSegment2D(lineSegmentVerts, lineSegment.m_lineStart, lineSegment.m_lineEnd, 3.f, Rgba8::SAPPHIRE);
		g_theRenderer->BindTexture(nullptr);
		g_theRenderer->DrawVertexArray(lineSegmentVerts);
	}
}

void GameRaycastVsLinesegments::DrawRaycast() const
{
	Vec2 startToEnd = m_rayCastEnd - m_rayCastStart;
	Vec2 rayCastDirection = startToEnd.GetNormalized();
	float maxDist = startToEnd.GetLength();

	std::vector<Vertex_PCU> arrowVerts;
	bool didRaycasthitLine = false;
	RaycastResult2D nearestImpact;
	int nearestLine = 0;

	for (int lineIndex = 0; lineIndex < (int)m_lineSegments.size(); ++lineIndex)
	{
		RaycastResult2D raycastResult = RaycastVsLineSegment2D(m_rayCastStart, rayCastDirection, maxDist, m_lineSegments[lineIndex].m_lineStart, m_lineSegments[lineIndex].m_lineEnd);

		if (raycastResult.m_didImpact)
		{
			if (didRaycasthitLine == false || raycastResult.m_impactDist < nearestImpact.m_impactDist)
			{
				nearestImpact = raycastResult;
				nearestLine = lineIndex;
				didRaycasthitLine = true;
			}
		}
	}

	if (didRaycasthitLine)
	{
		std::vector<Vertex_PCU> impactedLineVerts;

		AddVertsForArrow2D(arrowVerts, m_rayCastStart, m_rayCastEnd, 20.f, 1.f, Rgba8::DARKGRAY);
		AddVertsForArrow2D(arrowVerts, m_rayCastStart, nearestImpact.m_impactPos, 20.f, 1.f, Rgba8::ORANGE);
		AddVertsForArrow2D(arrowVerts, nearestImpact.m_impactPos, nearestImpact.m_impactPos + nearestImpact.m_impactNormal * 80.f, 20.f, 1.f, Rgba8::CYAN);
		AddVertsForDisc2D(arrowVerts, nearestImpact.m_impactPos, 4.f, Rgba8::WHITE);

		AddVertsForLineSegment2D(impactedLineVerts, m_lineSegments[nearestLine].m_lineStart, m_lineSegments[nearestLine].m_lineEnd, 3.f, Rgba8::LIGHTBLUE);
		g_theRenderer->BindTexture(nullptr);
		g_theRenderer->DrawVertexArray(impactedLineVerts);
	}
	else
	{
		AddVertsForArrow2D(arrowVerts, m_rayCastStart, m_rayCastEnd, 20.f, 3.f, Rgba8::WHITE);
	}
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->DrawVertexArray(arrowVerts);
}

void GameRaycastVsLinesegments::ArrowMovement()
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
