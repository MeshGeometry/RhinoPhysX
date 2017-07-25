using System;
using Rhino;
using Rhino.Commands;
using Rhino.Geometry;

namespace RhinoPhysics
{
    [System.Runtime.InteropServices.Guid("316c530d-07c9-4d53-b620-4f0876f171ca")]
    public class rpBake : Command
    {
        static rpBake _instance;
        public rpBake()
        {
            _instance = this;
        }

        ///<summary>The only instance of the rpBake command.</summary>
        public static rpBake Instance
        {
            get { return _instance; }
        }

        public override string EnglishName
        {
            get { return "rpBake"; }
        }

        protected override Result RunCommand(RhinoDoc doc, RunMode mode)
        {
            if (Global.allActors == null)
                return Result.Success;

            var allObj = Global.allActors.Values;
            foreach (var go in allObj)
            {
                if (go.type == ActorType.RigidDynamic)
                {
                    //var x = Transform.Multiply(go.pose, Transform.Translation((Vector3d)(-go.oPos)));
                    doc.Objects.Transform(go.objRef, go.pose, false);
                }

                else if (go.type == ActorType.Cloth)
                {
                    doc.Objects.AddMesh(go.displayMesh);
                }

                else if (go.type == ActorType.FluidEmmiter)
                {
                    doc.Objects.AddPointCloud(go.displayPoints);
                }
              
            }
            return Result.Success;
        }
    }
}
