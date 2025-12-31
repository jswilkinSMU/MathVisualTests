#include "Game/Game2DCurves.hpp"
#include "Game/App.h"
#include "Engine/Input/InputSystem.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Core/EngineCommon.h"
#include "Engine/Math/MathUtils.cpp"
// -----------------------------------------------------------------------------
EasingFunctionPtr g_easingFunctions[] =
{
	{"SmoothStart2", SmoothStart2},
	{"SmoothStart3", SmoothStart3},
	{"SmoothStart4", SmoothStart4},
	{"SmoothStart5", SmoothStart5},
	{"SmoothStart6", SmoothStart6},
	{"SmoothStop2",  SmoothStop2},
	{"SmoothStop3",  SmoothStop3},
	{"SmoothStop4",  SmoothStop4},
	{"SmoothStop5",  SmoothStop5},
	{"SmoothStop6",  SmoothStop6},
	{"SmoothStep3",  SmoothStep3},
	{"SmoothStep5",  SmoothStep5},
	{"Hesitate3",    Hesitate3},
	{"Hesitate5",    Hesitate5},
	{"CustomFunky",  CustomFunkyFunction}
};
constexpr int NUM_EASING_FUNCS = sizeof(g_easingFunctions) / sizeof(g_easingFunctions[0]);
constexpr int HIGH_SUBDIVISIONS = 64;
// -----------------------------------------------------------------------------
Game2DCurves::Game2DCurves(App* owner)
	:m_theApp(owner)
{
	m_font = g_theRenderer->CreateOrGetBitmapFont("Data/Fonts/SquirrelFixedFont");
	m_easing = &g_easingFunctions[m_currentEasingFunction];

	InitializePanes();
	RandomizeCurves();
}

void Game2DCurves::RandomizeCurves()
{
	// Easing randomization
	m_currentEasingFunction = g_rng->RollRandomIntInRange(0, NUM_EASING_FUNCS - 1);
	m_easing = &g_easingFunctions[m_currentEasingFunction];

	// Bezier randomization
	float minX = m_cubicBezierPane.m_mins.x;
	float maxX = m_cubicBezierPane.m_maxs.x;
	float minY = m_cubicBezierPane.m_mins.y;
	float maxY = m_cubicBezierPane.m_maxs.y;
	Vec2 randomAPoint = Vec2(g_rng->RollRandomFloatInRange(minX, maxX), g_rng->RollRandomFloatInRange(minY, maxY));
	Vec2 randomBPoint = Vec2(g_rng->RollRandomFloatInRange(minX, maxX), g_rng->RollRandomFloatInRange(minY, maxY));
	Vec2 randomCPoint = Vec2(g_rng->RollRandomFloatInRange(minX, maxX), g_rng->RollRandomFloatInRange(minY, maxY));
	Vec2 randomDPoint = Vec2(g_rng->RollRandomFloatInRange(minX, maxX), g_rng->RollRandomFloatInRange(minY, maxY));
	m_currentBezier = CubicBezierCurve2D(randomAPoint, randomBPoint, randomCPoint, randomDPoint);

	// Spline randomization
	std::vector<Vec2> randomSplinePoints;
	int controlPoints = g_rng->RollRandomIntInRange(4, 6);
	float minSplinePaneX = m_splinesPane.m_mins.x;
	float maxSplinePaneX = m_splinesPane.m_maxs.x;
	float minSplinesPaneY = m_splinesPane.m_mins.y;
	float maxSplinesPaneY = m_splinesPane.m_maxs.y;
	float stepX = (maxSplinePaneX - minSplinePaneX) / controlPoints - 1;
	for (int splinePointIndex = 0; splinePointIndex < controlPoints; ++splinePointIndex)
	{
		float xPosition = minSplinePaneX + splinePointIndex * stepX + g_rng->RollRandomFloatInRange(20.f, 200.f);
		float yPosition = g_rng->RollRandomFloatInRange(minSplinesPaneY, maxSplinesPaneY);
		randomSplinePoints.push_back(Vec2(xPosition, yPosition));
	}
	m_currentSpline = Spline(randomSplinePoints);
}

void Game2DCurves::InitializePanes()
{
	// Full screen box
	m_gameSceneCoords = AABB2(Vec2::ZERO, Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y));

	// Padded box for outer room for text
	m_testPane = m_gameSceneCoords;
	m_testPane.AddPadding(-10.f, -45.f);
	//AddVertsForAABB2D(m_paneVerts, m_testPane, Rgba8::MAGENTA);

	// Easing pane
	m_easingPane = m_testPane.GetBoxAtUVs(Vec2(0.f, 0.5f), Vec2(0.5f, 1.0f));
	m_easingPane.ChopOffBottom(0.1f, 0.f);
	m_easingPane.ChopOffRight(0.1f, 1.0f);
	AddVertsForAABB2D(m_paneVerts, m_easingPane, Rgba8::CRIMSON);

	m_easingBox = m_easingPane;
	m_easingBox.ReduceToNewAspect(1.f);
	m_easingBox.ChopOffBottom(0.1f, 0.f);

	// Cubic Bezier pane
	m_cubicBezierPane = m_testPane.GetBoxAtUVs(Vec2::ONEHALF, Vec2::ONE);
	m_cubicBezierPane.ChopOffBottom(0.1f, 0.f);
	m_cubicBezierPane.ChopOffLeft(0.1f, 1.f);
	AddVertsForAABB2D(m_paneVerts, m_cubicBezierPane, Rgba8::CRIMSON);

	// Splines pane
	m_splinesPane = m_testPane.GetBoxAtUVs(Vec2::ZERO, Vec2(1.0f, 0.5f));
	m_splinesPane.ChopOffTop(0.1f, 15.f);
	AddVertsForAABB2D(m_paneVerts, m_splinesPane, Rgba8::CRIMSON);
}

void Game2DCurves::AddVertsForEasingCurves(std::vector<Vertex_PCU>& shapeVerts, std::vector<Vertex_PCU>& textVerts, AABB2 box) const
{
	AddVertsForAABB2D(shapeVerts, box, Rgba8::SAPPHIRE);

	double time = fmod(m_theApp->m_gameClock->GetTotalSeconds(), 1.0);
	EasingFunctionEntry easing = m_easing->g_easingFunctions;
	const char* name = m_easing->m_name;
	float u = 1.f / static_cast<float>(HIGH_SUBDIVISIONS);

	for (int parametricIndex = 0; parametricIndex < HIGH_SUBDIVISIONS; ++parametricIndex)
	{
		float startParametric = parametricIndex * u;
		float endParametric = (parametricIndex + 1) * u;

		Vec2 minUV = Vec2(startParametric, easing(startParametric));
		Vec2 maxUV = Vec2(endParametric, easing(endParametric));

		Vec2 startPos = box.GetPointAtUV(minUV);
		Vec2 endPos = box.GetPointAtUV(maxUV);
		AddVertsForLineSegment2D(shapeVerts, startPos, endPos, 2.f, Rgba8::DARKGRAY);
	}

	u = 1.f / static_cast<float>(m_numSubdivisions);
	for (int parametricIndex = 0; parametricIndex < m_numSubdivisions; ++parametricIndex)
	{
		float startParametric = parametricIndex * u;
		float endParametric = (parametricIndex + 1) * u;

		Vec2 minUV = Vec2(startParametric, easing(startParametric));
		Vec2 maxUV = Vec2(endParametric, easing(endParametric));

		Vec2 startPos = box.GetPointAtUV(minUV);
		Vec2 endPos = box.GetPointAtUV(maxUV);
		AddVertsForLineSegment2D(shapeVerts, startPos, endPos, 2.f, Rgba8::LIMEGREEN);
	}

	float easedTime = easing(static_cast<float>(time));
	Vec2 pointPos = box.GetPointAtUV(Vec2(static_cast<float>(time), easedTime));

	AddVertsForLineSegment2D(shapeVerts, Vec2(box.m_mins.x, pointPos.y), pointPos, 1.f, Rgba8::BLUE);
	AddVertsForLineSegment2D(shapeVerts, Vec2(pointPos.x, box.m_mins.y), pointPos, 1.f, Rgba8::BLUE);
	AddVertsForDisc2D(shapeVerts, pointPos, 3.f, Rgba8::WHITE);

	AABB2 textBox = AABB2(box.m_mins, Vec2(box.m_maxs.x, box.m_mins.y + 40.f));
	m_font->AddVertsForText2D(textVerts, box.m_mins - Vec2(-75.f, 20.f), 15.f, name, Rgba8::LIMEGREEN);
}

void Game2DCurves::AddVertsForBezierCurves(std::vector<Vertex_PCU>& shapeVerts) const
{
	AddVertsForLineSegment2D(shapeVerts, m_currentBezier.m_positionA, m_currentBezier.m_positionB, 1.5f, Rgba8::SAPPHIRE);
	AddVertsForLineSegment2D(shapeVerts, m_currentBezier.m_positionB, m_currentBezier.m_positionC, 1.5f, Rgba8::SAPPHIRE);
	AddVertsForLineSegment2D(shapeVerts, m_currentBezier.m_positionC, m_currentBezier.m_positionD, 1.5f, Rgba8::SAPPHIRE);

	for (int paraIndex = 0; paraIndex < HIGH_SUBDIVISIONS; ++paraIndex)
	{
		float paraStart = static_cast<float>(paraIndex) / HIGH_SUBDIVISIONS;
		float paraEnd = static_cast<float>(paraIndex + 1) / HIGH_SUBDIVISIONS;
		Vec2 startPos = m_currentBezier.EvaluateAtParametric(paraStart);
		Vec2 endPos = m_currentBezier.EvaluateAtParametric(paraEnd);
		AddVertsForLineSegment2D(shapeVerts, startPos, endPos, 3.f, Rgba8::DARKGRAY);
	}

	for (int paraIndex = 0; paraIndex < m_numSubdivisions; ++paraIndex)
	{
		float paraStart = static_cast<float>(paraIndex) / m_numSubdivisions;
		float paraEnd = static_cast<float>(paraIndex + 1) / m_numSubdivisions;
		Vec2 startPos = m_currentBezier.EvaluateAtParametric(paraStart);
		Vec2 endPos = m_currentBezier.EvaluateAtParametric(paraEnd);
		AddVertsForLineSegment2D(shapeVerts, startPos, endPos, 3.f, Rgba8::LIMEGREEN);
	}

	AddVertsForDisc2D(shapeVerts, m_currentBezier.m_positionA, 5.f, Rgba8::SAPPHIRE);
	AddVertsForDisc2D(shapeVerts, m_currentBezier.m_positionB, 5.f, Rgba8::SAPPHIRE);
	AddVertsForDisc2D(shapeVerts, m_currentBezier.m_positionC, 5.f, Rgba8::SAPPHIRE);
	AddVertsForDisc2D(shapeVerts, m_currentBezier.m_positionD, 5.f, Rgba8::SAPPHIRE);

	double time = fmod(m_theApp->m_gameClock->GetTotalSeconds(), 1.0);
	Vec2 movingParametrically = m_currentBezier.EvaluateAtParametric(static_cast<float>(time));
	AddVertsForDisc2D(shapeVerts, movingParametrically, 5.f, Rgba8::WHITE);

	float curveLength = m_currentBezier.GetApproximateLength(m_numSubdivisions);
	float speed = curveLength / 1.f;
	float distanceAlongCurve = speed * static_cast<float>(time);
	Vec2  movingFixed = m_currentBezier.EvaluateApproximateDistance(distanceAlongCurve, m_numSubdivisions);
	AddVertsForDisc2D(shapeVerts, movingFixed, 5.f, Rgba8::LIMEGREEN);
}

void Game2DCurves::AddVertsForSplineCurves(std::vector<Vertex_PCU>& shapeVerts) const
{
	for (size_t splinePointIndex = 0; splinePointIndex < m_currentSpline.m_positions.size() - 1; ++splinePointIndex)
	{
		AddVertsForLineSegment2D(shapeVerts, m_currentSpline.m_positions[splinePointIndex], m_currentSpline.m_positions[splinePointIndex + 1], 1.f, Rgba8::SAPPHIRE);
	}

	for (int paraIndex = 0; paraIndex < HIGH_SUBDIVISIONS; ++paraIndex)
	{
		float paraStart = static_cast<float>(paraIndex) / HIGH_SUBDIVISIONS;
		float paraEnd = static_cast<float>(paraIndex + 1) / HIGH_SUBDIVISIONS;
		Vec2 startPos = m_currentSpline.EvaluateAtParametric(paraStart);
		Vec2 endPos = m_currentSpline.EvaluateAtParametric(paraEnd);
		AddVertsForLineSegment2D(shapeVerts, startPos, endPos, 3.f, Rgba8::DARKGRAY);
	}

	for (int paraIndex = 0; paraIndex < m_numSubdivisions; ++paraIndex)
	{
		float paraStart = static_cast<float>(paraIndex) / m_numSubdivisions;
		float paraEnd = static_cast<float>(paraIndex + 1) / m_numSubdivisions;
		Vec2 startPos = m_currentSpline.EvaluateAtParametric(paraStart);
		Vec2 endPos = m_currentSpline.EvaluateAtParametric(paraEnd);
		AddVertsForLineSegment2D(shapeVerts, startPos, endPos, 3.f, Rgba8::LIMEGREEN);
	}

	for (int splinePointIndex = 1; splinePointIndex < static_cast<int>(m_currentSpline.m_positions.size()) - 1; ++splinePointIndex)
	{
		Vec2 velocityAtPosition = m_currentSpline.m_velocites[splinePointIndex];
		Vec2 startPos = m_currentSpline.m_positions[splinePointIndex];

		if (velocityAtPosition != Vec2::ZERO)
		{
			Vec2 endPos = startPos + velocityAtPosition * 0.3f;
			AddVertsForArrow2D(shapeVerts, startPos, endPos, 15.f, 3.f, Rgba8::RED);
		}
	}

	for (int splinePointIndex = 0; splinePointIndex < static_cast<int>(m_currentSpline.m_positions.size()); ++splinePointIndex)
	{
		AddVertsForDisc2D(shapeVerts, m_currentSpline.m_positions[splinePointIndex], 5.f, Rgba8::SAPPHIRE);
	}

	// Moving white point parametrically
	int numCurveSections = static_cast<int>(m_currentSpline.m_positions.size() - 1);
	double totalTime = m_theApp->m_gameClock->GetTotalSeconds();
	float normalizedTime = static_cast<float>(fmod(totalTime, numCurveSections) / numCurveSections);
	Vec2 movingParametrically = m_currentSpline.EvaluateAtParametric(normalizedTime);
	AddVertsForDisc2D(shapeVerts, movingParametrically, 5.f, Rgba8::WHITE);

	// Moving green point at a constant speed
	float totalLength = 0.f;
	std::vector<float> segmentLengths;
	for (int splinePointIndex = 0; splinePointIndex < static_cast<int>(m_currentSpline.m_positions.size()) - 1; ++splinePointIndex)
	{
		float segmentLength = (m_currentSpline.m_positions[splinePointIndex + 1] - m_currentSpline.m_positions[splinePointIndex]).GetLength();
		segmentLengths.push_back(segmentLength);
		totalLength += segmentLength;
	}

	float speed = totalLength / numCurveSections;
	float distanceAlongCurve = speed * static_cast<float>(fmod(totalTime, numCurveSections));
	float traveledDistance = 0.f;
	float paraVal = 0.f;
	for (int segmentIndex = 0; segmentIndex < static_cast<int>(segmentLengths.size()); ++segmentIndex)
	{
		if (traveledDistance + segmentLengths[segmentIndex] >= distanceAlongCurve)
		{
			float remainingDistance = distanceAlongCurve - traveledDistance;
			float t = remainingDistance / segmentLengths[segmentIndex];
			paraVal = (static_cast<float>(segmentIndex) + t) / numCurveSections;
			break;
		}
		traveledDistance += segmentLengths[segmentIndex];
	}

	Vec2 movingFixed = m_currentSpline.EvaluateAtParametric(paraVal);
	AddVertsForDisc2D(shapeVerts, movingFixed, 5.f, Rgba8::LIMEGREEN);
}

void Game2DCurves::NextAndPreviousInputs()
{
	if (g_theInput->WasKeyJustPressed('W'))
	{
		GetPreviousEasingFunction();
	}

	if (g_theInput->WasKeyJustPressed('E'))
	{
		GetNextEasingFunction();
	}
}

void Game2DCurves::GetNextEasingFunction()
{
	m_currentEasingFunction = (m_currentEasingFunction + 1) % NUM_EASING_FUNCS;
	m_easing = &g_easingFunctions[m_currentEasingFunction];
}

void Game2DCurves::GetPreviousEasingFunction()
{
	m_currentEasingFunction = (m_currentEasingFunction - 1 + NUM_EASING_FUNCS) % NUM_EASING_FUNCS;
	m_easing = &g_easingFunctions[m_currentEasingFunction];
}


void Game2DCurves::Update(float deltaSeconds)
{
	AdjustForPauseAndTimeDistortion(deltaSeconds);

	if (g_theInput->WasKeyJustPressed(KEYCODE_F1))
	{
		m_arePanesShowing = !m_arePanesShowing;
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_F8))
	{
		RandomizeCurves();
	}

	IncreaseDecreaseSubdivisions();
	NextAndPreviousInputs();
}

void Game2DCurves::IncreaseDecreaseSubdivisions()
{
	if (g_theInput->WasKeyJustPressed('M'))
	{
		m_numSubdivisions *= 2;
		if (m_numSubdivisions > 2048)
		{
			m_numSubdivisions = 2048;
		}
	}

	if (g_theInput->WasKeyJustPressed('N'))
	{
		m_numSubdivisions /= 2;
		if (m_numSubdivisions < 2)
		{
			m_numSubdivisions = 2;
		}
	}
}

void Game2DCurves::Render() const
{
	g_theRenderer->BeginCamera(m_theApp->m_screenCamera);
	GamemodeAndControlsText();

	if (m_arePanesShowing == true)
	{
		g_theRenderer->BindTexture(nullptr);
		g_theRenderer->DrawVertexArray(m_paneVerts);
	}

	std::vector<Vertex_PCU> curveVerts;
	std::vector<Vertex_PCU> textVerts;

	AddVertsForEasingCurves(curveVerts, textVerts, m_easingBox);
	AddVertsForBezierCurves(curveVerts);
	AddVertsForSplineCurves(curveVerts);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
	g_theRenderer->SetDepthMode(DepthMode::DISABLED);
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->DrawVertexArray(curveVerts);

	g_theRenderer->BindTexture(&m_font->GetTexture());
	g_theRenderer->DrawVertexArray(textVerts);
}

void Game2DCurves::GamemodeAndControlsText() const
{
	std::vector<Vertex_PCU> textVerts;
	std::string controlText = "F8 to Randomize; WE = Prev/Next Easing Function; N/M Curve Subdivisions (" + std::to_string(m_numSubdivisions) + "); Hold T for slow";
	m_font->AddVertsForTextInBox2D(textVerts, "Mode (F6/F7 for Prev/Next): Curves, Splines, Easing (2D)", m_gameSceneCoords, 15.f, Rgba8::GOLD, 0.8f, Vec2(0.f, 0.97f));
	m_font->AddVertsForTextInBox2D(textVerts, controlText, m_gameSceneCoords, 15.f, Rgba8::ALICEBLUE, 1.f, Vec2(0.f, 0.945f));
	g_theRenderer->BindTexture(&m_font->GetTexture());
	g_theRenderer->DrawVertexArray(textVerts);
}
// -----------------------------------------------------------------------------
float CustomFunkyFunction(float easeAmount)
{
	float t = easeAmount;
	return ComputeQuinticBezier1D(0.f, 0.f, PI * 0.01f, 1.f, PI * 0.01f, 1.f, t);
}
// -----------------------------------------------------------------------------