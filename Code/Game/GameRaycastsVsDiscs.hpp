#pragma once
#include "Game/Game.h"
#include "Game/GameCommon.h"
#include "Engine/Math/AABB2.h"
#include <vector>
// -----------------------------------------------------------------------------
class BitmapFont;
// -----------------------------------------------------------------------------
struct Disc
{
	Vec2 m_discCenter = Vec2::ZERO;
	float m_discRadius = 0.0f;
};
// -----------------------------------------------------------------------------
class GameRaycastVsDiscs : public Game
{
public:
	GameRaycastVsDiscs(App* owner);

	void Update(float deltaSeconds) override;
	void Render() const override;
	void ArrowMovement();

private:
	void RandomizeDiscs();
	void DrawDiscs() const;
	void DrawRaycast() const;
	void GameModeAndControlsText() const;

private:
	App* m_theApp;
	BitmapFont* m_font = nullptr;
	static const int NUM_DISCS = 10;
	Vec2 m_rayCastStart = Vec2::ZERO;
	Vec2 m_rayCastEnd = Vec2::ZERO;
	AABB2 m_gameSceneCoords;
	std::vector<Disc> m_discs;
};