using System;
using System.Collections.Generic;
using Rhino;
using Rhino.Commands;
using Rhino.Geometry;
using Rhino.DocObjects;
using Rhino.Display;
using PhysXEngineWrapper;

namespace RhinoPhysics
{
    public enum ActorType
    {
        Rigid,
        Cloth,
        Fluid,
        Joint
    }
    
    public class ActorBase : Rhino.DocObjects.Custom.CustomMeshObject
    {
        public ActorType actorType;

        public virtual void Update()
        {
        }

        protected override void OnDuplicate(RhinoObject source)
        {
            var obj = (ActorBase)source;
            this.actorType = obj.actorType;
            base.OnDuplicate(source);
        }
    }
}
