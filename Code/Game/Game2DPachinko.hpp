#pragma once
#include "Game/Game.h"
#include "Engine/Math/AABB2.h"
#include "Engine/Math/OBB2.hpp"
#include "Engine/Core/Vertex_PCU.h"
#include <vector>
// -----------------------------------------------------------------------------
class BitmapFont;
class Timer;
// -----------------------------------------------------------------------------
struct Shapes
{
	Vec2 m_discCenter = Vec2::ZERO;
	float m_discRadius = 0.0f;

	Vec2 m_capsuleboneStart = Vec2::ZERO;
	Vec2 m_capsuleboneEnd = Vec2::ZERO;
	float m_capsuleRadius = 0.f;

	OBB2 m_orientedBox;

	float m_fixedShapeElasticity = 0.9f;
	Rgba8 m_fixedShapeColor = Rgba8::WHITE;
};
struct Balls
{
	Vec2 m_discCenter = Vec2::ZERO;
	float m_discRadius = 0.0f;

	Vec2 m_velocity = Vec2::ZERO;

	float m_ballElasticity = 0.9f;
	Rgba8 m_ballColor = Rgba8::WHITE;
};
class Game2DPachinko : public Game
{
public:
	Game2DPachinko(App* owner);

	void InitializeGameConfigElements();

	void Update(float deltaSeconds) override;

	void AdjustTimeStep();

	// Physics checks
	void UpdatePhysics(float deltaSeconds);
	void ApplyGravityAndMoveBalls(float deltaSeconds);
	void BallsVsBalls();
	void BallsVsBumpers();
	void BallsVsWalls();

	void CheckEastAndWestWalls();
	void CheckNorthAndSouthWalls();
	void BallSpawning();
	void AdjustBallElasticity();

	void ArrowMovement();
	void RandomizeFixedShapes();
	void SpawnBalls();

	void Render() const override;
	void GamemodeAndControlsText() const;
	void DrawShapes() const;
// -----------------------------------------------------------------------------
private:
	App* m_theApp = nullptr;
	BitmapFont* m_font = nullptr;
	AABB2 m_gameSceneCoords;
	std::vector<Shapes> m_shapes;
	std::vector<Balls>  m_balls;
	bool m_isBottomWarpOn = true;

	// Time
	Timer* m_physicsTimer = nullptr;
	float  m_timeStepAmount = 0.0f;
	bool   m_isFixedTimeStep = false;

	// Shapes
	Vec2 m_rayCastStart = Vec2::ZERO;
	Vec2 m_rayCastEnd = Vec2::ZERO;

	float  m_pachinkoMinBallRadius = 0.f;
	float  m_pachinkoMaxBallRadius = 0.f;

	int    m_numFixedDiscs = 0;
	float  m_fixedDiscMinRadius = 0.f;
	float  m_fixedDiscMaxRadius = 0.f;

	int    m_numFixedCapsules = 0;
	float  m_minCapsuleLength = 0.f;
	float  m_maxCapsuleLength = 0.f;
	float  m_minCapsuleRadius = 0.f;
	float  m_maxCapsuleRadius = 0.f;

	int    m_numFixedOBB2s = 0;
	float  m_minOBBwidth = 0.0f;
	float  m_maxOBBwidth = 0.0f;

	// Elasticity
	float m_wallElasticity = 0.0f;
	float m_minElasticity = 0.0f;
	float m_maxElasticity = 0.0f;
	int   m_extraWarpHeight = 0;
};