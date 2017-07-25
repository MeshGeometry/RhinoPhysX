// This is the main DLL file.

#include "stdafx.h"
#include "PhysXEngineWrapper.h"
#include "ClothHelpers.h"
#include <Windows.h>
#include <map>
#include <fstream>
#include <iostream>

using namespace System::Collections;
using namespace System::Drawing;
using namespace Rhino::Geometry;

namespace PhysXEngineWrapper {

	bool EngineControl::InitPhysics(bool gpu, bool ground, Vector3d gravity)
	{
		PxVec3 g(gravity.X, gravity.Z, -gravity.Y);
		return PhysXEngine::InitPhysics(gpu, ground, g);
	}

	void EngineControl::ShutdownPhysX()
	{
		PhysXEngine::ShutdownPhysics();
	}

	cli::array<Mesh^>^ EngineControl::ReturnClothResults(cli::array<Guid>^ %idxOut)
	{
		map<PxActor*, GUID> allActors = PhysXEngine::ExportResults();	
		int count = allActors.size();
		cli::array<Mesh^>^ meshesOut = gcnew cli::array<Mesh^>(0);
		idxOut = gcnew cli::array<Guid>(0);


		for(map<PxActor*, GUID>::iterator it = allActors.begin(); it != allActors.end(); ++it)
		{
			PxCloth* tmpCloth = it->first->is<PxCloth>();

			if(tmpCloth == NULL)
				continue;

			Array::Resize(meshesOut, meshesOut->Length + 1);
			Array::Resize(idxOut, idxOut->Length + 1);
			idxOut[idxOut->Length - 1] = FromNativeGUID(it->second);

			vector<PxVec3> posOut;
			PhysXEngine::ClothHelpers::getParticlePositions(*tmpCloth, posOut);

			meshesOut[meshesOut->Length - 1] = gcnew Mesh();

			for(int j = 0; j< posOut.size();j++)
			{
				meshesOut[meshesOut->Length - 1]->Vertices->Add(Point3f(posOut[j].x, -posOut[j].z, posOut[j].y));
			}
		}

		return meshesOut;
	}

	void EngineControl::AddBox(Guid guid, BoundingBox box)
	{
		PxVec3 pos(box.Center.X, box.Center.Z, -box.Center.Y);
		PxReal x = 0.5 * (box.Max.X - box.Min.X);
		PxReal y = 0.5 * (box.Max.Z - box.Min.Z);
		PxReal z = 0.5 * (box.Max.Y - box.Min.Y);

		PxTransform pose(pos, PxQuat(PxIdentity));

		PhysXEngine::AddBox(FromManagedGuid(guid), pose, x, y, z);
	}

	void EngineControl::AddSphere(Guid guid, Point3d center, double r)
	{
		PxVec3 pos(center.X, center.Z, -center.Y);
		PxTransform pose(pos, PxQuat(PxIdentity));

		PhysXEngine::AddSphere(FromManagedGuid(guid), pose, (PxReal) r);
	}

	void EngineControl::AddCapsule(Guid guid, Point3d center, double height, double r)
	{
		PxVec3 pos(center.X, center.Z, -center.Y);
		PxTransform pose(pos, PxQuat(PxIdentity));

		PhysXEngine::AddCapsule(FromManagedGuid(guid), pose, (PxReal) height, (PxReal) r);
	}

	void EngineControl::AddCloth(Guid guid, Mesh^ meshIn, Point3d pos, double stiffness)
	{
		meshIn->Weld(3.0);

		int vertCount = meshIn->Vertices->Count;
		vector<int> quads;
		vector<int> tris;
		float* invMass = new float[vertCount];

		for(int i = 0; i < meshIn->Faces->Count; i++)
		{
			MeshFace f = meshIn->Faces[i];
			if(f.IsQuad)
			{
				quads.push_back(f.A);
				quads.push_back(f.B);
				quads.push_back(f.C);
				quads.push_back(f.D);
			}
			else
			{
				tris.push_back(f.A);
				tris.push_back(f.B);
				tris.push_back(f.C);
			}
		}

		//add the verts
		PxVec3* pts = new PxVec3[vertCount];
		for(int i = 0; i < vertCount; i++)
		{
			Point3d tmp = meshIn->Vertices[i];
			tmp += (Vector3d)(-pos);
			pts[i] = PxVec3(tmp.X, tmp.Z, -tmp.Y);

			//assign inv masses
			invMass[i] = 0.5;
		}

		PxTransform pose = PxTransform(PxVec3(pos.X, pos.Z, -pos.Y), PxQuat(PxIdentity));
		vector<int> fixedPts;

		int* quads_ptr = new int[quads.size()];
		for(int i = 0; i < quads.size(); i++)
			quads_ptr[i] = quads[i];
		int* tri_ptr = new int[tris.size()];
		for(int i = 0; i < tris.size(); i++)
			tri_ptr[i] = tris[i];

		PhysXEngine::AddCloth(FromManagedGuid(guid), vertCount, tris.size()/3, quads.size()/4, pts, tri_ptr,quads_ptr, invMass, pose, (float) stiffness, fixedPts);
		delete[] quads_ptr;
		delete[] tri_ptr;
		delete[] pts;
	}

	void EngineControl::SetClothParticlePosition(Guid guid, int index, Point3d newPos, bool release)
	{
		PhysXEngine::SetClothParticlePosition(FromManagedGuid(guid), index, ToPVec3(newPos), release);
	}

	int EngineControl::FindClosestClothParticle(Guid guid, Point3d pos)
	{
		int id = PhysXEngine::FindClosestClothParticle(FromManagedGuid(guid), ToPVec3(pos));
		return id;
	}

	void EngineControl::SetClothParticleWeights(Guid idx, cli::array<int>^ points, double weight)
	{
		vector<int> indices;

		for(int i = 0; i < points->Length; i++)
		{
			int idx = points[i];
			indices.push_back(idx);
		}

		PhysXEngine::SetClothParticleWeight(FromManagedGuid(idx), indices, weight);
	}

	void EngineControl::AddMultiShapeRigidDynamic(System::Guid guid, cli::array<Mesh^>^ meshIn, bool forceConvex)
	{
		if(meshIn->Length == 0)
			return;

		vector<int> ptCounts;
		vector<int> triCounts;
		vector<PxVec3*> ptsVec;
		vector<int*> trisVec;

		PxTransform pose;
		Point3d avgPos(0,0,0);
		//find average pos;
		for(int i = 0; i < meshIn->Length; i++)
		{
			avgPos += meshIn[i]->GetBoundingBox(true).Center;
		}
		avgPos /= meshIn->Length;

		for(int i = 0; i < meshIn->Length; i++)
		{
			int vertCount = meshIn[i]->Vertices->Count;
			int triCount = meshIn[i]->Faces->Count;

			int* tris = new int[triCount * 3];

			for(int j = 0; j < triCount; j++)
			{
				MeshFace f = meshIn[i]->Faces[j];
				tris[3 * j] = f.A;
				tris[3 * j + 1] = f.B;
				tris[3 * j + 2] = f.C;
			}

			PxVec3* pts = new PxVec3[vertCount];

			for(int j = 0; j < vertCount; j++)
			{
				Point3d tmp = meshIn[i]->Vertices[j];
				tmp += -avgPos;
				pts[j] = PxVec3(tmp.X, tmp.Z, -tmp.Y);
			}

			//objects to vectors
			ptCounts.push_back(vertCount);
			triCounts.push_back(triCount);
			ptsVec.push_back(pts);
			trisVec.push_back(tris);
		}

		pose = PxTransform(PxVec3(avgPos.X, avgPos.Z, -avgPos.Y), PxQuat(PxIdentity));

		PhysXEngine::AddMultiShapeRigidDynamic(FromManagedGuid(guid), ptCounts, triCounts, ptsVec,trisVec, pose, forceConvex);
	}

	void EngineControl::AddFluid(Guid guid, cli::array<Point3d>^ initParticles, int maxParticles, 
		double restitution, double viscosity, double stiffness, 
		double dynamicFriction, double particleDistance, int rate)
	{
		vector<PxVec3> initPos;

		for(int i = 0; i < initParticles->Length; i++)
		{
			Point3d tmp = initParticles[i];
			initPos.push_back(PxVec3(tmp.X, tmp.Z, -tmp.Y));
		}

		PhysXEngine::AddFluid(FromManagedGuid(guid), initPos, maxParticles, restitution, viscosity, stiffness, dynamicFriction, particleDistance, rate);
	}

	void EngineControl::SetRigidDynamicMass(Guid idx, double mass)
	{
		PhysXEngine::SetRigidDynamicMass(FromManagedGuid(idx), mass);
	}

	void EngineControl::SetRigidDynamicDrag(Guid idx, double drag)
	{
		PhysXEngine::SetRigidDynamicDrag(FromManagedGuid(idx), drag);
	}

	cli::array<PointCloud^>^ EngineControl::ReturnFluidResults(cli::array<Guid>^ %idxOut)
	{
		map<PxActor*, GUID> allActors = PhysXEngine::ExportResults();	
		int count = allActors.size();
		cli::array<PointCloud^>^ cloudsOut = gcnew cli::array<PointCloud^>(0);
		idxOut = gcnew cli::array<Guid>(0);

		for(map<PxActor*, GUID>::iterator it = allActors.begin(); it != allActors.end(); ++it)
		{
			PxParticleFluid* tmpFluid = it->first->is<PxParticleFluid>();

			if(tmpFluid == NULL)
				continue;

			//resice the arrays
			Array::Resize(cloudsOut, cloudsOut->Length + 1);
			Array::Resize(idxOut, idxOut->Length + 1);
			//add the relevant guid for use in the Rhino plugin
			idxOut[idxOut->Length - 1] = FromNativeGUID(it->second);

			cloudsOut[cloudsOut->Length - 1] = gcnew PointCloud();

			//get fluid results
			PxParticleFluidReadData* data = tmpFluid -> lockParticleFluidReadData();

			PxStrideIterator<const PxVec3> positions(data->positionBuffer);
			PxStrideIterator<const PxVec3> velocities(data->velocityBuffer);
			PxStrideIterator<const PxF32> densities(data->densityBuffer);

			for(int i = 0; i < data->nbValidParticles; i++)
			{
				Point3d pt(positions[i].x, -positions[i].z, positions[i].y);

				double d;

				if(densities.ptr() == NULL)
					d = 0.0;
				else
					d = (double)densities[i];

				if(d == NULL)
					d = 0.0;

				d = Math::Min(0.8, d);
				d = Math::Max(0.1, d);

				d = 1.0 - d;

				int delR = (int)(200.0 * d);
				int delG = (int)(200.0* d);
				int delB = 200;


				Color baseCol = Color::FromArgb(255, delR, delG, delB);

				cloudsOut[cloudsOut->Length - 1]->Add(pt, baseCol);
			}

			data->unlock();

		}

		allActors.clear();

		return cloudsOut;
	}

	void EngineControl::AddEmpty(System::Guid guid, Mesh^ meshIn, Point3d pos)
	{
		meshIn->Faces->ConvertQuadsToTriangles();
		meshIn->Weld(3.0);
		int vertCount = meshIn->Vertices->Count;
		int triCount = meshIn->Faces->Count;

		int* tris = new int[triCount * 3];

		for(int i = 0; i < triCount; i++)
		{
			MeshFace f = meshIn->Faces[i];
			tris[3 * i] = f.A;
			tris[3 * i + 1] = f.B;
			tris[3 * i + 2] = f.C;
		}

		PxVec3* pts = new PxVec3[vertCount];

		for(int i = 0; i < vertCount; i++)
		{
			Point3d tmp = meshIn->Vertices[i];
			tmp += (Vector3d)(-pos);
			pts[i] = PxVec3(tmp.X, tmp.Z, -tmp.Y);
		}

		PxTransform pose = PxTransform(PxVec3(pos.X, pos.Z, -pos.Y), PxQuat(PxIdentity));

		PhysXEngine::AddEmpty(FromManagedGuid(guid), vertCount, triCount, pts, tris, pose);

		delete[] tris;
		delete[] pts;
	}
	void EngineControl::AddConvexRigidDynamic(Guid guid, Mesh^ meshIn, Point3d pos, bool forceConvex)
	{
		meshIn->Faces->ConvertQuadsToTriangles();
		meshIn->Weld(3.0);
		int vertCount = meshIn->Vertices->Count;
		int triCount = meshIn->Faces->Count;

		int* tris = new int[triCount * 3];

		for(int i = 0; i < triCount; i++)
		{
			MeshFace f = meshIn->Faces[i];
			tris[3 * i] = f.A;
			tris[3 * i + 1] = f.B;
			tris[3 * i + 2] = f.C;
		}

		PxVec3* pts = new PxVec3[vertCount];

		for(int i = 0; i < vertCount; i++)
		{
			Point3d tmp = meshIn->Vertices[i];
			tmp += (Vector3d)(-pos);
			pts[i] = PxVec3(tmp.X, tmp.Z, -tmp.Y);
		}

		PxTransform pose = PxTransform(PxVec3(pos.X, pos.Z, -pos.Y), PxQuat(PxIdentity));

		PhysXEngine::AddConvexRigidDynamic(FromManagedGuid(guid), vertCount, triCount, pts, tris, pose, forceConvex);

		delete[] tris;
		delete[] pts;
	}

	void EngineControl::AddCompoundConvex(System::Guid guid, Mesh^ meshIn, Point3d pos)
	{

		meshIn->Faces->ConvertQuadsToTriangles();
		meshIn->Weld(3.0);
		int vertCount = meshIn->Vertices->Count;
		int triCount = meshIn->Faces->Count;
		int* tris = new int[triCount * 3];
		double* pts = new double[vertCount * 3];


		for(int i = 0; i < triCount; i++)
		{
			MeshFace f = meshIn->Faces[i];
			tris[3 * i] = f.A;
			tris[3 * i + 1] = f.B;
			tris[3 * i + 2] = f.C;
		}

		for(int i = 0; i < vertCount; i++)
		{
			Point3d tmp = meshIn->Vertices[i];
			tmp += (Vector3d)(-pos);
			pts[3 * i] = tmp.X;
			pts[3 * i + 1] = tmp.Z;
			pts[3 * i + 2] = -tmp.Y;
		}

		PxTransform pose = PxTransform(PxVec3(pos.X, pos.Z, -pos.Y), PxQuat(PxIdentity));

		PhysXEngine::AddCompoundConvex(FromManagedGuid(guid), vertCount, triCount, pts, tris, pose);

		delete[] tris;
		delete[] pts;
	}

	void EngineControl::AddRigidStatic(System::Guid guid, Mesh^ meshIn, Point3d pos)
	{
		meshIn->Faces->ConvertQuadsToTriangles();
		meshIn->Weld(3.0);

		Mesh^ flipped = gcnew Mesh();
		flipped->CopyFrom(meshIn);
		flipped->Flip(true, true, true);
		meshIn->Append(flipped);

		int vertCount = meshIn->Vertices->Count;
		int triCount = meshIn->Faces->Count;

		int* tris = new int[triCount * 3];

		for(int i = 0; i < triCount; i++)
		{
			MeshFace f = meshIn->Faces[i];
			tris[3 * i] = f.A;
			tris[3 * i + 1] = f.B;
			tris[3 * i + 2] = f.C;
		}

		PxVec3* pts = new PxVec3[vertCount];

		for(int i = 0; i < vertCount; i++)
		{
			Point3d tmp = meshIn->Vertices[i];
			tmp += (Vector3d)(-pos);
			pts[i] = PxVec3(tmp.X, tmp.Z, -tmp.Y);
		}

		PxTransform pose = PxTransform(PxVec3(pos.X, pos.Z, -pos.Y), PxQuat(PxIdentity));

		PhysXEngine::AddRigidStatic(FromManagedGuid(guid), vertCount, triCount, pts, tris, pose);
	}


	Guid EngineControl::FromNativeGUID(const GUID &guid)
	{
		return *reinterpret_cast<Guid *>(const_cast<GUID *>(&guid));
	}

	GUID EngineControl::FromManagedGuid(Guid &guid)
	{
		return *reinterpret_cast<GUID *>(const_cast<Guid *>(&guid));
	}

	void EngineControl::StepPhysics(float timeStep, int substeps)
	{		
		PhysXEngine::StepPhysics(timeStep, substeps);
	}

	void EngineControl::ResetScene()
	{
		PhysXEngine::ResetScene();
	}

	void EngineControl::ReturnActorResults(Guid guid, Transform %transformOut)
	{
		PxActor* actor = PhysXEngine::GetActorFromGUID(FromManagedGuid(guid));
		PxRigidActor* rigidActor = actor->is<PxRigidActor>();
		if(rigidActor == NULL)
			return;

		PxMat44 poseMat(rigidActor->getGlobalPose());
		transformOut = PoseToRhinoTransform(poseMat);
	}

	void EngineControl::RemoveActor(System::Guid guid)
	{
		PhysXEngine::RemoveActor(FromManagedGuid(guid));
	}

	void EngineControl::ReturnActorResults(Guid guid, Mesh^ %meshOut)
	{
		PxActor* actor = PhysXEngine::GetActorFromGUID(FromManagedGuid(guid));
		PxCloth* clothActor = actor->is<PxCloth>();
		if(clothActor == NULL)
			return;

		vector<PxVec3> posOut;
		PhysXEngine::ClothHelpers::getParticlePositions(*clothActor, posOut);

		//modifiy input vertices;
		for(int j = 0; j< posOut.size();j++)
			meshOut->Vertices[j] = Point3f(posOut[j].x, -posOut[j].z, posOut[j].y);

	}

	Mesh^ EngineControl::ExportCollisionMesh(Guid guid)
	{
		vector<vector<double>> ptsOut = PhysXEngine::ExportCollisionShape(FromManagedGuid(guid));
		Mesh^ meshOut = gcnew Mesh();
		for(int i = 0; i < ptsOut.size(); i++)
		{
			cli::array<Point3d>^ facePts = gcnew cli::array<Point3d>(ptsOut[i].size()/3 + 1);
			for(int j = 0; j < ptsOut[i].size()/3; j++)	
				facePts[j] = Point3d(ptsOut[i][3*j], -ptsOut[i][3*j + 2], ptsOut[i][3*j + 1]);
			facePts[ptsOut[i].size()/3] = facePts[0];
			Rhino::Geometry::Polyline^ facePoly = gcnew Rhino::Geometry::Polyline(facePts);
			Mesh^ faceMesh  = Mesh::CreateFromClosedPolyline(facePoly);
			meshOut->Append(faceMesh);
		}
		meshOut->Normals->ComputeNormals();
		meshOut->UnifyNormals();

		return meshOut;
	}

	cli::array<Transform>^ EngineControl::ReturnRigidResults(cli::array<Guid>^ %idxOut)
	{
		map<PxActor*, GUID> allActors = PhysXEngine::ExportResults();

		int count = allActors.size();
		cli::array<Transform>^ transformArr = gcnew cli::array<Transform>(0);
		idxOut = gcnew cli::array<Guid>(0);

		for(map<PxActor*, GUID>::iterator it = allActors.begin(); it != allActors.end(); ++it)
		{
			PxRigidDynamic* tmpRD = it->first->is<PxRigidDynamic>();

			if(tmpRD == NULL)
				continue;

			Array::Resize(transformArr, transformArr->Length + 1);
			Array::Resize(idxOut, idxOut->Length + 1);
			idxOut[idxOut->Length - 1] = FromNativeGUID(it->second);

			PxMat44 poseMat(tmpRD->getGlobalPose());
			transformArr[transformArr->Length - 1] = PoseToRhinoTransform(poseMat);

		}

		return transformArr;
	}

	Guid EngineControl::Raycast(Point3d origin, Vector3d dir, float %dist, Point3d %hitPoint)
	{
		PxVec3 newOrigin = PxVec3(origin.X, origin.Z, -origin.Y);
		PxVec3 newDir = PxVec3(dir.X, dir.Z, -dir.Y);

		float tmpDist;
		PxVec3 tmpHitPoint;
		GUID id = PhysXEngine::Raycast(newOrigin, newDir, tmpDist, tmpHitPoint);

		dist = tmpDist;
		hitPoint = Point3d(tmpHitPoint.x, -tmpHitPoint.z, tmpHitPoint.y);

		return FromNativeGUID(id);
	}

	void EngineControl::RemoveJoint(Guid id)
	{
		PhysXEngine::RemoveJoint(FromManagedGuid(id));
	}

	void EngineControl::SetJointEndPoint(Guid idx,int index, Point3d p)
	{
		PxVec3 px = ToPVec3(p);
		PhysXEngine::SetJointEndPoints(FromManagedGuid(idx), index, px);
	}
	void EngineControl::CreateJointFromPair(Guid fromIdx, Guid toIdx, Point3d p1, Point3d p2, double distMult, double linear, double swing, double twist)
	{
		PxVec3 px1 = ToPVec3(p1);
		PxVec3 px2 = ToPVec3(p2);
		PhysXEngine::AddJointFromPair(FromManagedGuid(fromIdx), FromManagedGuid(toIdx), px1, px2, distMult, linear, twist, swing);
	}

	void EngineControl::AddDistanceJoint(Guid id, Guid fromIdx, Guid toIdx, Point3d p1, Point3d p2,double maxDistance, double minDistance, double stiffness, double breakForce)
	{
		PxVec3 px1 = ToPVec3(p1);
		PxVec3 px2 = ToPVec3(p2);
		PhysXEngine::AddDistanceJoint(FromManagedGuid(id), FromManagedGuid(fromIdx), FromManagedGuid(toIdx), px1, px2, maxDistance, minDistance, stiffness, breakForce);
	}

	void EngineControl::AddRevoluteJoint(Guid id, Guid fromIdx, Guid toIdx, Point3d p1, Point3d p2,double maxAngle, double minAngle, double stiffness, double breakForce, double driveVel)
	{
		PxVec3 px1 = ToPVec3(p1);
		PxVec3 px2 = ToPVec3(p2);
		PhysXEngine::AddRevoluteJoint(FromManagedGuid(id), FromManagedGuid(fromIdx), FromManagedGuid(toIdx), px1, px2, maxAngle, minAngle, stiffness, breakForce, driveVel);
	}

	void EngineControl::LockDOF(System::Guid guid, Point3d pos, int flags)
	{
		PxVec3 newPos = PxVec3(pos.X, pos.Z, -pos.Y);
		PxTransform anchor = PxTransform(newPos);
		PhysXEngine::LockDOF(FromManagedGuid(guid), anchor, flags);
	}

	void EngineControl::SetKinematic(Guid idx, bool state)
	{

		PhysXEngine::SetKinematic(FromManagedGuid(idx), state);
	}

	void EngineControl::SetKinematicPose(Guid idx, Point3d moveVec)
	{

		PxVec3 tmpVec = PxVec3(moveVec.X, moveVec.Z, -moveVec.Y); 
		PhysXEngine::SetKinematicPose(FromManagedGuid(idx), tmpVec);
	}

	void EngineControl::SetKinematicPose(Guid idx, Transform x)
	{
		GUID guid = FromManagedGuid(idx);
		PxRigidDynamic* actor = PhysXEngine::GetActorFromGUID(guid)->is<PxRigidDynamic>();
		PxTransform globalPose =  actor -> getGlobalPose();
		PxMat44 physxTrans;

		//if(!((int)x.M00 == 1 && (int)x.M01 == 0 && (int)x.M02 == 0 && 
		//	(int)x.M10 == 0 && (int)x.M11 == 1 && (int)x.M12 == 0 && 
		//	(int)x.M20 == 0 && (int)x.M21 == 0 && (int)x.M22 == 1))
		//{
		//	//need to zero out the translation part of a rhino gumball rotation
		//	x.M03 = 0; x.M13 = 0; x.M23 = 0;
		//}

		PxTransform xform = PxTransform(RhinoTransformToPose(x));
		PxTransform fullX = PxTransform(xform.p, xform.q);
		actor -> setKinematicTarget(fullX);
	}

	void EngineControl::IncrementKinematicPose(Guid idx, Transform x)
	{
		GUID guid = FromManagedGuid(idx);
		PxRigidDynamic* actor = PhysXEngine::GetActorFromGUID(guid)->is<PxRigidDynamic>();
		PxTransform globalPose =  actor -> getGlobalPose();
		PxMat44 physxTrans;

		if(!(Math::Abs(x.M00 - 1) < 0.001 && Math::Abs(x.M01) < 0.001 && Math::Abs(x.M02) < 0.001 && 
			Math::Abs(x.M10) < 0.001 && Math::Abs(x.M11 - 1) < 0.001 && Math::Abs(x.M12) < 0.001 && 
			Math::Abs(x.M20) < 0.001 && Math::Abs(x.M21) < 0.001 && Math::Abs(x.M22 - 1) < 0.001))
		{
			//need to zero out the translation part of a rhino gumball rotation
			x.M03 = 0; x.M13 = 0; x.M23 = 0;
		}

		PxTransform xform = PxTransform(RhinoTransformToPose(x));
		PxTransform fullX = PxTransform(globalPose.p + xform.p, xform.q * globalPose.q);
		actor -> setKinematicTarget(fullX);
	}


	Transform EngineControl::PoseToRhinoTransform(PxMat44 mat)
	{
		Transform rot(Transform::Rotation(0.5 * 3.141592, Vector3d::XAxis, Point3d::Origin));
		Transform rotInv(Transform::Rotation(-0.5 * 3.141592, Vector3d::XAxis, Point3d::Origin));

		Transform x;

		x.M00 = (double)mat.column0.x; x.M01 = (double)mat.column1.x; x.M02 = (double)mat.column2.x; x.M03 = (double)mat.column3.x;
		x.M10 = (double)mat.column0.y; x.M11 = (double)mat.column1.y; x.M12 = (double)mat.column2.y; x.M13 = (double)mat.column3.y;
		x.M20 = (double)mat.column0.z; x.M21 = (double)mat.column1.z; x.M22 = (double)mat.column2.z; x.M23 = (double)mat.column3.z;
		x.M30 = (double)mat.column0.w; x.M31 = (double)mat.column1.w; x.M32 = (double)mat.column2.w; x.M33 = (double)mat.column3.w;

		//PhysX is Y-up, so rotate by 90 deg around X axis and apply world transform
		x = Transform::Multiply(rot, x);

		//rotate back
		x = Transform::Multiply(x, rotInv);

		return x;
	}

	PxMat44 EngineControl::RhinoTransformToPose(Transform x)
	{
		Transform rot(Transform::Rotation(0.5 * 3.141592, Vector3d::XAxis, Point3d::Origin));
		Transform rotInv(Transform::Rotation(-0.5 * 3.141592, Vector3d::XAxis, Point3d::Origin));

		//Rhino is Z-up, so rotate by -90 deg around X axis and apply world transform
		//x = Transform::Multiply(rotInv, x);

		//rotate back
		//x = Transform::Multiply(x, rot);

		PxMat44 mat;
		mat.column0.x = (PxReal)x.M00; mat.column1.x = (PxReal)x.M01; mat.column2.x = (PxReal)x.M02; mat.column3.x = (PxReal)x.M03;
		mat.column0.y = (PxReal)x.M10; mat.column1.y = (PxReal)x.M11; mat.column2.y = (PxReal)x.M12; mat.column3.y = (PxReal)x.M13;
		mat.column0.z = (PxReal)x.M20; mat.column1.z = (PxReal)x.M21; mat.column2.z = (PxReal)x.M22; mat.column3.z = (PxReal)x.M23;
		mat.column0.w = (PxReal)x.M30; mat.column1.w = (PxReal)x.M31; mat.column2.w = (PxReal)x.M32; mat.column3.w = (PxReal)x.M33;

		PxMat44 rotXmat = PxMat44(PxQuat(0.5 * 3.141592, PxVec3(1,0,0)));
		PxMat44 rotXInvmat = PxMat44(PxQuat(-0.5 * 3.141592, PxVec3(1,0,0)));

		mat = rotXInvmat * mat;
		mat = mat * rotXmat;

		return mat;
	}

	PxVec3 EngineControl::ToPVec3(Point3d pt)
	{
		return PxVec3(pt.X, pt.Z, -pt.Y); 
	}
	Point3d EngineControl::ToPoint3d(PxVec3 pt)
	{
		return Point3d(pt.x, -pt.z, pt.y);
	}

}
