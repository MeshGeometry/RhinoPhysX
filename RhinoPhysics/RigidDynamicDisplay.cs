using System.Text;
using System.Threading.Tasks;

using Rhino;
using Rhino.Display;
using Rhino.Geometry;

namespace RhinoPhysics
{
    public class RigidDynamicDisplay : DisplayConduit
    {
        private Transform rot = Transform.Rotation(0.5 * 3.141592, Vector3d.XAxis, Point3d.Origin);
        private Transform rotInv = Transform.Rotation(-0.5 * 3.141592, Vector3d.XAxis, Point3d.Origin);
        private DisplayMaterial[] materials;

        public RigidDynamicDisplay()
        {
            materials = new DisplayMaterial[]{
                new DisplayMaterial(System.Drawing.Color.Aqua, 0.3),
                new DisplayMaterial(System.Drawing.Color.SteelBlue, 0.7),
                new DisplayMaterial(System.Drawing.Color.Red, 0.7)
            };
        }

        protected override void CalculateBoundingBox(CalculateBoundingBoxEventArgs e)
        {
            base.CalculateBoundingBox(e);

            var box = new BoundingBox(new Point3d(-1000.0, -1000.0, -1000.0), new Point3d(1000.0, 1000.0, 1000.0));
            e.IncludeBoundingBox(box);
        }
        protected override void PreDrawObjects(DrawEventArgs e)
        {
            base.PreDrawObjects(e);

            var allGameObjects = Global.allActors.Values;

            foreach (var go in allGameObjects)
            {
                if (go.type == ActorType.RigidKinematic || go.type == ActorType.RigidDynamic)
                {

                    //only quirk not handled internally. Need to undo the way PhysX defines collisions shapes at
                    //origin and then transforms them to their position in space. Basically, a local to world transformation.
                    //var x = Transform.Multiply(go.pose, Transform.Translation((Vector3d)(-go.oPos)));
                    e.Display.PushModelTransform(go.pose);
                    if (go.type == ActorType.RigidDynamic)
                    {
                        e.Display.DrawMeshShaded(go.displayMesh, materials[1]);
                        e.Display.DrawMeshWires(go.displayMesh, System.Drawing.Color.Aqua);
                    }
                    else
                    {
                        e.Display.DrawMeshShaded(go.displayMesh, new DisplayMaterial(System.Drawing.Color.DarkCyan, 0.3));
                        e.Display.DrawMeshWires(go.displayMesh, System.Drawing.Color.DarkCyan);
                    }

                    switch (go.collisionType)
                    {
                        case CollisionType.Box:
                            var box = new Box(go.orgBox);
                            e.Display.DrawBox(box, System.Drawing.Color.Black, 2);
                            break;
                        case CollisionType.Sphere:
                            e.Display.DrawSphere(new Sphere(go.oPos, go.rad), System.Drawing.Color.Black, 2);
                            break;
                        case CollisionType.Capsule:
                            var circ = new Circle(go.oPos - go.height * Vector3d.ZAxis, go.rad);
                            e.Display.DrawCylinder(new Cylinder(circ, go.height * 2.0), System.Drawing.Color.Black, 2);
                            break;
                        case CollisionType.Convex:
                            break;
                    }
                    if (go.selected)
                        e.Display.DrawMeshWires(go.displayMesh, System.Drawing.Color.Yellow);

                    e.Display.PopModelTransform();
                }
            }
        }
    }
}
