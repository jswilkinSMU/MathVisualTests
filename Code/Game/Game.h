#pragma once
#include "Game/GameCommon.h"

//--------------------------------------------------------------------
enum GameMode
{
	GAME_MODE_NEAREST_POINT, 
	GAME_MODE_RAYCAST_VS_DISCS,
	GAME_MODE_RAYCAST_VS_LINESEGMENTS,
	GAME_MODE_RAYCAST_VS_AABB2S,
	GAME_MODE_3D_SHAPES_AND_QUERIES,
	GAME_MODE_2D_CURVES,
	GAME_MODE_2D_PACHINKO,
	GAME_MODE_COUNT
};
//--------------------------------------------------------------------
class Game
{
public:
	virtual void Update(float deltaSeconds) = 0;
	virtual void Render() const = 0;

	virtual void AdjustForPauseAndTimeDistortion(float& ds);

private:
	bool m_isSlowMo = false;
};