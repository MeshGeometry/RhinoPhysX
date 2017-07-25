using System;
using System.Collections;
using System.Collections.Generic;
using System.Windows.Threading;
using System.Linq;
using Rhino;
using Rhino.Commands;
using Rhino.Geometry;
using Rhino.Input;
using Rhino.Input.Custom;

using PhysXEngineWrapper;

namespace RhinoPhysics
{
    [System.Runtime.InteropServices.Guid("d9ec1589-e8f1-449b-9d56-dd8e55e4d7b3")]
    public class rpSetClothParticleWeights : Command
    {
        static rpSetClothParticleWeights _instance;
        public rpSetClothParticleWeights()
        {
            _instance = this;
        }

        ///<summary>The only instance of the rpSetClothParticleWeights command.</summary>
        public static rpSetClothParticleWeights Instance
        {
            get { return _instance; }
        }

        public override string EnglishName
        {
            get { return "rpSetClothParticleWeights"; }
        }

        protected override Result RunCommand(RhinoDoc doc, RunMode mode)
        {
            if (!Global.active)
                return Result.Cancel;

            var allPoints = new List<Point3d>();

            // selects multiple points that already exist
            Rhino.DocObjects.ObjRef[] obj_refs;
            
            var rc = RhinoGet.GetMultipleObjects("Select Cloth Vertices", false, Rhino.DocObjects.ObjectType.Grip, out obj_refs);
            
            if (rc != Result.Success)
                return rc;
            foreach (var o_ref in obj_refs)
            {
                var point = o_ref.Point();
                allPoints.Add(new Point3d(
                  point.Location.X,
                  point.Location.Y,
                  point.Location.Z));
            }
            doc.Objects.UnselectAll();
            var weight = 0.0;
            var go = RhinoGet.GetNumber("VertexWeight", true, ref weight, 0.0, 1.0);
            //doc.Objects.UnselectAll();
            for (int i = 0; i < allPoints.Count; i++)
            {
                var currPoint = allPoints[i];
                for (int j = 0; j < Global.actors.Count; j++)
                {
                    //make sure the actor is cloth or mesh actor
                    var a = doc.Objects.Find(Global.actors[j]);
                    if (a.GetType() == typeof(ClothActor) || a.GetType() == typeof(Rhino.DocObjects.MeshObject))
                    {
                        //check bounding box
                        BoundingBox currBox = a.Geometry.GetBoundingBox(true);

                        if (currBox.Contains(currPoint, false))
                        {
                            var mesh = (Mesh)a.Geometry;
                            for (int k = 0; k < mesh.Vertices.Count; k++)
                            {
                                var dist = new Point3d(mesh.Vertices[k]).DistanceTo(currPoint);

                                if (dist < 0.01)
                                {
                                    EngineControl.SetClothParticleWeights(Global.actors[j], new int[] { k }, weight);
                                    break;
                                }
                            }
                        }
                    }
                }
            }

            return Result.Success;
        }
    }
}
