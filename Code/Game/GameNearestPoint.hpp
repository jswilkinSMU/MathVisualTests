#pragma once
#include "Game/Game.h"
#include "Game/GameCommon.h"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/AABB2.h"
#include "Engine/Math/OBB2.hpp"
#include "Engine/Core/Rgba8.h"
// -----------------------------------------------------------------------------
class App;
class BitmapFont;
// -----------------------------------------------------------------------------
class GameNearestPoint : public Game 
{
public:
	GameNearestPoint(App* owner);
	~GameNearestPoint();

	void Update(float deltaSeconds) override;
	void PlayerMovement(float deltaSeconds);

	void Render() const override;
	void GameModeAndControlsText() const;
	void RandomShapes();

	void GetNearestPointCheck();
	void GetClosestPointToPlayer();

public:
	void RenderDisc() const;
	void RenderAABB2() const;
	void RenderOBB2() const;
	void RenderCapsule() const;
	void RenderTriangle() const;
	void RenderLineSegment() const;
	void RenderInfiniteLine() const;
	void RenderPlayerPoint() const;

	void RenderNearestPoint(Vec2 const& point, Rgba8 color) const;
	void LineToPoint(Vec2 const& point) const;

private:
	App* m_theApp;
	Vec2 m_playerPoint;
	bool m_isSlowMo;

private:
	// ToDO Maybe: Refactor these member vars to separate child classes
	Vec2 m_discCenter = Vec2::ZERO; 
	float m_discRadius = 0.f;

	AABB2 m_alignedBox;
	OBB2 m_orientedBox;

	Vec2 m_boneStart = Vec2::ZERO;
	Vec2 m_boneEnd = Vec2::ZERO;
	float m_capsuleRadius = 0.f;

	Vec2 m_ccw0 = Vec2::ZERO;
	Vec2 m_ccw1 = Vec2::ZERO;
	Vec2 m_ccw2 = Vec2::ZERO;

	Vec2 m_start = Vec2::ZERO;
	Vec2 m_end = Vec2::ZERO;
	float m_thickness = 0.f;

	Vec2 m_infiniteStart = Vec2::ZERO;
	Vec2 m_infiniteEnd = Vec2::ZERO;
	float m_infiniteThickness = 0.f;

private:
	BitmapFont* m_font = nullptr;

	// ToDO Maybe: Refactor these member vars to separate child classes
	Vec2 m_closestPointToPlayer;
	Vec2 m_nearestDiscPoint;
	Vec2 m_nearestAABBPoint;
	Vec2 m_nearestOBBPoint;
	Vec2 m_nearestCapsulePoint;
	Vec2 m_nearestTrianglePoint;
	Vec2 m_nearestLineSegmentPoint;
	Vec2 m_nearestInfiniteLinePoint;
	AABB2 m_gameSceneCoords;
};