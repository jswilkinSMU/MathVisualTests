#pragma once
#include "Game/Game.h"
#include "Engine/Math/AABB2.h"
#include "Engine/Math/MathUtils.h"
#include "Engine/Math/CubicBezierCurve2D.hpp"
#include "Engine/Math/Splines.hpp"
#include "Engine/Core/Vertex_PCU.h"
#include <vector>
// -----------------------------------------------------------------------------
class BitmapFont;
// -----------------------------------------------------------------------------
typedef float (*EasingFunctionEntry)(float easingAmount);
// -----------------------------------------------------------------------------
struct EasingFunctionPtr
{
	const char* m_name;
	EasingFunctionEntry g_easingFunctions;
};
// -----------------------------------------------------------------------------
extern EasingFunctionPtr g_easingFunctions[];
// -----------------------------------------------------------------------------
float CustomFunkyFunction(float easeAmount);
// -----------------------------------------------------------------------------
class Game2DCurves : public Game
{
public:
	Game2DCurves(App* owner);

	void Update(float deltaSeconds) override;

	void Render() const override;
	void GamemodeAndControlsText() const;

	void RandomizeCurves();
	void InitializePanes();

private:
	void AddVertsForEasingCurves(std::vector<Vertex_PCU>& shapeVerts, std::vector<Vertex_PCU>& textVerts, AABB2 box) const;
	void AddVertsForBezierCurves(std::vector<Vertex_PCU>& shapeVerts) const;
	void AddVertsForSplineCurves(std::vector<Vertex_PCU>& shapeVerts) const;

	void NextAndPreviousInputs();
	void GetNextEasingFunction();
	void GetPreviousEasingFunction();
	void IncreaseDecreaseSubdivisions();

private:
	App* m_theApp = nullptr;
	BitmapFont* m_font = nullptr;
	int m_numSubdivisions = 2;
	AABB2 m_gameSceneCoords;

	// Panes
	std::vector<Vertex_PCU> m_paneVerts;
	AABB2 m_testPane;
	AABB2 m_easingPane;
	AABB2 m_easingBox;
	AABB2 m_cubicBezierPane;
	AABB2 m_splinesPane;
	bool  m_arePanesShowing = false;

	// Curves
	EasingFunctionPtr* m_easing = nullptr;
	int m_currentEasingFunction = 0;
	CubicBezierCurve2D m_currentBezier;
	Spline m_currentSpline;
};