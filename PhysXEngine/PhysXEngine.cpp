#include "stdafx.h"
#include "PhysXEngine.h"

#include <vector>
#include <map>
#include <Windows.h>
#include <iostream>
#include <string>
#include "ClothHelpers.h"
#include "ParticleSystem.h"
#include "ParticleFactory.h"
#include "FluidHelper.h"
#include "ClickableTrigger.h"
#include <time.h>
#include "Debug.h"

using namespace std;
using namespace physx;

namespace PhysXEngine
{
	PxPhysics* gPhysicsSDK = NULL;
	PxFoundation* gFoundation = NULL;
	PxDefaultErrorCallback gDefaultErrorCallback;
	PxDefaultAllocator gDefaultAllocatorCallback;
	PxScene* gScene = NULL;

	PxReal gTimeStep = 1.0f/120.0f;
	PxCudaContextManager* cm;
	PxCooking* cooking;
	PxMaterial* defaultMat;

	bool useGpu;

	vector<ParticleSystem> allParticleSystems;
	vector<FluidHelper*> allFluidHelpers;
	vector<ClickableTrigger*> allClickableTriggers;

	map<PxActor*, GUID> actorObjMap;
	map<PxJoint*, GUID> jointMap;

	string to_string(int num)
	{
		string Result;          // string which will contain the result
		ostringstream convert;   // stream used for the conversion
		convert << num;      // insert the textual representation of 'Number' in the characters in the stream
		return convert.str();
	}

	string to_string(PxVec3 vec)
	{
		string Result;          // string which will contain the result
		ostringstream convert;   // stream used for the conversion
		convert << vec.x<<", ";
		convert << vec.y<<", ";  
		convert << vec.z;  // insert the textual representation of 'Number' in the characters in the stream
		return convert.str();
	}

	void log(string msg)
	{
#ifdef _DEBUG
		fstream debugFile;
		debugFile.open("C:\\Users\\Daniel_office\\Desktop\\RhinoPhysics_Debug.txt", ios::app);
		debugFile<<msg.c_str();
		debugFile.close();
#endif
	}

	bool InitPhysics(bool gpu, bool ground, PxVec3 gravity)
	{

		time_t now;
		struct tm newyear;
		double seconds;

		time(&now);  /* get current time; same as: now = time(NULL)  */
		newyear = *localtime(&now);
		newyear.tm_hour = 0; newyear.tm_min = 0; newyear.tm_sec = 0;
		newyear.tm_mon = 10;  newyear.tm_mday = 1; newyear.tm_year = 118;

		seconds = difftime(mktime(&newyear),now);

		if(seconds < 0)
			return false;

		log("----------------new session------------------\n");
		//Creating foundation for PhysX
		gFoundation = PxCreateFoundation(PX_FOUNDATION_VERSION, gDefaultAllocatorCallback, gDefaultErrorCallback);
		//Creating instance of PhysX SDK
		PxTolerancesScale scale;
		gPhysicsSDK = PxCreatePhysics(PX_PHYSICS_VERSION, *gFoundation, scale);

		//Creating scene
		PxSceneDesc sceneDesc(scale);	//Descriptor class for scenes 

		//Creating Cooking
		PxCookingParams params(gPhysicsSDK->getTolerancesScale());
		params.meshWeldTolerance = 0.001f;
		params.meshPreprocessParams = PxMeshPreprocessingFlags(PxMeshPreprocessingFlag::eWELD_VERTICES);
		cooking = PxCreateCooking(PX_PHYSICS_VERSION, *gFoundation, params);

		//Set gravity
		sceneDesc.gravity = gravity;		//Setting gravity

		//Set up dispatchers
		sceneDesc.cpuDispatcher = PxDefaultCpuDispatcherCreate(2);

		//gpu check
		useGpu = gpu;
		if(useGpu)
		{
			PxCudaContextManagerDesc cmd;
			cm = PxCreateCudaContextManager(*gFoundation, cmd);
			if(cm != NULL)
			{
				sceneDesc.gpuDispatcher = cm->getGpuDispatcher();
				log("initializd the gpu");
			}
		}

		sceneDesc.filterShader  = PxDefaultSimulationFilterShader;	//Creates default collision filter shader for the scene
		gScene = gPhysicsSDK->createScene(sceneDesc);	//Creates a scene 

		//scene params
		gScene->setClothInterCollisionDistance(2);
		gScene->setClothInterCollisionStiffness(1.0);
		gScene->setClothInterCollisionNbIterations(25);

		//Create default PhysX material
		defaultMat = gPhysicsSDK->createMaterial(0.1,0.1,0.1); //Creating a PhysX material

		//1-Creating static plane	
		if(ground)
		{
			PxTransform planePos =	PxTransform(PxVec3(0.0f),PxQuat(PxHalfPi, PxVec3(0.0f, 0.0f, 1.0f)));	//Position and orientation(transform) for plane actor  
			PxRigidStatic* plane =  gPhysicsSDK->createRigidStatic(planePos);								//Creating rigid static actor	
			plane->createShape(PxPlaneGeometry(), *defaultMat);												//Defining geometry for plane actor
			gScene->addActor(*plane);//Adding plane actor to PhysX scene
		}

		if(gPhysicsSDK == NULL| gFoundation == NULL | gScene == NULL)
			return false;
		else
			return true;
	}

	void StepPhysics(float timeStep, int substeps)					//Stepping PhysX
	{ 
		for(int i = 0; i < allFluidHelpers.size(); i++)
		{
			allFluidHelpers[i]->Emit();
		}

		for(int i = 0; i < substeps; i++)
		{
			gScene->simulate(timeStep/substeps);
			while(!gScene->fetchResults(true))
				continue;
		}
	} 

	void AddFluid(GUID id, vector<PxVec3> initPts, PxU32 maxParticles, PxReal restitution, PxReal viscosity,
		PxReal stiffness, PxReal dynamicFriction, PxReal particleDistance, int rate) 
	{	
		PxParticleFluid* fluid = gPhysicsSDK->createParticleFluid(maxParticles);

		fluid->setGridSize(5.0f);
		fluid->setMaxMotionDistance(0.3f);
		fluid->setRestOffset(particleDistance*0.3f);
		fluid->setContactOffset(particleDistance*0.3f*2);
		fluid->setDamping(0.0f);
		fluid->setRestitution(restitution);
		fluid->setDynamicFriction(dynamicFriction);
		fluid->setRestParticleDistance(particleDistance);
		fluid->setViscosity(viscosity);
		fluid->setStiffness(stiffness);
		fluid->setParticleReadDataFlag(PxParticleReadDataFlag::eVELOCITY_BUFFER, true);
		fluid->setParticleReadDataFlag(PxParticleReadDataFlag::eDENSITY_BUFFER, true);

		if(useGpu)
			fluid->setParticleBaseFlag(PxParticleBaseFlag::eGPU, true);

		//create the particles if rate is zero (i.e. blob)
		if(rate == 0)
		{
			vector<PxU32> indices;
			for(int i = 0; i < initPts.size(); i++)
				indices.push_back(i);

			PxParticleCreationData particleCreationData;
			particleCreationData.numParticles = initPts.size();
			particleCreationData.indexBuffer = PxStrideIterator<const PxU32>(&indices[0]);
			particleCreationData.positionBuffer = PxStrideIterator<const PxVec3>(&initPts[0]);

			fluid->createParticles(particleCreationData);
		}

		//set up the helper
		FluidHelper* helper = new FluidHelper(fluid, initPts, rate);
		allFluidHelpers.push_back(helper);
		fluid->userData = (void*) helper;

		//add to scene
		gScene->addActor(*fluid);

		//add to object map for retrieval by Rhino
		actorObjMap[fluid] = id;
	}

	void ShutdownPhysics()
	{
		actorObjMap.clear();
		jointMap.clear();
		allFluidHelpers.clear();

		gScene->release();	
		cooking->release();
		gPhysicsSDK->release();
		cm->releaseContext();
		cm->release();
		gFoundation->release();

		gScene = NULL;
		gPhysicsSDK = NULL;
		cm = NULL;
		gFoundation = NULL;
	}

	void ResetScene()
	{

	}

	map<PxActor*, GUID> ExportResults()
	{
		return actorObjMap;
	}

	void AddBox(GUID guid, PxTransform transform, PxReal x, PxReal y, PxReal z)
	{
		PxBoxGeometry	boxGeometry(PxVec3(x,y,z));										
		PxRigidDynamic* gBox = PxCreateDynamic(*gPhysicsSDK, transform, boxGeometry, *defaultMat, 1.0f);
		gBox->setSolverIterationCounts(8, 4);
		//gBox->setMass(10);
		gScene->addActor(*gBox);

		actorObjMap[gBox] = guid;
	}

	void AddSphere(GUID guid, PxTransform transform, PxReal r)
	{
		PxSphereGeometry sphereGeometry(r);									
		PxRigidDynamic* gSphere = PxCreateDynamic(*gPhysicsSDK, transform, sphereGeometry, *defaultMat, 1.0f);	
		gSphere->setSolverIterationCounts(8, 4);
		gScene->addActor(*gSphere);

		actorObjMap[gSphere] = guid;
	}

	void AddCapsule(GUID guid, PxTransform transform, PxReal h, PxReal r)
	{
		PxCapsuleGeometry capsuleGeom(r, h);
		PxRigidDynamic* capsule = PxCreateDynamic(*gPhysicsSDK, transform, capsuleGeom, *defaultMat, 1.0f);	
		capsule->setSolverIterationCounts(8, 4);
		gScene->addActor(*capsule);
		actorObjMap[capsule] = guid;

	}

	void SetRigidDynamicMass(GUID actorIdx, double mass)
	{
		PxRigidDynamic* rigidActor = GetActorFromGUID(actorIdx)->is<PxRigidDynamic>();

		if(rigidActor != NULL)
			PxRigidBodyExt::setMassAndUpdateInertia(*rigidActor, mass);
	}

	void SetRigidDynamicDrag(GUID actorIdx, double drag)
	{
		PxRigidDynamic* rigidActor = GetActorFromGUID(actorIdx)->is<PxRigidDynamic>();

		if(rigidActor != NULL)
		{
			//log("drag: " + to_string((int) rigidActor->getLinearDamping()));
			rigidActor->setLinearDamping(drag);
			rigidActor->setAngularDamping(drag);
		}
	}

	void AddMultiShapeRigidDynamic(GUID guid, vector<int> ptCounts, 
		vector<int> triCounts, vector<PxVec3*> ptsVec, 
		vector<int*> trisVec, PxTransform pose, bool forceConvex)
	{
		PxRigidDynamic* actor = gPhysicsSDK->createRigidDynamic(pose);

		//loop through shape definitions
		for(int i = 0; i < ptCounts.size(); i++)
		{
			PxConvexMeshGeometry shape = CookConvexMesh(ptCounts.at(i), triCounts.at(i), ptsVec.at(i), trisVec.at(i), forceConvex);
			PxShape* tmpShape = gPhysicsSDK->createShape(shape, *defaultMat, PxShapeFlag::eSIMULATION_SHAPE);
			actor->attachShape(*tmpShape);
		}


		PxRigidBodyExt::setMassAndUpdateInertia(*actor, 10.0);
		actor->setSolverIterationCounts(8, 4);
		gScene->addActor(*actor);

		actorObjMap[actor] = guid;
	}

	void AddRigidStatic(GUID guid, int ptCount, int triCount, PxVec3* pts, int* tris, PxTransform transform)
	{
		PxRigidStatic* actor = gPhysicsSDK->createRigidStatic(transform);

		PxTriangleMeshGeometry shape = CookTriangleMesh(ptCount, triCount, pts, tris);
		actor->createShape(shape, *defaultMat);
		gScene->addActor(*actor);

		actorObjMap[actor] = guid;
	}

	void AddEmpty(GUID guid, int ptCount, int triCount, PxVec3* pts, int* tris, PxTransform transform)
	{							
		PxConvexMeshGeometry con_shape = CookConvexMesh(ptCount, triCount, pts, tris, true);	
		PxRigidDynamic* gBox = gPhysicsSDK->createRigidDynamic(transform);
		PxShape* shape = gBox->createShape(con_shape, *defaultMat);
		gBox->setMass(10.0);
		shape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, false);
		shape->setFlag(PxShapeFlag::eTRIGGER_SHAPE, true);
		//PxRigidBodyExt::updateMassAndInertia(*gBox, 10.0, NULL);
		gBox->setSolverIterationCounts(32, 32);
		gBox->setLinearDamping(10);
		gBox->setAngularDamping(10);
		gScene->addActor(*gBox);		
		actorObjMap[gBox] = guid;
	}

	void AddCompoundConvex(GUID guid, int ptCount, int triCount, double* pts, int* tris, PxTransform transform)
	{
		VHACD::IVHACD* myHACD = VHACD::CreateVHACD();
		if(myHACD == NULL)
			log("could not create hacd");
		VHACD::IVHACD::Parameters params;
		//set parameters
		params.Init();
		params.m_concavity = 0.01;
		params.m_maxNumVerticesPerCH = 128;
		//params.m_depth = 31;
		//params.m_convexhullDownsampling = 12;
		//params.m_planeDownsampling = 12;
		params.m_resolution = 500000;
		params.m_convexhullApproximation = 0;
		log("about to compute convex hull: ");
		//compute
		myHACD->Compute(pts, 3, ptCount, 
			tris, 3, triCount, params);
		log("finished computing");
		int nClusters = myHACD->GetNConvexHulls();

		//myHACD.Save("output.wrl", false);
		vector<PxShape*> conShapes;
		for (int c=0;c<nClusters;c++)
		{
			//generate convex result
			VHACD::IVHACD::ConvexHull currHull;
			myHACD->GetConvexHull(c, currHull);
			size_t nPoints = currHull.m_nPoints;
			size_t nTriangles = currHull.m_nTriangles;
			PxVec3* vertices = new PxVec3[nPoints];
			int* tris = new int[3 * nTriangles];

			// points
			for(size_t v = 0; v < nPoints; v++)
				vertices[v] = PxVec3(currHull.m_points[3*v],currHull.m_points[3*v + 1],currHull.m_points[3*v + 2]);

			// triangles
			for(size_t f = 0; f < nTriangles * 3; f++)
				tris[f] = currHull.m_triangles[f];

			PxConvexMeshGeometry conMesh = CookConvexMesh(nPoints, nTriangles, vertices, tris, true);
			PxShape* conShape = gPhysicsSDK ->createShape(conMesh, *defaultMat, PxShapeFlag::eSIMULATION_SHAPE);
			conShapes.push_back(conShape);
		}

		PxRigidDynamic* rBody = PxCreateDynamic(*gPhysicsSDK, transform, *conShapes.at(0), 1.0f);

		//assign the shapes to the rigidbody
		for (int i =1; i < conShapes.size(); i++)
			rBody -> attachShape(*conShapes.at(i));

		PxRigidBodyExt::updateMassAndInertia(*rBody, 1.0, NULL);
		rBody->setSolverIterationCounts(8, 4);
		//add to the scene
		gScene->addActor(*rBody);
		//add to the map
		actorObjMap[rBody] = guid;
	}

	void AddConvexRigidDynamic(const GUID guid, int ptCount, int triCount, PxVec3* pts, int* tris, PxTransform transform, bool forceConvex)
	{
		PxRigidDynamic* actor = gPhysicsSDK->createRigidDynamic(transform);

		PxConvexMeshGeometry shape = CookConvexMesh(ptCount, triCount, pts, tris, forceConvex);
		actor->createShape(shape, *defaultMat);

		PxRigidBodyExt::setMassAndUpdateInertia(*actor, 10.0);
		actor->setSolverIterationCounts(8, 4);
		gScene->addActor(*actor);

		actorObjMap[actor] = guid;
	}

	PxActor* GetActorFromGUID(GUID id)
	{		
		for(map<PxActor*, GUID>::iterator it = actorObjMap.begin(); it != actorObjMap.end(); ++it)
		{
			if(id == it->second)
				return it->first;
		}
	}

	void LockDOF(GUID guid, PxTransform anchor, int flags)
	{
		PxRigidDynamic* rbody = GetActorFromGUID(guid)->is<PxRigidDynamic>();
		if(rbody != NULL)
		{
			log("setting constraint with flags: " + to_string(static_cast<long long>(flags)) + " flags\n");
			PxTransform f1 = PxTransform(anchor.p);
			f1 = rbody->getGlobalPose().transformInv(f1);
			PxD6Joint* joint = PxD6JointCreate(*gPhysicsSDK, rbody, f1, NULL, anchor);
			if(flags & 1)
				joint -> setMotion(PxD6Axis::eX,		PxD6Motion::eLOCKED);
			else
				joint -> setMotion(PxD6Axis::eX,		PxD6Motion::eFREE);
			if(flags & 2)
				joint -> setMotion(PxD6Axis::eY,		PxD6Motion::eLOCKED);
			else
				joint -> setMotion(PxD6Axis::eY,		PxD6Motion::eFREE);
			if(flags & 4)
				joint -> setMotion(PxD6Axis::eZ,		PxD6Motion::eLOCKED);
			else
				joint -> setMotion(PxD6Axis::eZ,		PxD6Motion::eFREE);
			if(flags & 8)
				joint -> setMotion(PxD6Axis::eSWING1,	PxD6Motion::eLOCKED);
			else
				joint -> setMotion(PxD6Axis::eSWING1,	PxD6Motion::eFREE);
			if(flags & 16)
				joint -> setMotion(PxD6Axis::eSWING2,	PxD6Motion::eLOCKED);
			else
				joint -> setMotion(PxD6Axis::eSWING2,	PxD6Motion::eFREE);
			if(flags & 32)
				joint -> setMotion(PxD6Axis::eTWIST,	PxD6Motion::eLOCKED);
			else
				joint -> setMotion(PxD6Axis::eTWIST,	PxD6Motion::eFREE);

			jointMap[joint] = guid;
		}
	}

	void RemoveJoint(GUID jointId)
	{
		log("Joint map has: " + to_string(jointMap.size()) + " elements\n");
		PxJoint* joint;
		for(map<PxJoint*, GUID>::iterator it = jointMap.begin(); it != jointMap.end(); ++it)
		{
			if(jointId == it->second)
			{
				joint = it->first;
				break;
			}
		}

		if(joint == NULL)
			return;

		jointMap.erase(joint);
		joint->release();
		log("Joint map has: " + to_string(jointMap.size()) + " elements\n");
	}

	void SetJointEndPoints(GUID jointId, int index, PxVec3 pos)
	{
		PxJoint* joint;
		for(map<PxJoint*, GUID>::iterator it = jointMap.begin(); it != jointMap.end(); ++it)
		{
			if(jointId == it->second)
			{
				joint = it->first;
				break;
			}
		}

		if(joint == NULL)
			return;

		log("about to set joint end points\n");

		PxRigidActor* actor1;
		PxRigidActor* actor2;
		joint->getActors(actor1, actor2);
		PxTransform pose;

		log("made it this far");
		switch(index)
		{
		case 0:
			if(actor1 != NULL)
			{
				pose = actor1->getGlobalPose();
				pose = PxTransform(pose.transformInv(pos), pose.q);
			}
			else
				pose = PxTransform(pos);
			joint->setLocalPose(PxJointActorIndex::eACTOR0, pose);
			break;
		case 1:
			if(actor2 != NULL)
			{
				pose = actor2->getGlobalPose();
				pose = PxTransform(pose.transformInv(pos), pose.q);
			}
			else
				pose = PxTransform(pos);
			joint->setLocalPose(PxJointActorIndex::eACTOR1, pose);
			break;
		}

	}

	void AddDistanceJoint(GUID id, GUID fromIdx, GUID toIdx, PxVec3 p1, PxVec3 p2, double maxDistance, double minDistance, double stiffness, double breakForce)
	{
		PxRigidDynamic* from = GetActorFromGUID(fromIdx)->is<PxRigidDynamic>();
		PxRigidDynamic* to;
		GUID empty = GUID();
		if(toIdx != empty)
			to = GetActorFromGUID(toIdx)->is<PxRigidDynamic>();
		else
			to = NULL;

		if(from == NULL)
			return;

		from->wakeUp();

		PxVec3 vec = p2 - p1;
		PxTransform t1 = PxTransform(from->getGlobalPose().transformInv(p1));
		PxTransform t2;

		if(to != NULL)
		{
			t2 = PxTransform(to->getGlobalPose().transformInv(p2));
			to->wakeUp();
		}
		else
			t2 = PxTransform(p2);

		PxDistanceJoint* newJoint = PxDistanceJointCreate(*gPhysicsSDK, from, t1, to, t2);
		if(maxDistance >= 0 && minDistance >= 0)
		{
			newJoint->setMaxDistance(maxDistance);
			newJoint->setMinDistance(minDistance);
		}
		else
		{
			newJoint->setMaxDistance(vec.magnitude());
			newJoint->setMinDistance(vec.magnitude());
		}

		newJoint->setStiffness(stiffness);
		newJoint->setDistanceJointFlag(PxDistanceJointFlag::eMAX_DISTANCE_ENABLED, true);
		newJoint->setDistanceJointFlag(PxDistanceJointFlag::eMIN_DISTANCE_ENABLED, true);
		if(stiffness > 0)
			newJoint->setDistanceJointFlag(PxDistanceJointFlag::eSPRING_ENABLED, true);
		if(breakForce > 0)
			newJoint->setBreakForce(breakForce, breakForce);
		newJoint->setConstraintFlag(PxConstraintFlag::eCOLLISION_ENABLED, true);
		newJoint->setConstraintFlag(PxConstraintFlag::ePROJECTION, true);

		jointMap[newJoint] = id;
	}

	void AddRevoluteJoint(GUID id, GUID fromIdx, GUID toIdx, PxVec3 p1, PxVec3 p2, double maxAngle, double minAngle, double stiffness, double breakForce, double driveVel)
	{
		PxRigidDynamic* from = GetActorFromGUID(fromIdx)->is<PxRigidDynamic>();
		PxRigidDynamic* to;
		GUID empty = GUID();
		if(toIdx != empty)
			to = GetActorFromGUID(toIdx)->is<PxRigidDynamic>();
		else
			to = NULL;

		if(from == NULL)
			return;

		from->wakeUp();

		PxVec3 vec = (p1 - p2);
		PxVec3 cross = vec.cross(PxVec3(1,0,0));
		PxReal angle = PxAcos(vec.dot(cross));
		cross.getNormalized();
		PxQuat rot = PxQuat(angle, cross.getNormalized());

		PxTransform t1 = PxTransform(from->getGlobalPose().transformInv(p1), rot);
		PxTransform t2;
		if(to != NULL)
		{
			t2 = PxTransform(to->getGlobalPose().transformInv(p1), rot);
			to->wakeUp();
		}
		else
			t2 = PxTransform(p1, rot);
		PxRevoluteJoint* newJoint = PxRevoluteJointCreate(*gPhysicsSDK, from, t1, to, t2);
		PxJointAngularLimitPair limits = PxJointAngularLimitPair(minAngle, maxAngle);

		if(maxAngle != 0 || minAngle != 0)
		{
			newJoint->setLimit(limits);
			newJoint->setRevoluteJointFlag(PxRevoluteJointFlag::eLIMIT_ENABLED, true);
		}
		if(breakForce > 0)
			newJoint->setBreakForce(breakForce, breakForce);
		if(driveVel > 0)
		{
			newJoint->setDriveVelocity(driveVel);
			newJoint->setRevoluteJointFlag(PxRevoluteJointFlag::eDRIVE_ENABLED, true);
		}
		newJoint->setConstraintFlag(PxConstraintFlag::eCOLLISION_ENABLED, true);
		newJoint->setConstraintFlag(PxConstraintFlag::ePROJECTION, true);

		jointMap[newJoint] = id;
	}

	void AddJointFromPair(GUID fromIdx, GUID toIdx, PxVec3 p1, PxVec3 p2, double distMultiplier, double linear, double twist, double swing)
	{
		PxRigidDynamic* from = GetActorFromGUID(fromIdx)->is<PxRigidDynamic>();
		PxRigidDynamic* to = GetActorFromGUID(toIdx)->is<PxRigidDynamic>();

		if(from == NULL || to == NULL)
			return;

		PxVec3 vec = (p1 - p2);
		PxVec3 cross = vec.cross(PxVec3(1,0,0));
		PxReal angle = PxAcos(vec.dot(cross));
		cross.getNormalized();
		PxQuat rot = PxQuat(angle, cross.getNormalized());

		PxTransform t1 = PxTransform(from->getGlobalPose().transformInv(p1), rot);
		PxTransform t2 = PxTransform(to->getGlobalPose().transformInv(p1), rot);

		//PxD6Joint* newJoint = PxD6JointCreate(*gPhysicsSDK, from, t1, to, t2);
		PxRevoluteJoint* newJoint = PxRevoluteJointCreate(*gPhysicsSDK, from, t1, to, t2);
		//PxDistanceJoint* newJoint = PxDistanceJointCreate(*gPhysicsSDK, from, t1, to, t2);
		//newJoint->setMaxDistance(distMultiplier * vec.magnitude());
		//newJoint->setStiffness(linear);
		//newJoint->setDistanceJointFlag(PxDistanceJointFlag::eMAX_DISTANCE_ENABLED, true);
		//if(linear > 0)
		//	newJoint->setDistanceJointFlag(PxDistanceJointFlag::eSPRING_ENABLED, true);

		newJoint->setConstraintFlag(PxConstraintFlag::eCOLLISION_ENABLED, true);
		newJoint->setConstraintFlag(PxConstraintFlag::ePROJECTION, true);
		//newJoint->setBreakForce(10000, 10000);

	}

	void AddCloth(GUID guid, int ptCount, int triCount, int qCount, PxVec3* pts, int* tris, int* quads, float* invMasses, PxTransform transform, float stiffness, vector<int> fixedPts)
	{
		PxClothMeshDesc meshDesc = PxClothMeshDesc();
		PxClothParticle* vertices = new PxClothParticle[ptCount];

		for(int i = 0; i < ptCount; i++)		
			vertices[i] = PxClothParticle(pts[i], invMasses[i]);		

		//fix particles
		if(fixedPts.size() > 0)
		{
			for(int i = 0; i < fixedPts.size(); i++)
				vertices[i].invWeight = 0.0;
		}
		meshDesc.points.count		= ptCount;
		meshDesc.triangles.count	= triCount;
		meshDesc.quads.count		= qCount;
		meshDesc.points.stride		= sizeof(PxClothParticle);
		meshDesc.triangles.stride	= sizeof(int) * 3;
		meshDesc.quads.stride		= sizeof(int) * 4;
		meshDesc.points.data		= vertices;
		meshDesc.triangles.data		= tris;
		meshDesc.quads.data			= quads;


		PxClothFabric* clothFabric = PxClothFabricCreate(*gPhysicsSDK, meshDesc, gScene->getGravity());

		// create the cloth actor
		PxCloth* cloth = gPhysicsSDK->createCloth( transform, *clothFabric, vertices, PxClothFlags());

		ClothHelpers::createVirtualParticles(*cloth, meshDesc, 5);

		//create motion constraints
		PxClothParticleMotionConstraint* constraints = new PxClothParticleMotionConstraint[ptCount];
		for(int i = 0; i < ptCount; i++)
			constraints[i] = PxClothParticleMotionConstraint(pts[i], FLT_MAX);

		// set solver settings
		cloth->setSolverFrequency(240);
		cloth->setStiffnessFrequency(20.0f);

		// damp global particle velocity to 90% every 0.1 seconds
		cloth->setDampingCoefficient(PxVec3(0.01f)); // damp local particle velocity
		cloth->setLinearDragCoefficient(PxVec3(0.05f)); // transfer frame velocity
		cloth->setAngularDragCoefficient(PxVec3(0.05f)); // transfer frame rotation

		cloth->setClothFlag(PxClothFlag::eGPU, useGpu);
		cloth->setClothFlag(PxClothFlag::eSCENE_COLLISION, true);
		cloth->setClothFlag(PxClothFlag::eSWEPT_CONTACT, true);
		cloth->setContactOffset(5);

		ClothHelpers::setStiffness(*cloth,stiffness);

		cloth->setSelfCollisionDistance(1);
		cloth->setSelfCollisionStiffness(50);

		// add this cloth into the scene
		gScene->addActor(*cloth);
		actorObjMap[cloth] = guid;

		log("added a cloth actor");

	}

	void SetClothParticlePosition(GUID actorIdx, int index, PxVec3 newPos, bool release)
	{
		PxActor*  actor = GetActorFromGUID(actorIdx);

		if(actor != NULL)
		{
			PxCloth* cloth = actor->is<PxCloth>();

			if(cloth != NULL)
			{
				int nb = cloth->getNbParticles();
				PxClothParticleMotionConstraint* constraints = new PxClothParticleMotionConstraint[nb];
				for(int i = 0; i < nb; i++)
				{
					if(i == index && !release)
						constraints[i] = PxClothParticleMotionConstraint(newPos, 0.1);
					else
						constraints[i] = PxClothParticleMotionConstraint(PxVec3(0,0,0), FLT_MAX);
				}
				cloth->setMotionConstraints(constraints);;
			}
		}
	}

	int FindClosestClothParticle(GUID actorIdx, PxVec3 pos)
	{
		PxActor*  actor = GetActorFromGUID(actorIdx);
		int id_out = -1;
		float d = 100000.00f;
		if(actor != NULL)
		{
			PxCloth* cloth = actor->is<PxCloth>();

			if(cloth != NULL)
			{
				//remap to local space
				pos = cloth->getGlobalPose().transformInv(pos);
				PxClothParticleData* particleData = cloth->lockParticleData(PxDataAccessFlag::eWRITABLE);
				if (particleData)
				{

					PxU32 numParticles = cloth->getNbParticles();
					PxClothParticle* particles = particleData->previousParticles;

					for(int i = 0; i < numParticles; i++)
					{
						float test = (particles[i].pos - pos).magnitude();
						if(test<d)
						{
							d = test;
							id_out = i;
						}
					}
				}

				particleData->unlock();
			}
		}

		return id_out;
	}

	void SetClothParticleWeight(GUID actorIdx, vector<int> fixedPts, double weight)
	{
		PxActor*  actor = GetActorFromGUID(actorIdx);

		if(actor != NULL)
		{
			PxCloth* cloth = actor->is<PxCloth>();

			if(cloth != NULL)
			{
				PxClothParticleData* particleData = cloth->lockParticleData(PxDataAccessFlag::eWRITABLE);
				if (particleData)
				{

					PxU32 numParticles = cloth->getNbParticles();
					PxClothParticle* particles = particleData->previousParticles;

					for(int i = 0; i < fixedPts.size(); i++)
					{
						if(fixedPts[i] < numParticles)
						{
							particles[fixedPts[i]].invWeight = weight;
						}
					}
				}

				particleData->unlock();
			}

		}
	}

	void RemoveActor(GUID guid)
	{
		PxActor* actor = GetActorFromGUID(guid);
		if(actor != NULL)
			gScene -> removeActor(*actor, true);

		//TODO: delete connected joints.
		actorObjMap.erase(actor);
	}

	GUID Raycast(PxVec3 origin, PxVec3 dir, float &dist, PxVec3 &hitPoint)
	{
		GUID result = GUID();

		// raycast rigid bodies in scene
		PxRaycastHit hit; hit.shape = NULL;
		PxRaycastBuffer hit1;

		gScene->raycast(origin, dir, PX_MAX_F32, hit1, PxHitFlag::eMESH_ANY);
		hit = hit1.block;

		if(hit.actor && hit.actor->is<PxRigidDynamic>() != NULL)
		{ 
			result = actorObjMap[hit.actor];
			hitPoint = hit.position;
			dist = hit.distance;
		}

		return result;
	}

	vector<vector<double>> ExportCollisionShape(GUID guid)
	{
		PxRigidActor* actor = GetActorFromGUID(guid)->is<PxRigidActor>();
		vector<vector<double>> ptsOut;
		if(actor != NULL)
		{
			PxU32 nbShapes = actor->getNbShapes();
			//PxShape** shapesOut = (PxShape**)(sizeof(PxShape*)*nbShapes);
			PxShape* shapesOut[256];
			PxU32 nb = actor->getShapes(shapesOut, nbShapes);
			for(PxU32 i=0;i<nbShapes;i++)
			{			
				PxShape* currShape = shapesOut[i];
				if(currShape->getGeometryType() == PxGeometryType::eCONVEXMESH && currShape != NULL)
				{
					const PxVec3* pVerts;
					const PxU8* indices;
					PxConvexMeshGeometry convexShapeOut;
					currShape->getConvexMeshGeometry(convexShapeOut);
					int numVerts = convexShapeOut.convexMesh->getNbVertices();
					pVerts = convexShapeOut.convexMesh->getVertices();
					int numPolys = convexShapeOut.convexMesh->getNbPolygons();
					indices = convexShapeOut.convexMesh->getIndexBuffer();
					for(int j = 0; j < numPolys; j++)
					{
						PxHullPolygon currPoly;
						vector<double> currPts;
						convexShapeOut.convexMesh->getPolygonData(j, currPoly);
						for(int k = 0; k < currPoly.mNbVerts; k++)
						{
							PxVec3 v = pVerts[indices[currPoly.mIndexBase + k]];
							v += actor->getGlobalPose().p;
							actor->getGlobalPose().q.rotate(v);
							currPts.push_back(v.x);
							currPts.push_back(v.y);
							currPts.push_back(v.z);
						}

						ptsOut.push_back(currPts);
					}
				}
				else
					log("not the right type or null");
			}
		}

		log("return mesh with " + to_string(ptsOut.size()) + " faces\n");
		return ptsOut;
	}

	void SetKinematic(GUID actorIdx, bool state)
	{
		PxActor*  actor = GetActorFromGUID(actorIdx);

		if(actor != NULL)
		{
			actor->is<PxRigidDynamic>()->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, state);
			if(!state)
				actor->is<PxRigidDynamic>()->wakeUp();
		}
	}

	void SetKinematicPose(GUID actorIdx, PxVec3 translation)
	{
		PxActor*  actor = GetActorFromGUID(actorIdx);

		if(actor != NULL)
		{
			PxRigidDynamic* currActor = actor->is<PxRigidDynamic>();
			if(currActor != NULL)
			{
				PxTransform translationT = PxTransform(currActor->getGlobalPose().p + translation);
				currActor->setKinematicTarget(translationT);
				return;
			}
		}
	}

	void SetKinematicPose(GUID actorIdx, PxTransform xform)
	{
		PxActor*  actor = GetActorFromGUID(actorIdx);

		if(actor != NULL)
		{
			PxRigidDynamic* currActor = actor->is<PxRigidDynamic>();
			if(currActor != NULL)
			{
				if(currActor->isSleeping())
					currActor->wakeUp();
				PxTransform translationT = currActor->getGlobalPose() * xform;
				currActor->setKinematicTarget(translationT);
				return;
			}
		}
	}

	void AddTriangleRigidStatic(int ptCount, int triCount, PxVec3* pts, int* tris, PxTransform transform)
	{
		PxRigidStatic* actor = gPhysicsSDK->createRigidStatic(transform);

		PxTriangleMeshGeometry shape = CookTriangleMesh(ptCount, triCount, pts, tris);
		actor->createShape(shape, *defaultMat);
		gScene->addActor(*actor);
	}

	PxTriangleMeshGeometry CookTriangleMesh(int ptCount, int triCount, PxVec3* pts, int* tris)
	{
		PxTriangleMeshDesc meshDesc;
		meshDesc.points.count		= ptCount;
		meshDesc.triangles.count	= triCount;
		meshDesc.points.stride		= sizeof(PxVec3);
		meshDesc.triangles.stride	= sizeof(int) * 3;
		meshDesc.points.data		= pts;
		meshDesc.triangles.data		= tris;

		PxTriangleMesh* mesh = NULL;
		PxDefaultMemoryOutputStream stream(gFoundation->getAllocatorCallback());

		bool ok;
		{			
			ok = cooking->cookTriangleMesh(meshDesc, stream);
		}
		if ( ok )
		{		
			PxDefaultMemoryInputData inStream(stream.getData(), stream.getSize());
			mesh = gPhysicsSDK->createTriangleMesh(inStream);
		}

		PxMeshScale meshScale = PxMeshScale(PxVec3(1.0), PxQuat(PxIdentity));
		PxTriangleMeshGeometry triMeshGeo(mesh, meshScale);

		return triMeshGeo;
	}

	PxConvexMeshGeometry CookConvexMesh(int ptCount, int triCount, PxVec3* pts, int* tris, bool forceConvex)
	{
		PxConvexMeshDesc meshDesc;

		meshDesc.points.count = ptCount;
		meshDesc.points.stride = sizeof(PxVec3);
		meshDesc.points.data = pts;
		meshDesc.vertexLimit = 256;

		if(forceConvex)
			meshDesc.flags = PxConvexFlag::eCOMPUTE_CONVEX;
		else
		{
			meshDesc.indices.count = triCount;
			meshDesc.indices.stride = sizeof(PxU32) * 3;
			meshDesc.indices.data = (void*)tris;
		}


		PxConvexMesh* mesh = NULL;
		PxDefaultMemoryOutputStream stream(gDefaultAllocatorCallback);

		bool ok;
		{
			//PxDefaultFileOutputStream stream(filenameCooked);
			ok = cooking->cookConvexMesh(meshDesc, stream);
		}
		if(ok)
		{
			PxDefaultMemoryInputData stream(stream.getData(), stream.getSize());
			//PxDefaultFileInputData stream(filenameCooked);
			mesh = gPhysicsSDK->createConvexMesh(stream);
		}

		PxMeshScale meshScale = PxMeshScale(PxVec3(1.0), PxQuat(PxIdentity));
		PxConvexMeshGeometry convexMesh(mesh, meshScale);

		return convexMesh;
	}

}
