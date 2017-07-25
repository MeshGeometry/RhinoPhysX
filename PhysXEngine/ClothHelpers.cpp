// This code contains NVIDIA Confidential Information and is disclosed to you
// under a form of NVIDIA software license agreement provided separately to you.
//
// Notice
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software and related documentation and
// any modifications thereto. Any use, reproduction, disclosure, or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA Corporation is strictly prohibited.
//
// ALL NVIDIA DESIGN SPECIFICATIONS, CODE ARE PROVIDED "AS IS.". NVIDIA MAKES
// NO WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ALL IMPLIED WARRANTIES OF NONINFRINGEMENT,
// MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Information and code furnished is believed to be accurate and reliable.
// However, NVIDIA Corporation assumes no responsibility for the consequences of use of such
// information or for any infringement of patents or other rights of third parties that may
// result from its use. No license is granted by implication or otherwise under any patent
// or patent rights of NVIDIA Corporation. Details are subject to change without notice.
// This code supersedes and replaces all information previously supplied.
// NVIDIA Corporation products are not authorized for use as critical
// components in life support devices or systems without express written approval of
// NVIDIA Corporation.
//
// Copyright (c) 2008-2013 NVIDIA Corporation. All rights reserved.
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  

#include "stdafx.h"
#include "ClothHelpers.h"
#include "foundation/../../src/foundation/include/PsBasicTemplates.h"
#include "foundation/../../src/foundation/include/PsHashSet.h"
#include "foundation/../../src/foundation/include/PsArray.h"

using namespace physx;
using namespace std;

bool PhysXEngine::ClothHelpers::attachBorder(PxClothParticle* particles, PxU32 numParticles, BorderFlags borderFlag)
{
	// compute bounds in x and z
	PxBounds3 bounds = PxBounds3::empty();

	for(PxU32 i = 0; i < numParticles; i++)
		bounds.include(particles[i].pos);

	PxVec3 skin = bounds.getExtents() * 0.01f;
	bounds.minimum += skin;
	bounds.maximum -= skin;

	if (borderFlag & BORDER_LEFT)
	{
		for (PxU32 i = 0; i < numParticles; i++)
			if (particles[i].pos.x <= bounds.minimum.x)
				particles[i].invWeight = 0.0f;
	}
	if (borderFlag & BORDER_RIGHT)
	{
		for (PxU32 i = 0; i < numParticles; i++)
			if (particles[i].pos.x >= bounds.maximum.x)
				particles[i].invWeight = 0.0f;
	}
	if (borderFlag & BORDER_BOTTOM)
	{
		for (PxU32 i = 0; i < numParticles; i++)
			if (particles[i].pos.y <= bounds.minimum.y)
				particles[i].invWeight = 0.0f;
	}
	if (borderFlag & BORDER_TOP)
	{
		for (PxU32 i = 0; i < numParticles; i++)
			if (particles[i].pos.y >= bounds.maximum.y)
				particles[i].invWeight = 0.0f;
	}
	if (borderFlag & BORDER_NEAR)
	{
		for (PxU32 i = 0; i < numParticles; i++)
			if (particles[i].pos.z <= bounds.minimum.z)
				particles[i].invWeight = 0.0f;
	}
	if (borderFlag & BORDER_FAR)
	{
		for (PxU32 i = 0; i < numParticles; i++)
			if (particles[i].pos.z >= bounds.maximum.z)
				particles[i].invWeight = 0.0f;
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////
bool  PhysXEngine::ClothHelpers::attachBorder(PxCloth& cloth, BorderFlags borderFlag)
{
	PxClothParticleData* particleData = cloth.lockParticleData(PxDataAccessFlag::eWRITABLE);
	if (!particleData)
		return false;

	PxU32 numParticles = cloth.getNbParticles();
	PxClothParticle* particles = particleData->previousParticles;

	attachBorder(particles, numParticles, borderFlag);

	particleData->particles = 0;
	particleData->unlock();

	return true;
}

namespace
{
	static PxVec3 gVirtualParticleWeights[] = 
	{ 
		// center point
		PxVec3(1.0f / 3, 1.0f / 3, 1.0f / 3),

		// off-center point
		PxVec3(4.0f / 6, 1.0f / 6, 1.0f / 6),

		// edge point
		PxVec3(1.0f / 2, 1.0f / 2, 0.0f),
	};

	shdfnd::Pair<PxU32, PxU32> makeEdge(PxU32 v0, PxU32 v1)
	{
		if(v0 < v1)
			return shdfnd::Pair<PxU32, PxU32>(v0, v1);
		else
			return shdfnd::Pair<PxU32, PxU32>(v1, v0);
	}
}

bool PhysXEngine::ClothHelpers::createVirtualParticles(PxCloth& cloth, PxClothMeshDesc& meshDesc, int level)
{
	if(level < 1 || level > 5)
		return false;

	PxU32 edgeSampleCount[] = { 0, 0, 1, 1, 0, 1 };
	PxU32 triSampleCount[] = { 0, 1, 0, 1, 3, 3 };
	PxU32 quadSampleCount[] = { 0, 1, 0, 1, 4, 4 };

	PxU32 numEdgeSamples = edgeSampleCount[level];
	PxU32 numTriSamples = triSampleCount[level];
	PxU32 numQuadSamples = quadSampleCount[level];

	PxU32 numTriangles = meshDesc.triangles.count;
	PxU8* triangles = (PxU8*)meshDesc.triangles.data;

	PxU32 numQuads = meshDesc.quads.count;
	PxU8* quads = (PxU8*)meshDesc.quads.data;

	vector<PxU32> indices;
	indices.reserve(numTriangles * (numTriSamples + 3*numEdgeSamples) 
		+ numQuads * (numQuadSamples + 4*numEdgeSamples));

	typedef shdfnd::Pair<PxU32, PxU32> Edge;
	shdfnd::HashSet<Edge> edges;

	for (PxU32 i = 0; i < numTriangles; i++)
	{
		PxU32 v0, v1, v2;

		if (meshDesc.flags & PxMeshFlag::e16_BIT_INDICES)
		{
			PxU16* triangle = (PxU16*)triangles;
			v0 = triangle[0];
			v1 = triangle[1];
			v2 = triangle[2];
		}
		else
		{
			PxU32* triangle = (PxU32*)triangles;
			v0 = triangle[0];
			v1 = triangle[1];
			v2 = triangle[2];
		}

		if(numTriSamples == 1)
		{
			indices.push_back(v0);
			indices.push_back(v1);
			indices.push_back(v2);
			indices.push_back(0);
		}

		if(numTriSamples == 3)
		{
			indices.push_back(v0);
			indices.push_back(v1);
			indices.push_back(v2);
			indices.push_back(1);

			indices.push_back(v1);
			indices.push_back(v2);
			indices.push_back(v0);
			indices.push_back(1);

			indices.push_back(v2);
			indices.push_back(v0);
			indices.push_back(v1);
			indices.push_back(1);
		}

		if(numEdgeSamples == 1)
		{
			if(edges.insert(makeEdge(v0, v1)))
			{
				indices.push_back(v0);
				indices.push_back(v1);
				indices.push_back(v2);
				indices.push_back(2);
			}

			if(edges.insert(makeEdge(v1, v2)))
			{
				indices.push_back(v1);
				indices.push_back(v2);
				indices.push_back(v0);
				indices.push_back(2);
			}

			if(edges.insert(makeEdge(v2, v0)))
			{
				indices.push_back(v2);
				indices.push_back(v0);
				indices.push_back(v1);
				indices.push_back(2);
			}
		}

		triangles += meshDesc.triangles.stride;
	}

	for (PxU32 i = 0; i < numQuads; i++)
	{
		PxU32 v0, v1, v2, v3;

		if (meshDesc.flags & PxMeshFlag::e16_BIT_INDICES)
		{
			PxU16* quad = (PxU16*)quads;
			v0 = quad[0];
			v1 = quad[1];
			v2 = quad[2];
			v3 = quad[3];
		}
		else
		{
			PxU32* quad = (PxU32*)quads;
			v0 = quad[0];
			v1 = quad[1];
			v2 = quad[2];
			v3 = quad[3];
		}

		if(numQuadSamples == 1)
		{
			indices.push_back(v0);
			indices.push_back(v2);
			indices.push_back(v3);
			indices.push_back(2);
		}

		if(numQuadSamples == 4)
		{
			indices.push_back(v0);
			indices.push_back(v1);
			indices.push_back(v2);
			indices.push_back(1);

			indices.push_back(v1);
			indices.push_back(v2);
			indices.push_back(v3);
			indices.push_back(1);

			indices.push_back(v2);
			indices.push_back(v3);
			indices.push_back(v0);
			indices.push_back(1);

			indices.push_back(v3);
			indices.push_back(v0);
			indices.push_back(v1);
			indices.push_back(1);
		}

		if(numEdgeSamples == 1)
		{
			if(edges.insert(makeEdge(v0, v1)))
			{
				indices.push_back(v0);
				indices.push_back(v1);
				indices.push_back(v2);
				indices.push_back(2);
			}

			if(edges.insert(makeEdge(v1, v2)))
			{
				indices.push_back(v1);
				indices.push_back(v2);
				indices.push_back(v3);
				indices.push_back(2);
			}

			if(edges.insert(makeEdge(v2, v3)))
			{
				indices.push_back(v2);
				indices.push_back(v3);
				indices.push_back(v0);
				indices.push_back(2);
			}

			if(edges.insert(makeEdge(v3, v0)))
			{
				indices.push_back(v3);
				indices.push_back(v0);
				indices.push_back(v1);
				indices.push_back(2);
			}
		}

		quads += meshDesc.quads.stride;
	}

	cloth.setVirtualParticles(indices.size()/4, 
		&indices[0], 3, gVirtualParticleWeights);

	return true;
}

////////////////////////////////////////////////////////////////////////////////
bool  PhysXEngine::ClothHelpers::attachClothOverlapToShape(PxCloth& cloth, PxShape& shape,PxReal radius)
{
	PxClothParticleData* particleData = cloth.lockParticleData(PxDataAccessFlag::eWRITABLE);
	if (!particleData)
		return false;

	PxU32 numParticles = cloth.getNbParticles();
	PxClothParticle* particles = particleData->previousParticles;

	PxSphereGeometry sphere(radius);
	PxTransform position = PxTransform(PxIdentity);
	PxTransform pose = cloth.getGlobalPose();
	for (PxU32 i = 0; i < numParticles; i++)
	{
		// check if particle overlaps shape volume
		position.p = pose.transform(particles[i].pos);
		if (PxGeometryQuery::overlap(shape.getGeometry().any(), PxShapeExt::getGlobalPose(shape, *shape.getActor()), sphere, position))
			particles[i].invWeight = 0.0f;
	}

	particleData->particles = 0;
	particleData->unlock();

	return true;
}

////////////////////////////////////////////////////////////////////////////////
bool  PhysXEngine::ClothHelpers::createCollisionCapsule(
	const PxTransform &pose,
	const PxVec3 &center0, PxReal r0, const PxVec3 &center1, PxReal r1, 
	vector<PxClothCollisionSphere> &spheres, vector<PxU32> &indexPairs)
{
	PxTransform invPose = pose.getInverse();

	spheres.resize(2);

	spheres[0].pos = invPose.transform(center0);
	spheres[0].radius = r0;
	spheres[1].pos = invPose.transform(center1);
	spheres[1].radius = r1;

	indexPairs.resize(2);
	indexPairs[0] = 0;
	indexPairs[1] = 1;

	return true;
}


////////////////////////////////////////////////////////////////////////////////
bool  PhysXEngine::ClothHelpers::setMotionConstraints(PxCloth &cloth, PxReal radius)
{


	return true;
}

bool  PhysXEngine::ClothHelpers::setStiffness(PxCloth& cloth, PxReal newStiffness)
{
	PxClothStretchConfig stretchConfig;
	stretchConfig = cloth.getStretchConfig(PxClothFabricPhaseType::eVERTICAL);
	stretchConfig.stiffness = newStiffness;
	cloth.setStretchConfig(PxClothFabricPhaseType::eVERTICAL, stretchConfig);

	stretchConfig = cloth.getStretchConfig(PxClothFabricPhaseType::eHORIZONTAL);
	stretchConfig.stiffness = newStiffness;
	cloth.setStretchConfig(PxClothFabricPhaseType::eHORIZONTAL, stretchConfig);

	PxClothStretchConfig shearingConfig = cloth.getStretchConfig(PxClothFabricPhaseType::eSHEARING);
	shearingConfig.stiffness = newStiffness;
	cloth.setStretchConfig(PxClothFabricPhaseType::eSHEARING, shearingConfig);

	PxClothStretchConfig bendingConfig = cloth.getStretchConfig(PxClothFabricPhaseType::eBENDING);
	bendingConfig.stiffness = newStiffness;
	cloth.setStretchConfig(PxClothFabricPhaseType::eBENDING, bendingConfig);

	return true;
}



////////////////////////////////////////////////////////////////////////////////
bool  PhysXEngine::ClothHelpers::getParticlePositions(PxCloth &cloth, vector<PxVec3> &positions)
{
	PxClothParticleData* readData = cloth.lockParticleData();
	if (!readData)
		return false;

	const PxClothParticle* particles = readData->particles;
	if (!particles)
		return false;

	PxU32 nbParticles = cloth.getNbParticles();
	positions.resize(nbParticles);
	for (PxU32 i = 0; i < nbParticles; i++) 
		positions[i] = particles[i].pos;

	readData->unlock();

	return true;
}

////////////////////////////////////////////////////////////////////////////////
bool  PhysXEngine::ClothHelpers::setParticlePositions(PxCloth &cloth, const vector<PxVec3> &positions, bool useConstrainedOnly, bool useCurrentOnly)
{
	PxU32 nbParticles = cloth.getNbParticles();
	if (nbParticles != positions.size())
		return false;

	PxClothParticleData* particleData = cloth.lockParticleData(PxDataAccessFlag::eWRITABLE);
	if (!particleData)
		return false;

	PxClothParticle* particles = particleData->particles;
	for (PxU32 i = 0; i < nbParticles; i++) 
	{		
		bool constrained = particles[i].invWeight == 0.0f;
		if (!useConstrainedOnly || constrained)
			particles[i].pos = positions[i];
	}

	if(!useCurrentOnly)
		particleData->previousParticles = particleData->particles;

	particleData->unlock();

	return true;
}

