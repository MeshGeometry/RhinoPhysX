// PhysXEngineWrapper.h

#pragma once
#include "PhysXEngine.h"
#include "PxPhysicsAPI.h"
#include "Windows.h"

using namespace System;
using namespace System::Collections::Generic;
using namespace System::Collections;
using namespace Rhino::Geometry;
using namespace System::Runtime::InteropServices;
using namespace System::ComponentModel;

namespace PhysXEngineWrapper {

	public ref class EngineControl abstract sealed 
	{
	public:
		//system methods
		static bool InitPhysics(bool gpu, bool ground, Vector3d gravity);
		static void ShutdownPhysX();
		static void ResetScene();
		static void StepPhysics(float timeStep, int substeps);

		//add actor methods
		static void RemoveActor(System::Guid guid);
		static void RemoveJoint(Guid guid);
		static void AddConvexRigidDynamic(System::Guid guid, Mesh^ meshIn, Point3d pos, bool forceConvex);
		static void AddMultiShapeRigidDynamic(System::Guid guid, cli::array<Mesh^>^ meshIn, bool forceConvex);
		static void AddCompoundConvex(System::Guid guid, Mesh^ meshIn, Point3d pos);
		static void AddCloth(System::Guid guid, Mesh^ meshIn, Point3d pos, double stiffness);
		static void AddEmpty(System::Guid guid, Mesh^ meshIn, Point3d pos);
		static void AddBox(Guid guid, BoundingBox box);
		static void AddSphere(Guid guid, Point3d center, double r);
		static void AddCapsule(Guid guid, Point3d center, double height, double r);
		static void AddFluid(Guid guid, cli::array<Point3d>^ initParticles, int maxParticles, double restitution, double viscosity, double stiffness, double dynamicFriction, double particleDistance, int rate);
		static void AddDistanceJoint(Guid id, Guid fromIdx, Guid toIdx, Point3d p1, Point3d p2,double maxDistance, double minDistance, double stiffness, double breakForce);
		static void AddRevoluteJoint(Guid id, Guid fromIdx, Guid toIdx, Point3d p1, Point3d p2,double maxAngle, double minAngle, double stiffness, double breakForce, double driveVel);
		static void CreateJointFromPair(Guid fromIdx, Guid toIdx, Point3d p1, Point3d p2, double distMult, double linear, double swing, double twist);
		static void AddRigidStatic(System::Guid guid, Mesh^ meshIn, Point3d pos);
		static void LockDOF(System::Guid guid, Point3d pos, int flags);
		
		//return methods
		static cli::array<Transform>^ ReturnRigidResults(cli::array<Guid>^ %idxOut);
		static cli::array<Mesh^>^ ReturnClothResults(cli::array<Guid>^ %idxOut);
		static cli::array<PointCloud^>^ ReturnFluidResults(cli::array<Guid>^ %idxOut);
		static void ReturnActorResults(Guid guid, Transform %transformOut);
		static void ReturnActorResults(Guid guid, Mesh^ %meshOut);

		//other
		static Transform PoseToRhinoTransform(PxMat44 mat);
		static PxMat44 RhinoTransformToPose(Transform x);
		static void SetKinematic(Guid idx, bool state);
		static void SetKinematicPose(Guid idx, Point3d moveVec);
		static void IncrementKinematicPose(Guid idx, Transform x);
		static void SetKinematicPose(Guid idx, Transform x);
		static void SetClothParticleWeights(Guid idx, cli::array<int>^ points, double weight);
		static void SetRigidDynamicMass(Guid idx, double mass);
		static void SetRigidDynamicDrag(Guid idx, double drag);
		static void SetJointEndPoint(Guid idx, int index, Point3d p);
		static Guid Raycast(Point3d origin, Vector3d dir, float %dist, Point3d %hitPoint);
		static Guid FromNativeGUID(const GUID &guid);
		static GUID FromManagedGuid(Guid &guid);
		static Mesh^ ExportCollisionMesh(Guid guid);
		static void SetClothParticlePosition(Guid guid, int index, Point3d newPos, bool release);
		static int FindClosestClothParticle(Guid guid, Point3d pos);
	private:
		static PxVec3 ToPVec3(Point3d pt);
		static Point3d ToPoint3d(PxVec3 pt);
	};
}
