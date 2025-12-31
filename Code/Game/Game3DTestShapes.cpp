#include "Game/Game3DTestShapes.hpp"
#include "Game/App.h"
#include "Engine/Core/EngineCommon.h"
#include "Engine/Core/VertexUtils.h"
#include "Engine/Input/InputSystem.h"
#include "Engine/Math/MathUtils.h"
#include "Engine/Math/OBB3.hpp"
#include "Engine/Math/Plane3.hpp"

Game3DTestShapes::Game3DTestShapes(App* owner)
	:m_theApp(owner)
{
	// Setting the world camera camera to render
	Mat44 cameraToRender(Vec3(0.0f, 0.0f, 1.0f), Vec3(-1.0f, 0.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f), Vec3(0.f, 0.f, 0.f));
	g_theApp->m_worldCamera.SetCameraToRenderTransform(cameraToRender);

	m_font = g_theRenderer->CreateOrGetBitmapFont("Data/Fonts/SquirrelFixedFont");
	m_texture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Test_StbiFlippedAndOpenGL.png");
	m_gameSceneCoords = AABB2(Vec2::ZERO, Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y));

	RandomizeShapes();
}

void Game3DTestShapes::Update(float deltaSeconds)
{
	AdjustForPauseAndTimeDistortion(deltaSeconds);
	CameraKeyPresses(deltaSeconds);
	ToggleRasterizerMode();

	m_orientation.m_pitchDegrees = GetClamped(m_orientation.m_pitchDegrees, -89.9f, 89.9f);

	g_theApp->m_worldCamera.SetPositionAndOrientation(m_position, m_orientation);

	LockPosition();

	if (g_theInput->WasKeyJustPressed(KEYCODE_LEFT_MOUSE))
	{
		ToggleGrabObject();
	}
	if (m_isObjectGrabbed)
	{
		Vec3 cameraForward = GetForwardNormal() * 2.f;
		Vec3 grabbedObjectWorldPosition = m_position + cameraForward * 1.5f;

		if (m_grabbedObjectIndex < static_cast<int>(m_sphereVerts.size()) && m_isSphere)
		{
			Sphere& grabbedSphere = m_sphereVerts[m_grabbedObjectIndex];
			grabbedSphere.m_sphereCenter = grabbedObjectWorldPosition;
		}
		if (m_grabbedObjectIndex < static_cast<int>(m_aabb3s.size()) && m_isAABB3)
		{
			AABB3D& grabbedAABB3 = m_aabb3s[m_grabbedObjectIndex];
			AABB3 grabbedBox = AABB3(grabbedAABB3.m_mins, grabbedAABB3.m_maxs);

			Vec3 offset = grabbedObjectWorldPosition - grabbedBox.GetCenter();
			grabbedAABB3.m_mins += offset;
			grabbedAABB3.m_maxs += offset;
		}
		if (m_grabbedObjectIndex < static_cast<int>(m_cylinders.size()) && m_isCylinder)
		{
			Cylinder& grabbedCylinder = m_cylinders[m_grabbedObjectIndex];
			grabbedCylinder.m_start = grabbedObjectWorldPosition;
		}
		if (m_grabbedObjectIndex < static_cast<int>(m_obb3s.size()) && m_isOBB3)
		{
			OBB3D& grabbedOBB3 = m_obb3s[m_grabbedObjectIndex];
			grabbedOBB3.m_center = grabbedObjectWorldPosition;

			Mat44 orientationMatrix = grabbedOBB3.m_orientation.GetAsMatrix_IFwd_JLeft_KUp();
			grabbedOBB3.m_iBasis = orientationMatrix.GetIBasis3D();
			grabbedOBB3.m_jBasis = orientationMatrix.GetJBasis3D();
			grabbedOBB3.m_kBasis = orientationMatrix.GetKBasis3D();

			if (g_theInput->WasKeyJustPressed('O'))
			{
				grabbedOBB3.m_orientation.m_yawDegrees += 10.f;
			}
			if (g_theInput->WasKeyJustPressed('I'))
			{
				grabbedOBB3.m_orientation.m_yawDegrees -= 10.f;
			}
			if (g_theInput->WasKeyJustPressed('K'))
			{
				grabbedOBB3.m_orientation.m_pitchDegrees += 10.f;
			}
			if (g_theInput->WasKeyJustPressed('J'))
			{
				grabbedOBB3.m_orientation.m_pitchDegrees -= 10.f;
			}
			if (g_theInput->WasKeyJustPressed('M'))
			{
				grabbedOBB3.m_orientation.m_rollDegrees += 10.f;
			}
			if (g_theInput->WasKeyJustPressed('N'))
			{
				grabbedOBB3.m_orientation.m_rollDegrees -= 10.f;
			}
		}
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_F8))
	{
		RandomizeShapes();
	}

	GetNearestPointCheck();
	ShapevsShapeOverlap(deltaSeconds);
}

void Game3DTestShapes::ToggleGrabObject()
{
	m_isSphere = false;
	m_isAABB3 = false;
	m_isCylinder = false;
	m_isOBB3 = false;

	Vec3 startToEnd = m_rayCastEnd - m_rayCastStart;
	Vec3 raycastDirection = startToEnd; raycastDirection.Normalize();
	float maxDist = startToEnd.GetLength();

	RaycastResult3D nearestImpact;
	int nearestShape = -1;
	bool rayDidHit = false;

	for (int sphereIndex = 0; sphereIndex < static_cast<int>(m_sphereVerts.size()); ++sphereIndex)
	{
		RaycastResult3D rayCastResult = RaycastVsSphere3D(m_rayCastStart, raycastDirection, maxDist, m_sphereVerts[sphereIndex].m_sphereCenter, m_sphereVerts[sphereIndex].m_sphereRadius);
		if (rayCastResult.m_didImpact)
		{
			if (rayDidHit == false || rayCastResult.m_impactDist < nearestImpact.m_impactDist)
			{
				nearestImpact = rayCastResult;
				nearestShape = sphereIndex;
				rayDidHit = true;
				m_isSphere = true;
			}
		}
	}

	for (int aabb3Index = 0; aabb3Index < static_cast<int>(m_aabb3s.size()); ++aabb3Index)
	{
		RaycastResult3D rayCastResult = RaycastVsAABB3D(m_rayCastStart, raycastDirection, maxDist, AABB3(m_aabb3s[aabb3Index].m_mins, m_aabb3s[aabb3Index].m_maxs));
		if (rayCastResult.m_didImpact)
		{
			if (rayDidHit == false || rayCastResult.m_impactDist < nearestImpact.m_impactDist)
			{
				nearestImpact = rayCastResult;
				nearestShape = aabb3Index;
				rayDidHit = true;
				m_isAABB3 = true;
			}
		}
	}

	// Cylinder raycast check
	for (int cylinderIndex = 0; cylinderIndex < static_cast<int>(m_cylinders.size()); ++cylinderIndex)
	{
		RaycastResult3D rayCastResult = RaycastVsCylinder3D(m_rayCastStart, raycastDirection, maxDist, m_cylinders[cylinderIndex].m_start, m_cylinders[cylinderIndex].m_radius, m_cylinders[cylinderIndex].m_height);
		if (rayCastResult.m_didImpact)
		{
			if (rayDidHit == false || rayCastResult.m_impactDist < nearestImpact.m_impactDist)
			{
				nearestImpact = rayCastResult;
				nearestShape = cylinderIndex;
				rayDidHit = true;
				m_isCylinder = true;
			}
		}
	}

	// OBB3 raycast check
	for (int obb3Index = 0; obb3Index < static_cast<int>(m_obb3s.size()); ++obb3Index)
	{
		OBB3 orientedBox = OBB3(m_obb3s[obb3Index].m_center, m_obb3s[obb3Index].m_iBasis, m_obb3s[obb3Index].m_jBasis, m_obb3s[obb3Index].m_kBasis, m_obb3s[obb3Index].m_halfDimensions);
		RaycastResult3D raycastResult = RaycastVsOBB3D(m_rayCastStart, raycastDirection, maxDist, orientedBox);
		if (raycastResult.m_didImpact)
		{
			if (rayDidHit == false || raycastResult.m_impactDist < nearestImpact.m_impactDist)
			{
				nearestImpact = raycastResult;
				nearestShape = obb3Index;
				rayDidHit = true;
				m_isOBB3 = true;
			}
		}
	}

	if (rayDidHit)
	{
		if (m_isObjectGrabbed && m_grabbedObjectIndex == nearestShape)
		{
			m_isObjectGrabbed = false;
			m_grabbedObjectIndex = -1;
		}
		else
		{
			m_isObjectGrabbed = true;
			m_grabbedObjectIndex = nearestShape;
			m_grabbedObjectOffset = nearestImpact.m_impactPos - m_position;
		}
	}
	else
	{
		m_isObjectGrabbed = false;
		m_grabbedObjectIndex = -1;
	}
}

void Game3DTestShapes::LockPosition()
{
	if (g_theInput->WasKeyJustPressed(' '))
	{
		if (!m_isPositionLocked)
		{
			m_refPosition = m_position;
			m_rayCastStart = m_refPosition;
			m_isPositionLocked = true;

			GetNearestPointForRefPosition();
			GetPointClosestToPlayer();
		}
		else
		{
			m_isPositionLocked = false;
		}
	}

	if (!m_isPositionLocked)
	{
		m_rayCastStart = m_position;
		m_rayCastEnd = m_position + GetForwardNormal() * 2.f;
	}
}

void Game3DTestShapes::CameraKeyPresses(float deltaSeconds)
{
	// Yaw and Pitch with mouse
	m_orientation.m_yawDegrees += 0.08f * g_theInput->GetCursorClientDelta().x;
	m_orientation.m_pitchDegrees -= 0.08f * g_theInput->GetCursorClientDelta().y;

	float movementSpeed = 2.f;

	if (g_theInput->IsKeyDown(KEYCODE_SHIFT))
	{
		movementSpeed *= 5.f;
	}

	// Move left or right
	if (g_theInput->IsKeyDown('A'))
	{
		m_position += movementSpeed * m_orientation.GetAsMatrix_IFwd_JLeft_KUp().GetJBasis3D() * deltaSeconds;
	}
	if (g_theInput->IsKeyDown('D'))
	{
		m_position += -movementSpeed * m_orientation.GetAsMatrix_IFwd_JLeft_KUp().GetJBasis3D() * deltaSeconds;
	}

	// Move Forward and Backward
	if (g_theInput->IsKeyDown('W'))
	{
		m_position += movementSpeed * m_orientation.GetAsMatrix_IFwd_JLeft_KUp().GetIBasis3D() * deltaSeconds;
	}
	if (g_theInput->IsKeyDown('S'))
	{
		m_position += -movementSpeed * m_orientation.GetAsMatrix_IFwd_JLeft_KUp().GetIBasis3D() * deltaSeconds;
	}

	// Move Up and Down
	if (g_theInput->IsKeyDown('Q'))
	{
		m_position += -movementSpeed * Vec3::ZAXE * deltaSeconds;
	}
	if (g_theInput->IsKeyDown('E'))
	{
		m_position += movementSpeed * Vec3::ZAXE * deltaSeconds;
	}
}

void Game3DTestShapes::Render() const
{
	g_theRenderer->BeginCamera(g_theApp->m_screenCamera);
	GameModeAndControlsText();

	g_theRenderer->SetRasterizerMode(m_currentRasterizerMode);

	g_theRenderer->BeginCamera(g_theApp->m_worldCamera);
	DrawSphere();
	for (int spherePointIndex = 0; spherePointIndex < static_cast<int>(m_nearestSpherePoints.size()); ++spherePointIndex)
	{
		RenderNearestPoint(m_nearestSpherePoints[spherePointIndex]);
	}


	DrawAABB3();
	for (int aabb3PointIndex = 0; aabb3PointIndex < static_cast<int>(m_nearestAABB3Points.size()); ++aabb3PointIndex)
	{
		RenderNearestPoint(m_nearestAABB3Points[aabb3PointIndex]);
	}

	DrawCylinder();
	for (int cylinderPointIndex = 0; cylinderPointIndex < static_cast<int>(m_nearestCylinderPoints.size()); ++cylinderPointIndex)
	{
		RenderNearestPoint(m_nearestCylinderPoints[cylinderPointIndex]);
	}

	DrawOBB3();
	for (int obb3PointIndex = 0; obb3PointIndex < static_cast<int>(m_nearestOBB3Points.size()); ++obb3PointIndex)
	{
		RenderNearestPoint(m_nearestOBB3Points[obb3PointIndex]);
	}

	DrawPlane();
	for (int planeIndex = 0; planeIndex < static_cast<int>(m_nearestPlanePoints.size()); ++planeIndex)
	{
		RenderNearestPoint(m_nearestPlanePoints[planeIndex]);
	}

	DrawBasis();
	DrawRaycast();

	if (m_isObjectGrabbed)
	{
		DrawGrabbedObject(m_grabbedObjectIndex);
	}
}

Mat44 Game3DTestShapes::GetModelToWorldTransform() const
{
	Mat44 modelToWorldMatrix;
	modelToWorldMatrix.SetTranslation3D(m_position);
	modelToWorldMatrix.Append(m_orientation.GetAsMatrix_IFwd_JLeft_KUp());
	return modelToWorldMatrix;
}

Vec3 Game3DTestShapes::GetForwardNormal() const
{
	return Vec3::MakeFromPolarDegrees(m_orientation.m_pitchDegrees, m_orientation.m_yawDegrees);
}

void Game3DTestShapes::DrawSphere() const
{
	for (int sphereIndex = 0; sphereIndex < static_cast<int>(m_sphereVerts.size()); ++sphereIndex)
	{
		std::vector<Vertex_PCU> sphereVerts;
		Sphere const& sphere = m_sphereVerts[sphereIndex];

		AddVertsForSphere3D(sphereVerts, sphere.m_sphereCenter, sphere.m_sphereRadius, sphere.m_color);
		g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
		g_theRenderer->SetRasterizerMode(m_currentRasterizerMode);
		g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);

		if (m_isSolidShapeTexture)
		{
			g_theRenderer->BindTexture(m_texture);
		}
		else
		{
			g_theRenderer->BindTexture(nullptr);
		}

		g_theRenderer->SetModelConstants();
		g_theRenderer->DrawVertexArray(sphereVerts);
	}
}

void Game3DTestShapes::DrawAABB3() const
{
	for (int aabb3Index = 0; aabb3Index < static_cast<int>(m_aabb3s.size()); ++aabb3Index)
	{
		std::vector<Vertex_PCU> aabb3Verts;
		AABB3D const& aabb3 = m_aabb3s[aabb3Index];

		AddVertsForAABB3D(aabb3Verts, AABB3(aabb3.m_mins, aabb3.m_maxs), aabb3.m_color);
		g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
		g_theRenderer->SetRasterizerMode(m_currentRasterizerMode);
		g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);

		if (m_isSolidShapeTexture)
		{
			g_theRenderer->BindTexture(m_texture);
		}
		else
		{
			g_theRenderer->BindTexture(nullptr);
		}

		g_theRenderer->SetModelConstants();
		g_theRenderer->DrawVertexArray(aabb3Verts);
	}
}

void Game3DTestShapes::DrawCylinder() const
{
	for (int cylinderIndex = 0; cylinderIndex < static_cast<int>(m_cylinders.size()); ++cylinderIndex)
	{
		std::vector<Vertex_PCU> cylinderVerts;
		Cylinder const& cylinder = m_cylinders[cylinderIndex];

		AddVertsForCylinderZ3D(cylinderVerts, cylinder.m_start, cylinder.m_radius, cylinder.m_height, cylinder.m_color);
		g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
		g_theRenderer->SetRasterizerMode(m_currentRasterizerMode);
		g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);

		if (m_isSolidShapeTexture)
		{
			g_theRenderer->BindTexture(m_texture);
		}
		else
		{
			g_theRenderer->BindTexture(nullptr);
		}

		g_theRenderer->SetModelConstants();
		g_theRenderer->DrawVertexArray(cylinderVerts);
	}
}

void Game3DTestShapes::DrawOBB3() const
{
	for (int obb3Index = 0; obb3Index < static_cast<int>(m_obb3s.size()); ++obb3Index)
	{
		std::vector<Vertex_PCU> obb3Verts;
		OBB3D const& obb3 = m_obb3s[obb3Index];
		OBB3 orientedBox = OBB3(obb3.m_center, obb3.m_iBasis, obb3.m_jBasis, obb3.m_kBasis, obb3.m_halfDimensions);

		AddVertsForOBB3D(obb3Verts, orientedBox, obb3.m_color);

		g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
		g_theRenderer->SetRasterizerMode(m_currentRasterizerMode);
		g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);

		if (m_isSolidShapeTexture)
		{
			g_theRenderer->BindTexture(m_texture);
		}
		else
		{
			g_theRenderer->BindTexture(nullptr);
		}

		g_theRenderer->SetModelConstants();
		g_theRenderer->DrawVertexArray(obb3Verts);
	}
}

void Game3DTestShapes::DrawPlane() const
{
	for (int planeIndex = 0; planeIndex < static_cast<int>(m_planes.size()); ++planeIndex)
	{
		std::vector<Vertex_PCU> planeVerts;
		Plane3D const& plane = m_planes[planeIndex];
		Plane3 planeGrid = Plane3(plane.m_normal, plane.m_distance);

		AddVertsForPlane3D(planeVerts, planeGrid);

		g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
		g_theRenderer->SetRasterizerMode(m_currentRasterizerMode);
		g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
		g_theRenderer->BindTexture(nullptr);

		g_theRenderer->SetModelConstants();
		g_theRenderer->DrawVertexArray(planeVerts);
	}
}

void Game3DTestShapes::DrawBasis() const
{
	std::vector<Vertex_PCU> basisVerts;
	Vec3 arrowPosition = m_position + GetForwardNormal() * 0.2f;
	float axisLength = 0.01f;    
	float shaftFactor = 0.6f;
	float shaftRadius = 0.0008f; 
	float coneRadius = 0.0015f;  

	Vec3 xConeEnd = arrowPosition + Vec3::XAXE * axisLength;
	Vec3 yConeEnd = arrowPosition + Vec3::YAXE * axisLength;
	Vec3 zConeEnd = arrowPosition + Vec3::ZAXE * axisLength;

	Vec3 xCylinderEnd = arrowPosition + Vec3::XAXE * (axisLength * shaftFactor);
	Vec3 yCylinderEnd = arrowPosition + Vec3::YAXE * (axisLength * shaftFactor);
	Vec3 zCylinderEnd = arrowPosition + Vec3::ZAXE * (axisLength * shaftFactor);

	AddVertsForCylinder3D(basisVerts, arrowPosition, xCylinderEnd, shaftRadius, Rgba8::RED, AABB2::ZERO_TO_ONE, 32);
	AddVertsForCylinder3D(basisVerts, arrowPosition, yCylinderEnd, shaftRadius, Rgba8::GREEN, AABB2::ZERO_TO_ONE, 32);
	AddVertsForCylinder3D(basisVerts, arrowPosition, zCylinderEnd, shaftRadius, Rgba8::BLUE, AABB2::ZERO_TO_ONE, 32);

	AddVertsForCone3D(basisVerts, xCylinderEnd, xConeEnd, coneRadius, Rgba8::RED, AABB2::ZERO_TO_ONE, 32);
	AddVertsForCone3D(basisVerts, yCylinderEnd, yConeEnd, coneRadius, Rgba8::GREEN, AABB2::ZERO_TO_ONE, 32);
	AddVertsForCone3D(basisVerts, zCylinderEnd, zConeEnd, coneRadius, Rgba8::BLUE, AABB2::ZERO_TO_ONE, 32);
	g_theRenderer->SetModelConstants();
	g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->DrawVertexArray(basisVerts);
}

void Game3DTestShapes::DrawRaycast() const
{
	Vec3 startToEnd = m_rayCastEnd - m_rayCastStart;
	Vec3 raycastDirection = startToEnd; raycastDirection.Normalize();
	float maxDist = startToEnd.GetLength();

	std::vector<Vertex_PCU> arrowVerts;
	bool didRayHit = false;
	bool isSphere = false;
	bool isAABB3 = false;
	bool isCylinder = false;
	bool isOBB3 = false;
	bool isPlane = false;
	RaycastResult3D nearestImpact;
	int nearestShape = 0;

	// Sphere raycast check
	for (int sphereIndex = 0; sphereIndex < static_cast<int>(m_sphereVerts.size()); ++sphereIndex)
	{
		RaycastResult3D rayCastResult = RaycastVsSphere3D(m_rayCastStart, raycastDirection, maxDist, m_sphereVerts[sphereIndex].m_sphereCenter, m_sphereVerts[sphereIndex].m_sphereRadius);
		if (rayCastResult.m_didImpact)
		{
			if (didRayHit == false || rayCastResult.m_impactDist < nearestImpact.m_impactDist)
			{
				nearestImpact = rayCastResult;
				nearestShape = sphereIndex;
				didRayHit = true;
				isSphere = true;
			}
		}
	}

	// AABB3 raycast check
	for (int aabb3Index = 0; aabb3Index < static_cast<int>(m_aabb3s.size()); ++aabb3Index)
	{
		RaycastResult3D rayCastResult = RaycastVsAABB3D(m_rayCastStart, raycastDirection, maxDist, AABB3(m_aabb3s[aabb3Index].m_mins, m_aabb3s[aabb3Index].m_maxs));
		if (rayCastResult.m_didImpact)
		{
			if (didRayHit == false || rayCastResult.m_impactDist < nearestImpact.m_impactDist)
			{
				nearestImpact = rayCastResult;
				nearestShape = aabb3Index;
				didRayHit = true;
				isAABB3 = true;
			}
		}
	}

	// Cylinder raycast check
	for (int cylinderIndex = 0; cylinderIndex < static_cast<int>(m_cylinders.size()); ++cylinderIndex)
	{
		RaycastResult3D rayCastResult = RaycastVsCylinder3D(m_rayCastStart, raycastDirection, maxDist, m_cylinders[cylinderIndex].m_start, m_cylinders[cylinderIndex].m_radius, m_cylinders[cylinderIndex].m_height);
		if (rayCastResult.m_didImpact)
		{
			if (didRayHit == false || rayCastResult.m_impactDist < nearestImpact.m_impactDist)
			{
				nearestImpact = rayCastResult;
				nearestShape = cylinderIndex;
				didRayHit = true;
				isCylinder = true;
			}
		}
	}

	// OBB3 raycast check
	for (int obb3Index = 0; obb3Index < static_cast<int>(m_obb3s.size()); ++obb3Index)
	{
		OBB3 orientedBox = OBB3(m_obb3s[obb3Index].m_center, m_obb3s[obb3Index].m_iBasis, m_obb3s[obb3Index].m_jBasis, m_obb3s[obb3Index].m_kBasis, m_obb3s[obb3Index].m_halfDimensions);
		RaycastResult3D raycastResult = RaycastVsOBB3D(m_rayCastStart, raycastDirection, maxDist, orientedBox);
		if (raycastResult.m_didImpact)
		{
			if (didRayHit == false || raycastResult.m_impactDist < nearestImpact.m_impactDist)
			{
				nearestImpact = raycastResult;
				nearestShape = obb3Index;
				didRayHit = true;
				isOBB3 = true;
			}
		}
	}

	// Plane raycast check
	for (int planeIndex = 0; planeIndex < static_cast<int>(m_planes.size()); ++planeIndex)
	{
		Plane3 plane = Plane3(m_planes[planeIndex].m_normal, m_planes[planeIndex].m_distance);
		RaycastResult3D raycastResult = RaycastVsPlane3D(m_rayCastStart, raycastDirection, maxDist, plane);
		if (raycastResult.m_didImpact)
		{
			if (didRayHit == false || raycastResult.m_impactDist < nearestImpact.m_impactDist)
			{
				nearestImpact = raycastResult;
				nearestShape = planeIndex;
				didRayHit = true;
				isPlane = true;
			}
		}
	}

	if (didRayHit)
	{
		if (didRayHit && m_isPositionLocked)
		{
			AddVertsForMathArrow3D(arrowVerts, m_rayCastStart, nearestImpact.m_impactPos, 0.2f, Rgba8::RED);
			AddVertsForMathArrow3D(arrowVerts, m_rayCastStart, m_rayCastEnd, 0.05f, Rgba8::DARKGRAY);
			AddVertsForMathArrow3D(arrowVerts, nearestImpact.m_impactPos, nearestImpact.m_impactPos + nearestImpact.m_impactNormal * 1.f, 0.2f, Rgba8::YELLOW);
			AddVertsForSphere3D(arrowVerts, nearestImpact.m_impactPos, 0.08f);
		}

		AddVertsForMathArrow3D(arrowVerts, nearestImpact.m_impactPos, nearestImpact.m_impactPos + nearestImpact.m_impactNormal * 1.f, 0.2f, Rgba8::YELLOW);
		AddVertsForSphere3D(arrowVerts, nearestImpact.m_impactPos, 0.08f);

		std::vector<Vertex_PCU> impactedShapeVerts;
		if (m_currentRasterizerMode == RasterizerMode::SOLID_CULL_BACK && isSphere)
		{
			AddVertsForSphere3D(impactedShapeVerts, m_sphereVerts[nearestShape].m_sphereCenter, m_sphereVerts[nearestShape].m_sphereRadius, Rgba8::BLUE);
		}
		if (m_currentRasterizerMode == RasterizerMode::SOLID_CULL_BACK && isAABB3)
		{
			AddVertsForAABB3D(impactedShapeVerts, AABB3(m_aabb3s[nearestShape].m_mins, m_aabb3s[nearestShape].m_maxs), Rgba8::BLUE);
		}
		if (m_currentRasterizerMode == RasterizerMode::SOLID_CULL_BACK && isCylinder)
		{
			AddVertsForCylinderZ3D(impactedShapeVerts, m_cylinders[nearestShape].m_start, m_cylinders[nearestShape].m_radius, m_cylinders[nearestShape].m_height, Rgba8::BLUE);
		}
		if (m_currentRasterizerMode == RasterizerMode::SOLID_CULL_BACK && isOBB3)
		{
			OBB3 orientedBox = OBB3(m_obb3s[nearestShape].m_center, m_obb3s[nearestShape].m_iBasis, m_obb3s[nearestShape].m_jBasis, m_obb3s[nearestShape].m_kBasis, m_obb3s[nearestShape].m_halfDimensions);
			AddVertsForOBB3D(impactedShapeVerts, orientedBox, Rgba8::BLUE);
		}
		g_theRenderer->SetModelConstants();
		g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
		g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
		g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
		g_theRenderer->BindTexture(m_texture);
		g_theRenderer->DrawVertexArray(impactedShapeVerts);
	}
	else if (!didRayHit && m_isPositionLocked)
	{
		AddVertsForMathArrow3D(arrowVerts, m_rayCastStart, m_rayCastEnd, 0.2f, Rgba8::GREEN);
	}
	g_theRenderer->SetModelConstants();
	g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->DrawVertexArray(arrowVerts);
}

void Game3DTestShapes::DrawGrabbedObject(int shapeIndex) const
{
	std::vector<Vertex_PCU> shapeVerts;

	if (shapeIndex < static_cast<int>(m_sphereVerts.size()) && m_isSphere)
	{
		Sphere const& grabbedSphere = m_sphereVerts[shapeIndex];
		AddVertsForSphere3D(shapeVerts, grabbedSphere.m_sphereCenter, grabbedSphere.m_sphereRadius, Rgba8::RED);
	}
	if (shapeIndex < static_cast<int>(m_aabb3s.size()) && m_isAABB3)
	{
		AABB3D const& grabbedAABB3 = m_aabb3s[shapeIndex];
		AddVertsForAABB3D(shapeVerts, AABB3(grabbedAABB3.m_mins, grabbedAABB3.m_maxs), Rgba8::RED);
	}
	if (shapeIndex < static_cast<int>(m_cylinders.size()) && m_isCylinder)
	{
		Cylinder const& grabbedCylinder = m_cylinders[shapeIndex];
		AddVertsForCylinderZ3D(shapeVerts, grabbedCylinder.m_start, grabbedCylinder.m_radius, grabbedCylinder.m_height, Rgba8::RED);
	}
	if (shapeIndex < static_cast<int>(m_obb3s.size()) && m_isOBB3)
	{
		OBB3D const& grabbedOBB3 = m_obb3s[shapeIndex];
		AddVertsForOBB3D(shapeVerts, OBB3(grabbedOBB3.m_center, grabbedOBB3.m_iBasis, grabbedOBB3.m_jBasis, grabbedOBB3.m_kBasis, grabbedOBB3.m_halfDimensions), Rgba8::RED);
	}

	g_theRenderer->SetModelConstants();
	g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
	g_theRenderer->SetRasterizerMode(m_currentRasterizerMode);
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
	g_theRenderer->BindTexture(m_texture);
	g_theRenderer->DrawVertexArray(shapeVerts);
}

void Game3DTestShapes::AddVertsForPlane3D(std::vector<Vertex_PCU>& verts, Plane3 const& plane) const
{
	// Set y values of grid
	for (int planeY = -20; planeY <= 20; ++planeY)
	{
		if (planeY == 0)
		{
			AddVertsForAABB3D(verts, AABB3(Vec3(-20.f, static_cast<float>(planeY - 0.04f), -0.04f), Vec3(20.f, static_cast<float>(planeY + 0.04f), 0.04f)), Rgba8::DARKRED);
		}
		if (planeY % 2 == 0)
		{
			AddVertsForAABB3D(verts, AABB3(Vec3(-20.f, static_cast<float>(planeY - 0.02f), -0.02f), Vec3(20.f, static_cast<float>(planeY + 0.02f), 0.02f)), Rgba8::DARKRED);
		}
	}

	// Set x values of grid
	for (int planeX = -20; planeX <= 20; ++planeX)
	{
		if (planeX == 0)
		{
			AddVertsForAABB3D(verts, AABB3(Vec3(static_cast<float>(planeX - 0.04f), -20.f, -0.04f), Vec3(static_cast<float>(planeX + 0.04f), 20.f, 0.04f)), Rgba8::SEAWEED);
		}
		if (planeX % 2 == 0)
		{
			AddVertsForAABB3D(verts, AABB3(Vec3(static_cast<float>(planeX - 0.02f), -20.f, -0.02f), Vec3(static_cast<float>(planeX + 0.02f), 20.f, 0.02f)), Rgba8::SEAWEED);
		}
	}

	Vec3 planeCenter = plane.GetPlaneCenter();
	Vec3 zCrossPlaneNormal = CrossProduct3D(Vec3::ZAXE, plane.m_normal);
	Vec3 iBasis = Vec3::ZERO;
	Vec3 jBasis = Vec3::ZERO;

	if (zCrossPlaneNormal == Vec3::ZERO)
	{
		jBasis = Vec3::YAXE;
	}
	else
	{
		jBasis = zCrossPlaneNormal.GetNormalized();
	}
	iBasis = CrossProduct3D(jBasis, plane.m_normal).GetNormalized();
	Vec3 kBasis = plane.m_normal;

	Vec3 origin = Vec3::ZERO;
	Vec3 planeNormal = plane.m_normal;
	float distanceToOrigin = DotProduct3D(planeNormal, origin) - plane.m_distance;
	Vec3 closestPointOnPlane = origin - (distanceToOrigin * planeNormal);

	AddVertsForSphere3D(verts, origin, 0.2f, Rgba8::GRAY);
	AddVertsForCylinder3D(verts, origin, closestPointOnPlane, 0.05f, Rgba8::LIGHTGRAY);

	Mat44 transform;
	transform.SetIJKT3D(iBasis, jBasis, kBasis, planeCenter);
	TransformVertexArray3D(verts, transform);
}

void Game3DTestShapes::RandomizeShapes()
{
	m_sphereVerts.clear();
	for (int sphereIndex = 0; sphereIndex < NUM_SPHERES; ++sphereIndex)
	{
		Sphere newSpheres;
		newSpheres.m_sphereCenter = Vec3(g_rng->RollRandomFloatInRange(0.f, 10.f), g_rng->RollRandomFloatInRange(0.f, 10.f), g_rng->RollRandomFloatInRange(-10.f, 10.f));
		newSpheres.m_sphereRadius = g_rng->RollRandomFloatInRange(0.3f, 2.0f);
		m_sphereVerts.push_back(newSpheres);
	}

	m_aabb3s.clear();
	for (int aabb3sIndex = 0; aabb3sIndex < NUM_AABB3S; ++aabb3sIndex)
	{
		AABB3D newAABB3s;
		newAABB3s.m_mins = Vec3(g_rng->RollRandomFloatInRange(0.f, 3.f), g_rng->RollRandomFloatInRange(0.f, 3.f), g_rng->RollRandomFloatInRange(-3.f, 3.f));
		newAABB3s.m_maxs = Vec3(g_rng->RollRandomFloatInRange(newAABB3s.m_mins.x + 0.2f, 5.f),
								g_rng->RollRandomFloatInRange(newAABB3s.m_mins.y + 0.2f, 4.f),
								g_rng->RollRandomFloatInRange(newAABB3s.m_mins.z + 0.2f, 4.f));
		m_aabb3s.push_back(newAABB3s);
	}

	m_cylinders.clear();
	for (int cylinderIndex = 0; cylinderIndex < NUM_CYLINDERS; ++cylinderIndex)
	{
		Cylinder newCylinders;
		newCylinders.m_start = Vec3(g_rng->RollRandomFloatInRange(-10.f, 10.f), g_rng->RollRandomFloatInRange(0.f, 10.f), g_rng->RollRandomFloatInRange(-10.f, 10.f));
		newCylinders.m_radius = g_rng->RollRandomFloatInRange(0.5f, 2.0f);
		newCylinders.m_height = g_rng->RollRandomFloatInRange(0.5f, 3.0f);
		m_cylinders.push_back(newCylinders);
	}

	m_obb3s.clear();
	for (int obb3Index = 0; obb3Index < NUM_OBB3S; ++obb3Index)
	{
		OBB3D newOBB3;
		newOBB3.m_center = Vec3(g_rng->RollRandomFloatInRange(-10.f, 10.f), g_rng->RollRandomFloatInRange(-10.f, 10.f), g_rng->RollRandomFloatInRange(0.f, 10.f));
		newOBB3.m_halfDimensions = Vec3(g_rng->RollRandomFloatInRange(0.3f, 2.f), g_rng->RollRandomFloatInRange(0.3f, 2.f), g_rng->RollRandomFloatInRange(0.3f, 2.f));

		Vec3 iBasis; 
		do 
		{
			iBasis = Vec3(g_rng->RollRandomFloatInRange(-1.f, 1.f), g_rng->RollRandomFloatInRange(-1.f, 1.f), g_rng->RollRandomFloatInRange(-1.f, 1.f));
		} 
		while (iBasis.GetLengthSquared() < 0.0001f);
		iBasis.Normalize();

		Vec3 up = Vec3::ZAXE; 
		if (fabsf(DotProduct3D(iBasis, up)) > 0.95f)
		{
			up = Vec3::YAXE;
		}

		Vec3 kBasis = CrossProduct3D(iBasis, up).GetNormalized(); 
		Vec3 jBasis = CrossProduct3D(kBasis, iBasis).GetNormalized();
		newOBB3.m_iBasis = iBasis;  
		newOBB3.m_jBasis = jBasis;  
		newOBB3.m_kBasis = kBasis;  

		m_obb3s.push_back(newOBB3);
	}

	m_planes.clear();
	for (int planeIndex = 0; planeIndex < NUM_PLANES; ++planeIndex)
	{
		Vec3 randomNormal;
		do 
		{
			randomNormal = Vec3(g_rng->RollRandomFloatInRange(-1.f, 1.f), g_rng->RollRandomFloatInRange(-1.f, 1.f), g_rng->RollRandomFloatInRange(-1.f, 1.f));
		} 
		while (randomNormal.GetLengthSquared() < 0.001f);
		randomNormal.Normalize();

		Vec3 pointOnPlane = Vec3(g_rng->RollRandomFloatInRange(-10.f, 10.f), g_rng->RollRandomFloatInRange(-10.f, 10.f), g_rng->RollRandomFloatInRange(0.f, 10.f));
		float distance = DotProduct3D(randomNormal, pointOnPlane);

		Plane3D newPlane;
		newPlane.m_normal = randomNormal;
		newPlane.m_distance = distance;
		m_planes.push_back(newPlane);
	}
}

void Game3DTestShapes::ToggleRasterizerMode()
{
	if (g_theInput->WasKeyJustPressed('R'))
	{
		if (m_currentRasterizerMode == RasterizerMode::SOLID_CULL_BACK)
		{
			m_currentRasterizerMode = RasterizerMode::WIREFRAME_CULL_NONE;
			m_isSolidShapeTexture = false;
		}
		else
		{
			m_currentRasterizerMode = RasterizerMode::SOLID_CULL_BACK;
			m_isSolidShapeTexture = true;
		}
	}
}

void Game3DTestShapes::RenderNearestPoint(Vec3 const& point) const
{
	Rgba8 color = Rgba8::ORANGE;
	if (point == m_closestPointToPlayer)
	{
		color = Rgba8::LIMEGREEN;
	}
	else
	{
		color = Rgba8::ORANGE;
	}

	std::vector<Vertex_PCU> verts;
	AddVertsForSphere3D(verts, point, 0.1f, color);
	g_theRenderer->SetModelConstants();
	g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->DrawVertexArray(verts);
}

void Game3DTestShapes::GetNearestPointCheck()
{
	if (!m_isPositionLocked)
	{
		m_nearestSpherePoints.clear();
		for (int sphereIndex = 0; sphereIndex < static_cast<int>(m_sphereVerts.size()); ++sphereIndex)
		{
			Sphere const& sphere = m_sphereVerts[sphereIndex];
			Vec3 nearestPoint = GetNearestPointOnSphere3D(m_position, sphere.m_sphereCenter, sphere.m_sphereRadius);
			m_nearestSpherePoints.push_back(nearestPoint);
		}

		m_nearestAABB3Points.clear();
		for (int aabb3Index = 0; aabb3Index < static_cast<int>(m_aabb3s.size()); ++aabb3Index)
		{
			AABB3D const& aabb3 = m_aabb3s[aabb3Index];
			Vec3 nearestPoint = GetNearestPointOnAABB3D(m_position, AABB3(aabb3.m_mins, aabb3.m_maxs));
			m_nearestAABB3Points.push_back(nearestPoint);
		}

		m_nearestCylinderPoints.clear();
		for (int cylinderIndex = 0; cylinderIndex < static_cast<int>(m_cylinders.size()); ++cylinderIndex)
		{
			Cylinder const& cylinder = m_cylinders[cylinderIndex];
			Vec3 nearestPoint = GetNearestPointOnCylinderZ3D(m_position, cylinder.m_start, cylinder.m_radius, cylinder.m_height);
			m_nearestCylinderPoints.push_back(nearestPoint);
		}

		m_nearestOBB3Points.clear();
		for (int obb3Index = 0; obb3Index < static_cast<int>(m_obb3s.size()); ++obb3Index)
		{
			OBB3D const& obb3 = m_obb3s[obb3Index];
			Vec3 nearestPoint = GetNearestPointOnOBB3D(m_position, OBB3(obb3.m_center, obb3.m_iBasis, obb3.m_jBasis, obb3.m_kBasis, obb3.m_halfDimensions));
			m_nearestOBB3Points.push_back(nearestPoint);
		}

		m_nearestPlanePoints.clear();
		for (int planeIndex = 0; planeIndex < static_cast<int>(m_planes.size()); ++planeIndex)
		{
			Plane3D const& plane = m_planes[planeIndex];
			Vec3 nearestPoint = GetNearestPointOnPlane3D(m_position, Plane3(plane.m_normal, plane.m_distance));
			m_nearestPlanePoints.push_back(nearestPoint);
		}

		GetPointClosestToPlayer();
	}
}

void Game3DTestShapes::GetNearestPointForRefPosition()
{
	m_nearestSpherePoints.clear();
	for (int sphereIndex = 0; sphereIndex < static_cast<int>(m_sphereVerts.size()); ++sphereIndex)
	{
		Sphere const& sphere = m_sphereVerts[sphereIndex];
		Vec3 nearestPoint = GetNearestPointOnSphere3D(m_refPosition, sphere.m_sphereCenter, sphere.m_sphereRadius);
		m_nearestSpherePoints.push_back(nearestPoint);
	}

	m_nearestAABB3Points.clear();
	for (int aabb3Index = 0; aabb3Index < static_cast<int>(m_aabb3s.size()); ++aabb3Index)
	{
		AABB3D const& aabb3 = m_aabb3s[aabb3Index];
		Vec3 nearestPoint = GetNearestPointOnAABB3D(m_refPosition, AABB3(aabb3.m_mins, aabb3.m_maxs));
		m_nearestAABB3Points.push_back(nearestPoint);
	}

	m_nearestCylinderPoints.clear();
	for (int cylinderIndex = 0; cylinderIndex < static_cast<int>(m_cylinders.size()); ++cylinderIndex)
	{
		Cylinder const& cylinder = m_cylinders[cylinderIndex];
		Vec3 nearestPoint = GetNearestPointOnCylinderZ3D(m_refPosition, cylinder.m_start, cylinder.m_radius, cylinder.m_height);
		m_nearestCylinderPoints.push_back(nearestPoint);
	}

	m_nearestOBB3Points.clear();
	for (int obb3Index = 0; obb3Index < static_cast<int>(m_obb3s.size()); ++obb3Index)
	{
		OBB3D const& obb3 = m_obb3s[obb3Index];
		Vec3 nearestPoint = GetNearestPointOnOBB3D(m_refPosition, OBB3(obb3.m_center, obb3.m_iBasis, obb3.m_jBasis, obb3.m_kBasis, obb3.m_halfDimensions));
		m_nearestOBB3Points.push_back(nearestPoint);
	}

	m_nearestPlanePoints.clear();
	for (int planeIndex = 0; planeIndex < static_cast<int>(m_planes.size()); ++planeIndex)
	{
		Plane3D const& plane = m_planes[planeIndex];
		Vec3 nearestPoint = GetNearestPointOnPlane3D(m_position, Plane3(plane.m_normal, plane.m_distance));
		m_nearestPlanePoints.push_back(nearestPoint);
	}
}

void Game3DTestShapes::GetPointClosestToPlayer()
{
	float closestDistance = 99999.f;
	Vec3 closestPoint;
	std::vector<std::vector<Vec3>> points = { m_nearestSpherePoints, m_nearestAABB3Points, m_nearestCylinderPoints, m_nearestOBB3Points, m_nearestPlanePoints };

	for (int pointsContainer = 0; pointsContainer < static_cast<int>(points.size()); ++pointsContainer)
	{
		for (int pointIndex = 0; pointIndex < static_cast<int>(points[pointsContainer].size()); ++pointIndex)
		{
			float dist = (points[pointsContainer][pointIndex] - m_position).GetLengthSquared();
			if (dist < closestDistance)
			{
				closestDistance = dist;
				closestPoint = points[pointsContainer][pointIndex];
			}
		}
	}
	m_closestPointToPlayer = closestPoint;
}

void Game3DTestShapes::ShapevsShapeOverlap(float deltaSeconds)
{
	m_colorBrightness += 200.f * static_cast<float>(deltaSeconds);
	float sinColor = fabsf(SinDegrees(m_colorBrightness));
	unsigned char colorValue = static_cast<unsigned char>(GetClamped(sinColor, 0.f, 1.f) * 255);

	// Sphere vs Sphere
	for (int sphereAIndex = 0; sphereAIndex < static_cast<int>(m_sphereVerts.size()); ++sphereAIndex)
	{
		Sphere& sphereA = m_sphereVerts[sphereAIndex];
		for (int sphereBIndex = sphereAIndex + 1; sphereBIndex < static_cast<int>(m_sphereVerts.size()); ++sphereBIndex)
		{
			Sphere& sphereB = m_sphereVerts[sphereBIndex];
			if (DoSpheresOverlap(sphereA.m_sphereCenter, sphereA.m_sphereRadius, sphereB.m_sphereCenter, sphereB.m_sphereRadius))
			{
				sphereA.m_color = Rgba8(colorValue, colorValue, colorValue, 255);
				sphereB.m_color = Rgba8(colorValue, colorValue, colorValue, 255);
			}
		}
	}

	// AABB3 vs AABB3
	for (int aabb3AIndex = 0; aabb3AIndex < static_cast<int>(m_aabb3s.size()); ++aabb3AIndex)
	{
		AABB3D& aabb3A = m_aabb3s[aabb3AIndex];
		for (int aabb3BIndex = aabb3AIndex + 1; aabb3BIndex < static_cast<int>(m_aabb3s.size()); ++aabb3BIndex)
		{
			AABB3D& aabb3B = m_aabb3s[aabb3BIndex];
			if (DoAABB3sOverlap(AABB3(aabb3A.m_mins, aabb3A.m_maxs), AABB3(aabb3B.m_mins, aabb3B.m_maxs)))
			{
				aabb3A.m_color = Rgba8(colorValue, colorValue, colorValue, 255);
				aabb3B.m_color = Rgba8(colorValue, colorValue, colorValue, 255);
			}
		}
	}

	// Sphere vs AABB3
	for (int sphereIndex = 0; sphereIndex < static_cast<int>(m_sphereVerts.size()); ++sphereIndex)
	{
		Sphere& sphere = m_sphereVerts[sphereIndex];
		for (int aabb3Index = 0; aabb3Index < static_cast<int>(m_aabb3s.size()); ++aabb3Index)
		{
			AABB3D& aabb3 = m_aabb3s[aabb3Index];
			if (DoSpheresAndAABBOverlap3D(sphere.m_sphereCenter, sphere.m_sphereRadius, AABB3(aabb3.m_mins, aabb3.m_maxs)))
			{
				aabb3.m_color = Rgba8(colorValue, colorValue, colorValue, 255);
				sphere.m_color = Rgba8(colorValue, colorValue, colorValue, 255);
			}
		}
	}

	// Cylinder vs Cylinder
	for (int cylinderAIndex = 0; cylinderAIndex < static_cast<int>(m_cylinders.size()); ++cylinderAIndex)
	{
		Cylinder& cylinderA = m_cylinders[cylinderAIndex];
		for (int cylinderBIndex = cylinderAIndex + 1; cylinderBIndex < static_cast<int>(m_cylinders.size()); ++cylinderBIndex)
		{
			Cylinder& cylinderB = m_cylinders[cylinderBIndex];
			if (DoZCylindersOverlap3D(cylinderA.m_start, cylinderA.m_radius, cylinderA.m_height, cylinderB.m_start, cylinderB.m_radius, cylinderB.m_height))
			{
				cylinderA.m_color = Rgba8(colorValue, colorValue, colorValue, 255);
				cylinderB.m_color = Rgba8(colorValue, colorValue, colorValue, 255);
			}
		}
	}

	// Cylinder vs Sphere
	for (int cylinderIndex = 0; cylinderIndex < static_cast<int>(m_cylinders.size()); ++cylinderIndex)
	{
		Cylinder& cylinder = m_cylinders[cylinderIndex];
		for (int sphereIndex = 0; sphereIndex < static_cast<int>(m_sphereVerts.size()); ++sphereIndex)
		{
			Sphere& sphere = m_sphereVerts[sphereIndex];
			if (DoZCylinderAndSphereOverlap3D(cylinder.m_start, cylinder.m_radius, cylinder.m_height, sphere.m_sphereCenter, sphere.m_sphereRadius))
			{
				cylinder.m_color = Rgba8(colorValue, colorValue, colorValue, 255);
				sphere.m_color = Rgba8(colorValue, colorValue, colorValue, 255);
			}
		}
	}

	// Cylinder vs AABB3
	for (int cylinderIndex = 0; cylinderIndex < static_cast<int>(m_cylinders.size()); ++cylinderIndex)
	{
		Cylinder& cylinder = m_cylinders[cylinderIndex];
		for (int aabb3Index = 0; aabb3Index < static_cast<int>(m_aabb3s.size()); ++aabb3Index)
		{
			AABB3D& aabb3 = m_aabb3s[aabb3Index];
			if (DoZCylinderAndAABB3Overlap3D(cylinder.m_start, cylinder.m_radius, cylinder.m_height, AABB3(aabb3.m_mins, aabb3.m_maxs)))
			{
				cylinder.m_color = Rgba8(colorValue, colorValue, colorValue, 255);
				aabb3.m_color = Rgba8(colorValue, colorValue, colorValue, 255);
			}
		}
	}

	// Cylinder vs OBB3
	for (int cylinderIndex = 0; cylinderIndex < static_cast<int>(m_cylinders.size()); ++cylinderIndex)
	{
		Cylinder& cylinder = m_cylinders[cylinderIndex];
		for (int obb3Index = 0; obb3Index < static_cast<int>(m_obb3s.size()); ++obb3Index)
		{
			OBB3D& obb3 = m_obb3s[obb3Index];
			OBB3 orientedBox = OBB3(obb3.m_center, obb3.m_iBasis, obb3.m_jBasis, obb3.m_kBasis, obb3.m_halfDimensions);
			if (DoZCylinderAndOBB3sOverlap3D(cylinder.m_start, cylinder.m_radius, cylinder.m_height, orientedBox))
			{
				cylinder.m_color = Rgba8(colorValue, colorValue, colorValue, 255);
				obb3.m_color = Rgba8(colorValue, colorValue, colorValue, 255);
			}
		}
	}

	// OBB3 vs Sphere
	for (int obb3Index = 0; obb3Index < static_cast<int>(m_obb3s.size()); ++obb3Index)
	{
		OBB3D& obb3 = m_obb3s[obb3Index];
		for (int sphereIndex = 0; sphereIndex < static_cast<int>(m_sphereVerts.size()); ++sphereIndex)
		{
			Sphere& sphere = m_sphereVerts[sphereIndex];
			if (DoOBB3sAndSpheresOverlap3D(OBB3(obb3.m_center, obb3.m_iBasis, obb3.m_jBasis, obb3.m_kBasis, obb3.m_halfDimensions), sphere.m_sphereCenter, sphere.m_sphereRadius))
			{
				obb3.m_color = Rgba8(colorValue, colorValue, colorValue, 255);
				sphere.m_color = Rgba8(colorValue, colorValue, colorValue, 255);
			}
		}
	}

	// OBB3 vs Plane
	for (int obb3Index = 0; obb3Index < static_cast<int>(m_obb3s.size()); ++obb3Index)
	{
		OBB3D& obb3 = m_obb3s[obb3Index];
		for (int planeIndex = 0; planeIndex < static_cast<int>(m_planes.size()); ++planeIndex)
		{
			Plane3D& plane = m_planes[planeIndex];
			if (DoOBB3sAndPlanesOverlap3D(OBB3(obb3.m_center, obb3.m_iBasis, obb3.m_jBasis, obb3.m_kBasis, obb3.m_halfDimensions), Plane3(plane.m_normal, plane.m_distance)))
			{
				obb3.m_color = Rgba8(colorValue, colorValue, colorValue, 255);
			}
		}
	}

	// Plane vs Sphere
	for (int sphereIndex = 0; sphereIndex < static_cast<int>(m_sphereVerts.size()); ++sphereIndex)
	{
		Sphere& sphere = m_sphereVerts[sphereIndex];
		for (int planeIndex = 0; planeIndex < static_cast<int>(m_planes.size()); ++planeIndex)
		{
			Plane3D& plane = m_planes[planeIndex];
			if (DoPlanesAndSpheresOverlap3D(Plane3(plane.m_normal, plane.m_distance), sphere.m_sphereCenter, sphere.m_sphereRadius))
			{
				sphere.m_color = Rgba8(colorValue, colorValue, colorValue, 255);
			}
		}
	}

	// Plane vs AABB3
	for (int aabb3Index = 0; aabb3Index < static_cast<int>(m_aabb3s.size()); ++aabb3Index)
	{
		AABB3D& aabb3 = m_aabb3s[aabb3Index];
		for (int planeIndex = 0; planeIndex < static_cast<int>(m_planes.size()); ++planeIndex)
		{
			Plane3D& plane = m_planes[planeIndex];
			if (DoPlanesAndAABB3sOverlap3D(Plane3(plane.m_normal, plane.m_distance), AABB3(aabb3.m_mins, aabb3.m_maxs)))
			{
				aabb3.m_color = Rgba8(colorValue, colorValue, colorValue, 255);
			}
		}
	}
}

void Game3DTestShapes::GameModeAndControlsText() const
{
	std::vector<Vertex_PCU> textVerts;
	m_font->AddVertsForTextInBox2D(textVerts, "Mode (F6/F7 for Prev/Next): Test Shapes (3D)", m_gameSceneCoords, 15.f, Rgba8::GOLD, 0.8f, Vec2(0.f, 0.97f));
	m_font->AddVertsForTextInBox2D(textVerts, "F8 to Randomize; WASD = Fly Horizontal; QE = Fly Vertical; space = unlock raycast; R to toggle Wireframes; Hold T for slow", m_gameSceneCoords, 15.f, Rgba8::ALICEBLUE, 0.8f, Vec2(0.f, 0.945f));
	g_theRenderer->BindTexture(&m_font->GetTexture());
	g_theRenderer->DrawVertexArray(textVerts);
}