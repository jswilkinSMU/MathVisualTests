#include "Game/Game2DPachinko.hpp"
#include "Game/App.h"
#include "Engine/Core/EngineCommon.h"
#include "Engine/Core/Timer.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Input/InputSystem.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Math/MathUtils.h"

Game2DPachinko::Game2DPachinko(App* owner)
	:m_theApp(owner)
{
	m_font = g_theRenderer->CreateOrGetBitmapFont("Data/Fonts/SquirrelFixedFont");
	m_gameSceneCoords = AABB2(Vec2::ZERO, Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y));

	m_rayCastStart = Vec2(SCREEN_CENTER_X, SCREEN_CENTER_Y);
	m_rayCastEnd = Vec2(900.f, 300.f);

	m_timeStepAmount = 0.005f;
	m_physicsTimer = new Timer(static_cast<double>(m_timeStepAmount), m_theApp->m_gameClock);
	m_physicsTimer->m_startTime = m_theApp->m_gameClock->GetTotalSeconds();

	InitializeGameConfigElements();
	RandomizeFixedShapes();
}

void Game2DPachinko::InitializeGameConfigElements()
{
	// Balls
	m_pachinkoMinBallRadius = g_gameConfigBlackboard.GetValue("pachinkoMinBallRadius", 0.0f);
	m_pachinkoMaxBallRadius = g_gameConfigBlackboard.GetValue("pachinkoMaxBallRadius", 0.0f);

	// Discs
	m_numFixedDiscs = g_gameConfigBlackboard.GetValue("pachinkoNumDiscBumpers", 0);
	m_fixedDiscMinRadius = g_gameConfigBlackboard.GetValue("pachinkoMinDiscBumperRadius", 0.0f);
	m_fixedDiscMaxRadius = g_gameConfigBlackboard.GetValue("pachinkoMaxDiscBumperRadius", 0.0f);

	// Capsules
	m_numFixedCapsules = g_gameConfigBlackboard.GetValue("pachinkoNumCapsuleBumpers", 0);
	m_minCapsuleLength = g_gameConfigBlackboard.GetValue("pachinkoMinCapsuleBumperLength", 0.0f);
	m_maxCapsuleLength = g_gameConfigBlackboard.GetValue("pachinkoMaxCapsuleBumperLength", 0.0f);
	m_minCapsuleRadius = g_gameConfigBlackboard.GetValue("pachinkoMinCapsuleBumperRadius", 0.0f);
	m_maxCapsuleRadius = g_gameConfigBlackboard.GetValue("pachinkoMaxCapsuleBumperRadius", 0.0f);

	// Obb2s
	m_numFixedOBB2s = g_gameConfigBlackboard.GetValue("pachinkoNumObbBumpers", 0);
	m_minOBBwidth = g_gameConfigBlackboard.GetValue("pachinkoMinObbBumperWidth", 0.0f);
	m_maxOBBwidth = g_gameConfigBlackboard.GetValue("pachinkoMaxObbBumperWidth", 0.0f);

	// Elasticity
	m_wallElasticity = g_gameConfigBlackboard.GetValue("pachinkoWallElasticity", 0.0f);
	m_minElasticity = g_gameConfigBlackboard.GetValue("pachinkoMinBumperElasticity", 0.0f);
	m_maxElasticity = g_gameConfigBlackboard.GetValue("pachinkoMaxBumperElasticity", 0.0f);
	m_extraWarpHeight = g_gameConfigBlackboard.GetValue("pachinkoExtraWarpHeight", 0);
}

void Game2DPachinko::Update(float deltaSeconds)
{
	AdjustForPauseAndTimeDistortion(deltaSeconds);
	ArrowMovement();

	if (g_theInput->WasKeyJustPressed(KEYCODE_F8))
	{
		RandomizeFixedShapes();
	}

	AdjustBallElasticity();
	BallSpawning();

	if (g_theInput->WasKeyJustPressed('B'))
	{
		m_isBottomWarpOn = !m_isBottomWarpOn;
	}

	if (g_theInput->WasKeyJustPressed('P'))
	{
		m_isFixedTimeStep = !m_isFixedTimeStep;
	}

	if (m_isFixedTimeStep)
	{
		AdjustTimeStep();

		while (m_physicsTimer->DecrementPeriodIfElapsed())
		{
			UpdatePhysics(m_timeStepAmount);
		}
	}
	else
	{
		UpdatePhysics(deltaSeconds);
	}
}

void Game2DPachinko::AdjustTimeStep()
{
	if (g_theInput->WasKeyJustPressed(KEYCODE_RIGHTBRACKET))
	{
		m_timeStepAmount *= 1.1f;
	}
	else if (g_theInput->WasKeyJustPressed(KEYCODE_LEFTBRACKET))
	{
		m_timeStepAmount /= 1.1f;
	}
}

void Game2DPachinko::UpdatePhysics(float deltaSeconds)
{
	ApplyGravityAndMoveBalls(deltaSeconds);
	BallsVsBalls();
	BallsVsBumpers();
	BallsVsWalls();
}

void Game2DPachinko::ApplyGravityAndMoveBalls(float deltaSeconds)
{
	Vec2 downwardAcceleration = Vec2(0.f, -1.f) * 100.f;

	for (int ballIndex = 0; ballIndex < static_cast<int>(m_balls.size()); ++ballIndex)
	{
		m_balls[ballIndex].m_velocity += downwardAcceleration * deltaSeconds;
		m_balls[ballIndex].m_discCenter += m_balls[ballIndex].m_velocity * deltaSeconds;
	}
}

void Game2DPachinko::BallsVsBalls()
{
	for (int i = 0; i < static_cast<int>(m_balls.size()) - 1; ++i)
	{
		for (int j = i + 1; j < static_cast<int>(m_balls.size()); ++j)
		{
			BounceDiscsOffEachOther2D(m_balls[i].m_discCenter, m_balls[j].m_discCenter, m_balls[i].m_discRadius,     m_balls[j].m_discRadius,
									  m_balls[i].m_velocity,   m_balls[j].m_velocity,   m_balls[i].m_ballElasticity, m_balls[j].m_ballElasticity);
		}
	}
}

void Game2DPachinko::BallsVsBumpers()
{
	for (int ballIndex = 0; ballIndex < static_cast<int>(m_balls.size()); ++ballIndex)
	{
		Balls& ball = m_balls[ballIndex];
		for (int shapeIndex = 0; shapeIndex < static_cast<int>(m_shapes.size()); ++shapeIndex)
		{
			Shapes& shape = m_shapes[shapeIndex];

			// Check fixed disc
			BounceDiscOffFixedDisc2D(ball.m_discCenter, ball.m_discRadius, ball.m_velocity, ball.m_ballElasticity, shape.m_discCenter, shape.m_discRadius, shape.m_fixedShapeElasticity);

			// Check Capsule
			Vec2 nearestPointOnCapsule = GetNearestPointOnCapsule2D(ball.m_discCenter, shape.m_capsuleboneStart, shape.m_capsuleboneEnd, shape.m_capsuleRadius);
			float radiusSum = ball.m_discRadius + shape.m_capsuleRadius;
			float radiusSumSquared = radiusSum * radiusSum;
			float distanceSquaredCapsule = GetDistanceSquared2D(ball.m_discCenter, nearestPointOnCapsule);

			if (distanceSquaredCapsule < radiusSumSquared)
			{
				BounceDiscOffFixedPoint(ball.m_discCenter, ball.m_discRadius, ball.m_velocity, ball.m_ballElasticity, nearestPointOnCapsule, shape.m_fixedShapeElasticity);
			}

			//// Check OBB2
			Vec2 nearestPointOnOBB = GetNearestPointOnOBB2D(ball.m_discCenter, shape.m_orientedBox);
			float distanceSquared = GetDistanceSquared2D(ball.m_discCenter, nearestPointOnOBB);
			float ballRadiusSquared = ball.m_discRadius * ball.m_discRadius;

			if (distanceSquared < ballRadiusSquared)
			{
				BounceDiscOffFixedPoint(ball.m_discCenter, ball.m_discRadius, ball.m_velocity, ball.m_ballElasticity, nearestPointOnOBB, shape.m_fixedShapeElasticity);
			}
		}
	}
}

void Game2DPachinko::BallsVsWalls()
{
	CheckNorthAndSouthWalls();
	CheckEastAndWestWalls();
}

void Game2DPachinko::CheckEastAndWestWalls()
{
	for (int ballIndex = 0; ballIndex < static_cast<int>(m_balls.size()); ++ballIndex)
	{
		Balls& ball = m_balls[ballIndex];
		if (ball.m_discCenter.x < ball.m_discRadius)
		{
			BounceDiscOffFixedPoint(ball.m_discCenter, ball.m_discRadius, ball.m_velocity, ball.m_ballElasticity, Vec2(0.f, ball.m_discCenter.y), m_wallElasticity);
		}
		else if (ball.m_discCenter.x > ball.m_discRadius)
		{
			BounceDiscOffFixedPoint(ball.m_discCenter, ball.m_discRadius, ball.m_velocity, ball.m_ballElasticity, Vec2(SCREEN_SIZE_X, ball.m_discCenter.y), m_wallElasticity);
		}
	}
}

void Game2DPachinko::CheckNorthAndSouthWalls()
{
	for (int ballIndex = 0; ballIndex < static_cast<int>(m_balls.size()); ++ballIndex)
	{
		Balls& ball = m_balls[ballIndex];
		if (!m_isBottomWarpOn && ball.m_discCenter.y < ball.m_discRadius)
		{
			BounceDiscOffFixedPoint(ball.m_discCenter, ball.m_discRadius, ball.m_velocity, ball.m_ballElasticity, Vec2(ball.m_discCenter.x, 0.f), m_wallElasticity);
		}
		else if (m_isBottomWarpOn && ball.m_discCenter.y < -ball.m_discRadius)
		{
			ball.m_discCenter.y = SCREEN_SIZE_Y + ball.m_discRadius + m_extraWarpHeight;
		}
	}
}

void Game2DPachinko::BallSpawning()
{
	if (g_theInput->WasKeyJustPressed(' '))
	{
		SpawnBalls();
	}
	if (g_theInput->IsKeyDown('N'))
	{
		SpawnBalls();
	}
}

void Game2DPachinko::AdjustBallElasticity()
{
	for (int ballIndex = 0; ballIndex < static_cast<int>(m_balls.size()); ++ballIndex)
	{
		m_balls[ballIndex].m_ballElasticity = GetClamped(m_balls[ballIndex].m_ballElasticity, 0.f, 1.f);
		if (g_theInput->WasKeyJustPressed('G'))
		{
			m_balls[ballIndex].m_ballElasticity -= 0.05f;
		}
		if (g_theInput->WasKeyJustPressed('H'))
		{
			m_balls[ballIndex].m_ballElasticity += 0.05f;
		}
	}
}

void Game2DPachinko::ArrowMovement()
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

void Game2DPachinko::RandomizeFixedShapes()
{
	m_shapes.clear();
	m_balls.clear();

	// Randomizing Discs
	for (int discsIndex = 0; discsIndex < m_numFixedDiscs; ++discsIndex)
	{
		Shapes newDiscs;
		newDiscs.m_discCenter = Vec2(g_rng->RollRandomFloatInRange(0.f, SCREEN_SIZE_X), g_rng->RollRandomFloatInRange(0.f, SCREEN_SIZE_Y));
		newDiscs.m_discRadius = g_rng->RollRandomFloatInRange(m_fixedDiscMinRadius, m_fixedDiscMaxRadius);

		newDiscs.m_fixedShapeElasticity = g_rng->RollRandomFloatInRange(m_minElasticity, m_maxElasticity);
		float elasticityAmount = RangeMap(newDiscs.m_fixedShapeElasticity, m_minElasticity, m_maxElasticity, 0.f, 1.f);
		newDiscs.m_fixedShapeColor = newDiscs.m_fixedShapeColor.Rgba8Interpolate(Rgba8::RED, Rgba8::GREEN, elasticityAmount);

		m_shapes.push_back(newDiscs);
	}

	// Randomizing Capsules
	for (int capsulesIndex = 0; capsulesIndex < m_numFixedCapsules; ++capsulesIndex)
	{
		Shapes newCapsules;
		Vec2 center = Vec2(g_rng->RollRandomFloatInRange(50.f, SCREEN_SIZE_X - 50.f),
			g_rng->RollRandomFloatInRange(50.f, SCREEN_SIZE_Y - 50.f));

		float angleDegrees = g_rng->RollRandomFloatInRange(0.f, 360.f);
		float length = g_rng->RollRandomFloatInRange(m_minCapsuleLength, m_maxCapsuleLength);
		float halfLength = length * 0.5f;
		Vec2 direction = Vec2::MakeFromPolarDegrees(angleDegrees);

		newCapsules.m_capsuleboneStart = center - direction * halfLength;
		newCapsules.m_capsuleboneEnd = center + direction * halfLength;
		newCapsules.m_capsuleRadius = g_rng->RollRandomFloatInRange(m_minCapsuleRadius, m_maxCapsuleRadius);

		newCapsules.m_fixedShapeElasticity = g_rng->RollRandomFloatInRange(m_minElasticity, m_maxElasticity);
		float elasticityAmount = RangeMap(newCapsules.m_fixedShapeElasticity, m_minElasticity, m_maxElasticity, 0.f, 1.f);
		newCapsules.m_fixedShapeColor = newCapsules.m_fixedShapeColor.Rgba8Interpolate(Rgba8::RED, Rgba8::GREEN, elasticityAmount);

		m_shapes.push_back(newCapsules);
	}

	// Randomizing OBB2s
	for (int obbIndex = 0; obbIndex < m_numFixedOBB2s; ++obbIndex)
	{
		Shapes newOBBs;

		float halfWidth = g_rng->RollRandomFloatInRange(m_minOBBwidth, m_maxOBBwidth);
		float halfHeight = g_rng->RollRandomFloatInRange(m_minOBBwidth, m_maxOBBwidth);
		Vec2 boxCenter(g_rng->RollRandomFloatInRange(50.f + halfWidth, SCREEN_SIZE_X - 50.f - halfWidth), g_rng->RollRandomFloatInRange(50.f + halfHeight, SCREEN_SIZE_Y - 50.f - halfHeight));
		float angle = g_rng->RollRandomFloatInRange(0.f, 360.f);
		Vec2 iBasisNormal(CosDegrees(angle), SinDegrees(angle));
		newOBBs.m_orientedBox = OBB2(boxCenter, iBasisNormal, Vec2(halfWidth, halfHeight));

		newOBBs.m_fixedShapeElasticity = g_rng->RollRandomFloatInRange(m_minElasticity, m_maxElasticity);
		float elasticityAmount = RangeMap(newOBBs.m_fixedShapeElasticity, m_minElasticity, m_maxElasticity, 0.f, 1.f);
		newOBBs.m_fixedShapeColor = newOBBs.m_fixedShapeColor.Rgba8Interpolate(Rgba8::RED, Rgba8::GREEN, elasticityAmount);

		m_shapes.push_back(newOBBs);
	}
}

void Game2DPachinko::SpawnBalls()
{
	Balls* balls = new Balls();
	balls->m_discCenter = m_rayCastStart;
	balls->m_discRadius = g_rng->RollRandomFloatInRange(m_pachinkoMinBallRadius, m_pachinkoMaxBallRadius);
	balls->m_velocity = m_rayCastEnd - m_rayCastStart;

	float colorFraction = g_rng->RollRandomFloatInRange(0.f, 0.9f);
	balls->m_ballColor = balls->m_ballColor.Rgba8Interpolate(Rgba8::BLUE, Rgba8::WHITE, colorFraction);

	m_balls.push_back(*balls);
}

void Game2DPachinko::Render() const
{
	g_theRenderer->BeginCamera(m_theApp->m_screenCamera);
	DrawShapes();
	GamemodeAndControlsText();
}

void Game2DPachinko::GamemodeAndControlsText() const
{
	std::vector<Vertex_PCU> textVerts;
	Balls ball;

	std::string controlText = "F8 to Reset; LMB/RMB/ESDF/IJKL to Move; Hold T for slow; space/N = ball (" + std::to_string(m_balls.size()) + ");";
	std::string timeText = Stringf("Timestep = (%0.3f) (P, [,]),", m_timeStepAmount);
	std::string frameRateText = Stringf("dt = %.4f,", m_theApp->m_gameClock->GetDeltaSeconds());
	std::string fpsText = Stringf("FPS = %.2f", m_theApp->m_gameClock->GetFrameRate());
	std::string ballText;
	if (!m_balls.empty())
	{
		ballText = Stringf("Ball elasticity = %0.02f", m_balls[0].m_ballElasticity);
	}

	m_font->AddVertsForTextInBox2D(textVerts, "Mode (F6/F7 for Prev/Next): Pachinko Machine (2D)", m_gameSceneCoords, 15.f, Rgba8::GOLD, 0.8f, Vec2(0.f, 0.97f));
	m_font->AddVertsForTextInBox2D(textVerts, controlText, m_gameSceneCoords, 15.f, Rgba8::ALICEBLUE, 1.f, Vec2(0.f, 0.945f));
	m_font->AddVertsForTextInBox2D(textVerts, frameRateText, m_gameSceneCoords, 15.f, Rgba8::ALICEBLUE, 1.f, Vec2(0.375f, 0.925f));
	m_font->AddVertsForTextInBox2D(textVerts, fpsText, m_gameSceneCoords, 15.f, Rgba8::ALICEBLUE, 1.f, Vec2(0.515f, 0.925f));
	m_font->AddVertsForTextInBox2D(textVerts, ballText, m_gameSceneCoords, 15.f, Rgba8::ALICEBLUE, 1.f, Vec2(0.f, 0.9f));

	if (m_isFixedTimeStep)
	{
		m_font->AddVertsForTextInBox2D(textVerts, timeText, m_gameSceneCoords, 15.f, Rgba8::ALICEBLUE, 1.f, Vec2(0.f, 0.925f));
	}
	if (!m_isFixedTimeStep)
	{
		m_font->AddVertsForTextInBox2D(textVerts, "variable timestep (P)", m_gameSceneCoords, 15.f, Rgba8::ALICEBLUE, 1.f, Vec2(0.f, 0.925f));
	}

	if (m_isBottomWarpOn)
	{
		m_font->AddVertsForTextInBox2D(textVerts, "B=bottom warp on", m_gameSceneCoords, 15.f, Rgba8::ALICEBLUE, 1.f, Vec2(0.875f, 0.945f));
	}
	if (!m_isBottomWarpOn)
	{
		m_font->AddVertsForTextInBox2D(textVerts, "B=bottom warp off", m_gameSceneCoords, 15.f, Rgba8::ALICEBLUE, 1.f, Vec2(0.875f, 0.945f));
	}

	g_theRenderer->BindTexture(&m_font->GetTexture());
	g_theRenderer->DrawVertexArray(textVerts);
}

void Game2DPachinko::DrawShapes() const
{
	std::vector<Vertex_PCU> shapeVerts;

	// Draw arrow
	AddVertsForArrow2D(shapeVerts, m_rayCastStart, m_rayCastEnd, 20.f, 2.f, Rgba8::LIMEGREEN);


	// Draw fixed discs
	for (int discIndex = 0; discIndex < static_cast<int>(m_shapes.size()); ++discIndex)
	{
		Shapes const& shape = m_shapes[discIndex];
		AddVertsForDisc2D(shapeVerts, shape.m_discCenter, shape.m_discRadius, shape.m_fixedShapeColor);
	}

	// Draw fixed capsules
	for (int capsuleIndex = 0; capsuleIndex < static_cast<int>(m_shapes.size()); ++capsuleIndex)
	{
		Shapes const& shape = m_shapes[capsuleIndex];
		AddVertsForCapsule2D(shapeVerts, shape.m_capsuleboneStart, shape.m_capsuleboneEnd, shape.m_capsuleRadius, shape.m_fixedShapeColor);
	}

	// Draw fixed OBBs
	for (int obbIndex = 0; obbIndex < static_cast<int>(m_shapes.size()); ++obbIndex)
	{
		Shapes const& shape = m_shapes[obbIndex];
		AddVertsForOBB2D(shapeVerts, shape.m_orientedBox, shape.m_fixedShapeColor);
	}

	// Draw BALLS
	for (int ballIndex = 0; ballIndex < static_cast<int>(m_balls.size()); ++ballIndex)
	{
		Balls const& ball = m_balls[ballIndex];
		AddVertsForDisc2D(shapeVerts, ball.m_discCenter, ball.m_discRadius, ball.m_ballColor);
	}

	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
	g_theRenderer->SetDepthMode(DepthMode::DISABLED);
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->DrawVertexArray(shapeVerts);

	// Drawing rings around ray start
	DebugDrawRing(m_rayCastStart, m_pachinkoMinBallRadius, 1.f, Rgba8::SAPPHIRE);
	DebugDrawRing(m_rayCastStart, m_pachinkoMaxBallRadius, 1.f, Rgba8::SAPPHIRE);
}
