#include "stdafx.h"
#include "FluidHelper.h"
#include "foundation\PxMathUtils.h"
#include "foundation\PxMath.h"

using namespace physx;
using namespace std;

namespace PhysXEngine
{
	FluidHelper::FluidHelper()
	{
		FluidHelper::rand.setSeed(1);
	}

	void FluidHelper::SetFluid(PxParticleFluid* currFluid)
	{
		fluid = currFluid;
	}

	FluidHelper::FluidHelper(PxParticleFluid* currFluid, vector<PxVec3> pts, double r)
	{
		FluidHelper::rand.setSeed(1);
		for(int i = 0; i < pts.size(); i++)
			emitPoints.push_back(pts[i]);
		rate = r;
		fluid = currFluid;
		pCount = 0;
	}
	void FluidHelper::Emit()
	{
		if(rate == 0)
			return;

		int count = PxMax(1, rate);

		vector<PxVec3> pts;
		vector<PxU32> indices;

		if(pCount + count >= fluid->getMaxParticles())
			return;

		for(int i = pCount; i < count + pCount; i++)
		{
			indices.push_back(i);
			int currId = rand.rand(0, emitPoints.size());
			pts.push_back(emitPoints[currId]);
		}

		if(pts.size() < 1)
			return;

		PxParticleCreationData particleCreationData;
		particleCreationData.numParticles = count;
		particleCreationData.indexBuffer = PxStrideIterator<const PxU32>(&indices[0]);
		particleCreationData.positionBuffer = PxStrideIterator<const PxVec3>(&pts[0]);

		if(fluid->getMaxParticles() > pCount + particleCreationData.numParticles)
		{
			fluid->createParticles(particleCreationData);
			pCount += particleCreationData.numParticles;
		}
	}
}