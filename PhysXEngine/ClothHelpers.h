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
#include <vector>

using namespace physx;
using namespace std;

namespace PhysXEngine
{
	/// simple utility functions for PxCloth
	class ClothHelpers
	{
	public:

		// border flags
		enum BorderFlags
		{
			NONE = 0,
			BORDER_LEFT		= (1 << 0),
			BORDER_RIGHT	= (1 << 1),
			BORDER_BOTTOM	= (1 << 2),
			BORDER_TOP		= (1 << 3),
			BORDER_NEAR		= (1 << 4),
			BORDER_FAR		= (1 << 5)
		};

		// attach cloth border 
		static bool attachBorder(PxClothParticle* particles, PxU32 numParticles, BorderFlags borderFlag);
		static bool attachBorder(PxCloth& cloth, BorderFlags borderFlag);

		// constrain cloth particles that overlap the given shape
		static bool attachClothOverlapToShape(PxCloth& cloth, PxShape& shape, PxReal radius = 0.1f);

		// create cloth mesh descriptor for a grid mesh defined along two (u,v) axis.
		static PxClothMeshDesc createMeshGrid(PxVec3 dirU, PxVec3 dirV, PxU32 numU, PxU32 numV,
			vector<PxVec4>& vertices, vector<PxU32>& indices, vector<PxVec2>& texcoords);

		// create cloth mesh from obj file
		static PxClothMeshDesc createMeshFromObj(const char* filename, PxReal scale, PxQuat rot, PxVec3 offset, 
			vector<PxVec4>& vertices, vector<PxU32>& indices, vector<PxVec2>& texcoords);

		// create capsule data in local space of pose
		static bool createCollisionCapsule(const PxTransform &pose, const PxVec3 &center0, PxReal r0, const PxVec3 &center1, PxReal r1, 
			vector<PxClothCollisionSphere> &spheres, vector<PxU32> &indexPairs);

		// create virtual particle samples
		static bool createVirtualParticles(PxCloth& cloth, PxClothMeshDesc& meshDesc, int level);

		// get world bounds containing all the colliders and the cloth
		static PxBounds3 getAllWorldBounds(PxCloth& cloth);

		// get particle location from the cloth
		static bool getParticlePositions(PxCloth&cloth, vector<PxVec3> &positions);

		// remove duplicate vertices 
		static PxClothMeshDesc removeDuplicateVertices(PxClothMeshDesc &inMesh,  vector<PxVec2> &inTexcoords,
			vector<PxVec4>& vertices, vector<PxU32>& indices, vector<PxVec2>& texcoords);

		// merge two mesh descriptor and uvs into a single one
		static PxClothMeshDesc mergeMeshDesc(PxClothMeshDesc &desc1, PxClothMeshDesc &desc2,  
			vector<PxVec4>& vertices, vector<PxU32>& indices,
			vector<PxVec2>& texcoords1, vector<PxVec2>& texcoords2, vector<PxVec2>& texcoords);  

		// set motion constraint radius
		static bool setMotionConstraints(PxCloth &cloth, PxReal radius);

		// set particle location from the cloth
		static bool setParticlePositions(PxCloth&cloth, const vector<PxVec3> &positions, bool useConstrainedOnly = true, bool useCurrentOnly = true);

		// set stiffness for all the phases
		static bool setStiffness(PxCloth& cloth, PxReal stiffness);
	};
}
