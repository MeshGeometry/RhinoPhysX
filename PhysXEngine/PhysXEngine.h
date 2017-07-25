#include "stdafx.h"
#include <iostream> 
#include <conio.h>
#include <vector>
#include <list>
#include <Windows.h>
#include <map>
#include <PxPhysicsAPI.h>
#include <string>
#include <sstream>

//using namespace std;
using namespace physx;
using namespace std;

namespace PhysXEngine
{	
	//system
	map<PxActor*,GUID> ExportResults();
	//DebugFile debug_file;

	bool InitPhysics(bool gpu, bool ground, PxVec3 gravity);		
	void StepPhysics(float timeStep, int substeps);		
	void ShutdownPhysics();	
	void ResetScene();

	//adding actors to the scene
	void RemoveActor(GUID guid);
	void RemoveJoint(GUID guid);
	void AddConvexRigidDynamic(GUID guid, int ptCount, int triCount, PxVec3* pts, int* tris, PxTransform transform, bool forceConvex);
	void AddCompoundConvex(GUID guid, int ptCount, int triCount, double* pts, int* tris, PxTransform transform); 
	void AddBox(GUID guid, PxTransform transform, PxReal x, PxReal y, PxReal z);
	void AddSphere(GUID guid, PxTransform transform, PxReal r);
	void AddEmpty(GUID guid, int ptCount, int triCount, PxVec3* pts, int* tris, PxTransform transform);
	void AddCapsule(GUID guid, PxTransform transform, PxReal h, PxReal r);
	void AddDistanceJoint(GUID id, GUID fromIdx, GUID toIdx, PxVec3 p1, PxVec3 p2, double maxDistance, double minDistance, double stiffness, double breakForce);
	void AddRevoluteJoint(GUID id, GUID fromIdx, GUID toIdx, PxVec3 p1, PxVec3 p2, double maxAngle, double minAngle, double stiffness, double breakForce, double driveVel);
	void AddJointFromPair(GUID fromIdx, GUID toIdx, PxVec3 p1, PxVec3 p2, double distMultiplier, double linear, double twist, double swing);
	void LockDOF(GUID guid, PxTransform anchor, int flags);
	void AddCloth(GUID guid, int ptCount, int triCount, int qCount, PxVec3* pts, int* tris, int* quads, float* invMasses, PxTransform transform, float stiffness, vector<int> fixedPts);
	void AddMultiShapeRigidDynamic(GUID guid, vector<int> ptCounts, vector<int> triCounts, vector<PxVec3*> ptsVec, vector<int*> trisVec, PxTransform pose, bool forceConvex);
	void AddRigidStatic(GUID guid, int ptCount, int triCount, PxVec3* pts, int* tris, PxTransform transform);
	void AddFluid(GUID id, vector<PxVec3> initPts, PxU32 maxParticles, PxReal restitution, PxReal viscosity, PxReal stiffness, PxReal dynamicFriction, PxReal particleDistance, int rate); 

	//interaction and queries
	void SetKinematic(GUID actorIdx, bool state);
	void SetKinematicPose(GUID actorIdx, PxVec3 translation);
	void SetKinematicPose(GUID actorIdx, PxTransform xform);
	void SetClothParticleWeight(GUID actorIdx, vector<int> fixedPts, double weight);
	void SetClothParticlePosition(GUID actorIdx, int index, PxVec3 newPos, bool release);
	int FindClosestClothParticle(GUID actorIdx, PxVec3 pos);
	void SetRigidDynamicMass(GUID actorIdx, double mass);
	void SetRigidDynamicDrag(GUID actorIdx, double drag);
	void SetJointEndPoints(GUID jointId, int index, PxVec3 pos);
	GUID Raycast(PxVec3 origin, PxVec3 dir, float &dist, PxVec3 &hitPoint);
	PxActor* GetActorFromGUID(GUID id);
	vector<vector<double>> ExportCollisionShape(GUID guid);

	//utilites
	PxConvexMeshGeometry CookConvexMesh(int ptCount, int triCount, PxVec3* pts, int* tris, bool forceConvex);
	PxTriangleMeshGeometry CookTriangleMesh(int ptCount, int triCount, PxVec3* pts, int* tris);

	//debug
	void log(string msg);
	string to_string(int num);
	string to_string(PxVec3 vec);
}