#pragma once
#include "Game/Game.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Math/AABB2.h"
#include "Engine/Math/AABB3.hpp"
#include "Engine/Math/Vec3.h"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Core/Vertex_PCU.h"
#include "Engine/Math/RaycastUtils.hpp"
#include <vector>
// -----------------------------------------------------------------------------
class BitmapFont;
class Texture;
struct Plane3;
// -----------------------------------------------------------------------------
struct Sphere
{
	Vec3 m_sphereCenter = Vec3::ZERO;
	float m_sphereRadius = 0.0f;
	Rgba8 m_color = Rgba8::LIGHTBLUE;
};
struct Cylinder
{
	Vec3 m_start = Vec3::ZERO;
	float m_radius = 0.0f;
	float m_height = 0.0f;
	Rgba8 m_color = Rgba8::LIGHTBLUE;
};
struct AABB3D
{
	Vec3 m_mins = Vec3::ZERO;
	Vec3 m_maxs = Vec3::ZERO;
	Rgba8 m_color = Rgba8::LIGHTBLUE;
};
struct OBB3D
{
	Vec3 m_center = Vec3::ZERO;
	Vec3 m_iBasis = Vec3::ZERO;
	Vec3 m_jBasis = Vec3::ZERO;
	Vec3 m_kBasis = Vec3::ZERO;
	Vec3 m_halfDimensions = Vec3::ZERO;
	Rgba8 m_color = Rgba8::LIGHTBLUE;
	EulerAngles m_orientation = EulerAngles::ZERO;
};
struct Plane3D
{
	Vec3 m_normal = Vec3::ZAXE;
	float m_distance = 0.0f;
};
// -----------------------------------------------------------------------------
const int NUM_SPHERES = 4;
const int NUM_AABB3S = 2;
const int NUM_CYLINDERS = 4;
const int NUM_OBB3S = 3;
const int NUM_PLANES = 1;
// -----------------------------------------------------------------------------
class Game3DTestShapes : public Game
{
public:
	Game3DTestShapes(App* owner);

	void Update(float deltaSeconds) override;

	void ToggleGrabObject();
	void LockPosition();

	void CameraKeyPresses(float deltaSeconds);
	void Render() const override;

	Mat44 GetModelToWorldTransform() const;
	Vec3  GetForwardNormal() const;

private:
	void DrawSphere() const;
	void DrawAABB3() const;
	void DrawCylinder() const;
	void DrawOBB3() const;
	void DrawPlane() const;
	void DrawBasis() const;
	void DrawRaycast() const;
	void DrawGrabbedObject(int shapeIndex) const;
	void AddVertsForPlane3D(std::vector<Vertex_PCU>& verts, Plane3 const& plane) const;

private:
	void RandomizeShapes();
	void ToggleRasterizerMode();
	
	void RenderNearestPoint(Vec3 const& point) const;
	void GetNearestPointCheck();
	void GetNearestPointForRefPosition();
	void GetPointClosestToPlayer();

	void ShapevsShapeOverlap(float deltaSeconds);

	void GameModeAndControlsText() const;

private:
	App*	  m_theApp = nullptr;
	BitmapFont* m_font = nullptr;
	Texture* m_texture = nullptr;
	Vec3 m_rayCastStart = Vec3::ZERO;
	Vec3 m_rayCastEnd   = Vec3::ZERO;
	AABB2 m_gameSceneCoords;
	AABB3 m_shapesSpawnZone;

	RasterizerMode m_currentRasterizerMode = RasterizerMode::SOLID_CULL_BACK;
	bool		   m_isSolidShapeTexture = true;

	Vec3 m_position = Vec3::ZERO;
	Vec3 m_refPosition = Vec3::ZERO;
	Vec3 m_grabbedObjectOffset = Vec3::ZERO;
	EulerAngles m_orientation = EulerAngles(0.f, 0.f, 0.f);
	std::vector<Sphere> m_sphereVerts;
	std::vector<AABB3D> m_aabb3s;
	std::vector<Cylinder> m_cylinders;
	std::vector<OBB3D> m_obb3s;
	std::vector<Plane3D> m_planes;
	
	float m_colorBrightness = 0.f;
	int   m_grabbedObjectIndex = -1;
	bool  m_isPositionLocked = false;
	bool  m_isObjectGrabbed = false;

	// Nearest points
	Vec3			  m_closestPointToPlayer = Vec3::ZERO;
	std::vector<Vec3> m_nearestSpherePoints;
	std::vector<Vec3> m_nearestAABB3Points;
	std::vector<Vec3> m_nearestCylinderPoints;
	std::vector<Vec3> m_nearestOBB3Points;
	std::vector<Vec3> m_nearestPlanePoints;

	// Shape identifiers
	bool m_isSphere = false;
	bool m_isAABB3 = false;
	bool m_isCylinder = false;
	bool m_isOBB3 = false;
};