#include "stdafx.h"
#include "PxTkRandom.h"
using namespace physx;
using namespace std;

namespace PhysXEngine
{
	/// simple utility functions for PxCloth
	class FluidHelper
	{
	private:
		PxParticleFluid* fluid;
	public:
		FluidHelper();
		FluidHelper(PxParticleFluid* currFluid, vector<PxVec3> pts, double r);
		void SetFluid(PxParticleFluid* currFluid);
		BasicRandom rand;
		vector<PxVec3> emitPoints;
		int rate;
		int pCount;
		void Emit();
	};
}