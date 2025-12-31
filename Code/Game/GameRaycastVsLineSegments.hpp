#pragma once
#include "Game/Game.h"
#include "Engine/Math/AABB2.h"
#include <vector>
// -----------------------------------------------------------------------------
class BitmapFont;
// -----------------------------------------------------------------------------
struct LineSegment
{
	Vec2 m_lineStart = Vec2::ZERO;
	Vec2 m_lineEnd = Vec2::ZERO;
};
// -----------------------------------------------------------------------------
static const int NUM_LINES = 10;
const float LINE_MAX_LENGTH = 200.f;
// -----------------------------------------------------------------------------
class GameRaycastVsLinesegments : public Game
{
public:
	GameRaycastVsLinesegments(App* owner);

	void Update(float deltaSeconds) override;
	void Render() const override;

private:
	void RandomizeLineSegments();
	void ArrowMovement();

	void GameModeAndControlsText() const;
	void DrawLineSegments() const;
	void DrawRaycast() const;

private:
	App* m_theApp = nullptr;
	BitmapFont* m_font = nullptr;
	Vec2 m_rayCastStart = Vec2::ZERO;
	Vec2 m_rayCastEnd = Vec2::ZERO;
	AABB2 m_gameSceneCoords;

	std::vector<LineSegment> m_lineSegments;
};