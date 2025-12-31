#include "Game/GameNearestPoint.hpp"
#include "Game/App.h"
#include "Engine/Core/Vertex_PCU.h"
#include "Engine/Core/VertexUtils.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Input/InputSystem.h"
#include "Engine/Window/Window.hpp"
#include "Engine/Math/AABB2.h"
#include "Engine/Math/OBB2.hpp"
#include "Engine/Math/MathUtils.h"

GameNearestPoint::GameNearestPoint(App* owner)
	:m_theApp(owner)
{
	m_font = g_theRenderer->CreateOrGetBitmapFont("Data/Fonts/SquirrelFixedFont");
	RandomShapes();

	m_playerPoint = Vec2(SCREEN_CENTER_X, SCREEN_CENTER_Y);
	m_isSlowMo = false;
	m_gameSceneCoords = AABB2(Vec2::ZERO , Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y));
}

GameNearestPoint::~GameNearestPoint()
{
}

void GameNearestPoint::Update(float deltaSeconds)
{
	// Calls slowMo and esc key presses
	AdjustForPauseAndTimeDistortion(deltaSeconds);

	// Shape randomizing
	if (g_theInput->WasKeyJustPressed(KEYCODE_F8)) 
	{
		RandomShapes();
	}

	PlayerMovement(deltaSeconds);
	GetNearestPointCheck();
}

void GameNearestPoint::PlayerMovement(float deltaSeconds)
{
	// Player movement
	float movement = PLAYER_SPEED * deltaSeconds;

	if (g_theInput->IsKeyDown(KEYCODE_UPARROW) || g_theInput->IsKeyDown('E'))
	{
		m_playerPoint.y += movement;
	}
	if (g_theInput->IsKeyDown(KEYCODE_DOWNARROW) || g_theInput->IsKeyDown('D'))
	{
		m_playerPoint.y -= movement;
	}
	if (g_theInput->IsKeyDown(KEYCODE_LEFTARROW) || g_theInput->IsKeyDown('S'))
	{
		m_playerPoint.x -= movement;
	}
	if (g_theInput->IsKeyDown(KEYCODE_RIGHTARROW) || g_theInput->IsKeyDown('F'))
	{
		m_playerPoint.x += movement;
	}

	if (g_theInput->IsKeyDown(KEYCODE_LEFT_MOUSE))
	{
		Vec2 normalizedMouseUV = g_theWindow->GetNormalizedMouseUV();
		m_playerPoint = m_gameSceneCoords.GetPointAtUV(normalizedMouseUV);
	}
}

void GameNearestPoint::Render() const
{
	g_theRenderer->BeginCamera(g_theApp->m_screenCamera);
	// Rendering shapes
	RenderDisc();
	RenderAABB2();
	RenderOBB2();
	RenderCapsule();
	RenderTriangle();
	RenderLineSegment();
	RenderInfiniteLine();
	RenderPlayerPoint();

	// Rendering gold orange nearest Point
	RenderNearestPoint(m_nearestDiscPoint, Rgba8(255, 160, 0));
	RenderNearestPoint(m_nearestAABBPoint, Rgba8(255, 160, 0));
	RenderNearestPoint(m_nearestOBBPoint, Rgba8(255, 160, 0));
	RenderNearestPoint(m_nearestCapsulePoint, Rgba8(255, 160, 0));
	RenderNearestPoint(m_nearestTrianglePoint, Rgba8(255, 160, 0));
	RenderNearestPoint(m_nearestLineSegmentPoint, Rgba8(255, 160, 0));
	RenderNearestPoint(m_nearestInfiniteLinePoint, Rgba8(255, 160, 0));

	// Rendering white lines going from player to shapes
	LineToPoint(m_nearestDiscPoint);
	LineToPoint(m_nearestAABBPoint);
	LineToPoint(m_nearestOBBPoint);
	LineToPoint(m_nearestCapsulePoint);
	LineToPoint(m_nearestTrianglePoint);
	LineToPoint(m_nearestLineSegmentPoint);
	LineToPoint(m_nearestInfiniteLinePoint);

	GameModeAndControlsText();
}

void GameNearestPoint::GameModeAndControlsText() const
{
	std::vector<Vertex_PCU> textVerts;
	m_font->AddVertsForTextInBox2D(textVerts, "Mode (F6/F7 for Prev/Next): Nearest Point (2D)", m_gameSceneCoords, 15.f, Rgba8::GOLD, 0.8f, Vec2(0.f, 0.97f));
	m_font->AddVertsForTextInBox2D(textVerts, "F8 to Randomize; LMB to move dot; ESDF to move dot; Arrows to move dot; Hold T to slow", m_gameSceneCoords, 15.f, Rgba8::ALICEBLUE, 0.8f, Vec2(0.f, 0.945f));
	g_theRenderer->BindTexture(&m_font->GetTexture());
	g_theRenderer->DrawVertexArray(textVerts);
}

void GameNearestPoint::RenderNearestPoint(Vec2 const& point, Rgba8 color) const
{
	if (point == m_closestPointToPlayer)
	{
		color = Rgba8::LIMEGREEN;
	}
	else
	{
		color = Rgba8::ORANGE;
	}

	std::vector<Vertex_PCU> verts;
	AddVertsForDisc2D(verts, point, 5.f, color);
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->DrawVertexArray(static_cast<int>(verts.size()), verts.data());

	if (m_playerPoint == point) 
	{
		AddVertsForDisc2D(verts, m_playerPoint, 3.f, Rgba8(255, 255, 255));
		g_theRenderer->BindTexture(nullptr);
		g_theRenderer->DrawVertexArray(static_cast<int>(verts.size()), verts.data());
	}
}

void GameNearestPoint::LineToPoint(Vec2 const& point) const
{
	Rgba8 fadedWhite(255, 255, 255, 30);
	std::vector<Vertex_PCU> verts;

	AddVertsForLineSegment2D(verts, m_playerPoint, point, 1.f, fadedWhite);
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->DrawVertexArray(static_cast<int>(verts.size()), verts.data());
}

void GameNearestPoint::RandomShapes()
{
	// Disc randomizing
	m_discCenter = Vec2(g_rng->RollRandomFloatInRange(50.f, SCREEN_SIZE_X - 50.f), g_rng->RollRandomFloatInRange(50.f, SCREEN_SIZE_Y - 50.f));
	m_discRadius = g_rng->RollRandomFloatInRange(10.f, 100.f);

	// AABB2 randomizing
	float boxWidth = g_rng->RollRandomFloatInRange(10.f, MAX_AABB2_WIDTH); 
	float boxHeight = g_rng->RollRandomFloatInRange(10.f, MAX_AABB2_HEIGHT);
	Vec2 minPoint(g_rng->RollRandomFloatInRange(50.f, SCREEN_SIZE_X - boxWidth - 50.f), g_rng->RollRandomFloatInRange(50.f, SCREEN_SIZE_Y - boxHeight - 50.f));
	Vec2 maxPoint = minPoint + Vec2(boxWidth, boxHeight);
	m_alignedBox = AABB2(minPoint, maxPoint);

	// OBB2 randomizing
	float halfWidth = g_rng->RollRandomFloatInRange(10.f, 50.f);
	float halfHeight = g_rng->RollRandomFloatInRange(10.f, 50.f);
	Vec2 boxCenter(g_rng->RollRandomFloatInRange(50.f + halfWidth, SCREEN_SIZE_X - 50.f - halfWidth), g_rng->RollRandomFloatInRange(50.f + halfHeight, SCREEN_SIZE_Y - 50.f - halfHeight));
	float angle = g_rng->RollRandomFloatInRange(0.f, 360.f);
	Vec2 iBasisNormal(CosDegrees(angle), SinDegrees(angle));
	m_orientedBox = OBB2(boxCenter, iBasisNormal, Vec2(halfWidth, halfHeight));

	// Capsule randomizing
	m_boneStart = Vec2(g_rng->RollRandomFloatInRange(50.f, SCREEN_SIZE_X - 50.f), g_rng->RollRandomFloatInRange(50.f, SCREEN_SIZE_Y - 50.f));
	m_boneEnd = Vec2(g_rng->RollRandomFloatInRange(50.f, SCREEN_SIZE_X - 50.f), g_rng->RollRandomFloatInRange(50.f, SCREEN_SIZE_Y - 50.f));
	m_capsuleRadius = g_rng->RollRandomFloatInRange(15.f, 45.f);

	// Triangle randomizing
	m_ccw0 = Vec2(g_rng->RollRandomFloatInRange(700.f, 1000.f), g_rng->RollRandomFloatInRange(300.f, 700.f));
	m_ccw1 = Vec2(g_rng->RollRandomFloatInRange(700.f, 1000.f), g_rng->RollRandomFloatInRange(300.f, 700.f));
	m_ccw2 = Vec2(g_rng->RollRandomFloatInRange(700.f, 1000.f), g_rng->RollRandomFloatInRange(300.f, 700.f));

	// Line Segment randomizing
	m_start = Vec2(g_rng->RollRandomFloatInRange(50.f, SCREEN_SIZE_X - 50.f), g_rng->RollRandomFloatInRange(50.f, SCREEN_SIZE_Y - 50.f));
	m_end = Vec2(g_rng->RollRandomFloatInRange(50.f, SCREEN_SIZE_X - 50.f), g_rng->RollRandomFloatInRange(50.f, SCREEN_SIZE_Y - 50.f));
	m_thickness = (g_rng->RollRandomFloatInRange(1.f, 10.f));

	// Infinite Line randomizing
	m_infiniteStart = Vec2(g_rng->RollRandomFloatInRange(-1000.f, SCREEN_SIZE_X + 1000.f), g_rng->RollRandomFloatInRange(-1000.f, SCREEN_SIZE_Y + 1000.f));
	m_infiniteEnd = Vec2(g_rng->RollRandomFloatInRange(-1000.f, SCREEN_SIZE_X + 1000.f), g_rng->RollRandomFloatInRange(-1000.f, SCREEN_SIZE_Y + 1000.f));
	m_infiniteThickness = (g_rng->RollRandomFloatInRange(1.f, 15.f));
}

void GameNearestPoint::GetNearestPointCheck()
{
	m_nearestDiscPoint = GetNearestPointOnDisc2D(m_playerPoint, m_discCenter, m_discRadius);
	m_nearestAABBPoint = GetNearestPointOnAABB2D(m_playerPoint, m_alignedBox);
	m_nearestOBBPoint = GetNearestPointOnOBB2D(m_playerPoint, m_orientedBox);
	m_nearestCapsulePoint = GetNearestPointOnCapsule2D(m_playerPoint, m_boneStart, m_boneEnd, m_capsuleRadius);
	m_nearestTrianglePoint = GetNearestPointOnTriangle2D(m_playerPoint, m_ccw0, m_ccw1, m_ccw2);
	m_nearestLineSegmentPoint = GetNearestPointOnLineSegment2D(m_playerPoint, m_start, m_end);
	m_nearestInfiniteLinePoint = GetNearestPointOnInfiniteLine2D(m_playerPoint, m_infiniteStart, m_infiniteEnd);

	GetClosestPointToPlayer();
}

void GameNearestPoint::GetClosestPointToPlayer()
{
	float closestDistance = 99999.f;
	Vec2 closestPoint;
	std::vector<Vec2> points = { m_nearestDiscPoint, m_nearestAABBPoint, m_nearestOBBPoint, m_nearestCapsulePoint, m_nearestTrianglePoint, m_nearestLineSegmentPoint, m_nearestInfiniteLinePoint };

	for (int pointIndex = 0; pointIndex < static_cast<int>(points.size()); ++pointIndex)
	{
		float dist = (points[pointIndex] - m_playerPoint).GetLengthSquared();
		if (dist < closestDistance)
		{
			closestDistance = dist;
			closestPoint = points[pointIndex];
		}
	}
	m_closestPointToPlayer = closestPoint;
}

void GameNearestPoint::RenderDisc() const
{
	std::vector<Vertex_PCU> verts;

	Rgba8 color(102, 153, 204);
	Rgba8 brighterColor(173, 216, 230);

	if (IsPointInsideDisc2D(m_playerPoint, m_discCenter, m_discRadius)) 
	{
		AddVertsForDisc2D(verts, m_discCenter, m_discRadius, brighterColor);
	}
	else 
	{
		AddVertsForDisc2D(verts, m_discCenter, m_discRadius, color);
	}
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->DrawVertexArray(verts);
}

void GameNearestPoint::RenderAABB2() const
{
	std::vector<Vertex_PCU> verts;

	Rgba8 color(102, 153, 204);
	Rgba8 brighterColor(173, 216, 230);

	if (IsPointInsideAABB2D(m_playerPoint, m_alignedBox))
	{
		AddVertsForAABB2D(verts, m_alignedBox, brighterColor);
	}
	else
	{
		AddVertsForAABB2D(verts, m_alignedBox, color);
	}
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->DrawVertexArray(verts);
}

void GameNearestPoint::RenderOBB2() const
{
	std::vector<Vertex_PCU> verts;

	Rgba8 color(102, 153, 204);
	Rgba8 brighterColor(173, 216, 230);

	if (IsPointInsideOBB2D(m_playerPoint, m_orientedBox))
	{
		AddVertsForOBB2D(verts, m_orientedBox, brighterColor);
	}
	else
	{
		AddVertsForOBB2D(verts, m_orientedBox, color);
	}
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->DrawVertexArray(verts);
}

void GameNearestPoint::RenderCapsule() const
{
	std::vector<Vertex_PCU> verts;

	Rgba8 color(102, 153, 204);
	Rgba8 brighterColor(173, 216, 230);

	if (IsPointInsideCapsule(m_playerPoint, m_boneStart, m_boneEnd, m_capsuleRadius))
	{
		AddVertsForCapsule2D(verts, m_boneStart, m_boneEnd, m_capsuleRadius, brighterColor);
	}
	else
	{
		AddVertsForCapsule2D(verts, m_boneStart, m_boneEnd, m_capsuleRadius, color);
	}
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->DrawVertexArray(verts);
}

void GameNearestPoint::RenderTriangle() const
{
	std::vector<Vertex_PCU> verts;

	Rgba8 color(102, 153, 204);
	Rgba8 brighterColor(173, 216, 230);

	if (IsPointInsideTriangle2D(m_playerPoint, m_ccw0, m_ccw1, m_ccw2))
	{
		AddVertsForTriangle2D(verts, m_ccw0, m_ccw1, m_ccw2, brighterColor);
	}
	else
	{
		AddVertsForTriangle2D(verts, m_ccw0, m_ccw1, m_ccw2, color);
	}
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->DrawVertexArray(verts);
}

void GameNearestPoint::RenderLineSegment() const
{
	std::vector<Vertex_PCU> verts;
	Rgba8 color(102, 153, 204);

	AddVertsForLineSegment2D(verts, m_start, m_end, m_thickness, color);
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->DrawVertexArray(verts);
}

void GameNearestPoint::RenderInfiniteLine() const
{
	std::vector<Vertex_PCU> verts;
	Rgba8 color(102, 153, 204);

	AddVertsForLineSegment2D(verts, m_infiniteStart, m_infiniteEnd, m_infiniteThickness, color);
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->DrawVertexArray(verts);
}

void GameNearestPoint::RenderPlayerPoint() const
{
	std::vector<Vertex_PCU> verts;
	Rgba8 color(255, 255, 255);

	AddVertsForDisc2D(verts, m_playerPoint, 5.f, color);
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->DrawVertexArray(verts);
}


