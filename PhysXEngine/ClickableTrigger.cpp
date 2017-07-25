#include "stdafx.h"
#include "ClickableTrigger.h"
#include <PxPhysicsAPI.h>
using namespace physx;
using namespace std;

namespace PhysXEngine
{
	ClickableTrigger::ClickableTrigger()
	{
	}

	ClickableTrigger::ClickableTrigger(PxActor* actorToTrack,PxPhysics* SDK, PxScene* scene, PxMaterial* mat)
	{
		trackedActor = actorToTrack;
		PxBounds3 bounds = trackedActor->getWorldBounds();
		PxBoxGeometry box(0.5 * bounds.getDimensions());
		PxTransform pose(bounds.getCenter(), PxQuat::PxQuat());
		clickableActor = SDK->createRigidDynamic(PxTransform::PxTransform());
		PxShape* boxShape = clickableActor->createShape(box, *mat);

		clickableActor->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, true);		
		boxShape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, false);
		boxShape->setFlag(PxShapeFlag::eTRIGGER_SHAPE, true);

		scene->addActor(*clickableActor);
		clickableActor->userData = (void*)this;
	}
}