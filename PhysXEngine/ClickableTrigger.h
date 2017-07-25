#include "stdafx.h"
#include <PxPhysicsAPI.h>
using namespace physx;
using namespace std;

namespace PhysXEngine
{
	/// simple utility functions for PxCloth
	class ClickableTrigger
	{
	private:
		PxActor* trackedActor;
		PxRigidDynamic* clickableActor;
	public:
		ClickableTrigger();
		ClickableTrigger(PxActor* actorToTrack,PxPhysics* SDK, PxScene* scene, PxMaterial* mat);
	};
}