#pragma once
#include "Game/Game.h"
#include "Engine/Math/AABB2.h"
#include <vector>
// -----------------------------------------------------------------------------
class BitmapFont;
// -----------------------------------------------------------------------------
const int   NUM_AABB2S = 10;
const float AABB2_MIN_SIZE = 20.f;
const float AABB2_MAX_SIZE = 200.f;
// -----------------------------------------------------------------------------
class GameRaycastVsAABB2s : public Game
{
public:
	GameRaycastVsAABB2s(App* owner);

	void Update(float deltaSeconds) override;
	void Render() const override;

private:
	void DrawAABB2s() const;
	void DrawRaycast() const;
	void GameModeAndControlsText() const;

	void RandomizeAABB2s();
	void ArrowMovement();

private:
	App* m_theApp;
	BitmapFont* m_font = nullptr;
	Vec2 m_rayCastStart = Vec2::ZERO;
	Vec2 m_rayCastEnd = Vec2::ZERO;
	AABB2 m_gameSceneCoords;
	std::vector<AABB2> m_AABB2s;
};