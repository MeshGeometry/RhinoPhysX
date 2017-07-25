using System.Text;
using System.Threading.Tasks;

using Rhino;
using Rhino.Display;
using Rhino.Geometry;

namespace RhinoPhysics
{
    public class ClothDisplay : DisplayConduit
    {
        private DisplayMaterial[] materials;
        public ClothDisplay()
        {
            materials = new DisplayMaterial[]{
                new DisplayMaterial(System.Drawing.Color.Aqua, 0.3),
                new DisplayMaterial(System.Drawing.Color.SteelBlue, 0.7),
                new DisplayMaterial(System.Drawing.Color.OrangeRed, 0.7)
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

            var mat = new DisplayMaterial(System.Drawing.Color.SteelBlue, 0.7);
            var allGameObjects = Global.allActors.Values;
            foreach (var go in allGameObjects)
            {
                if (go.type != ActorType.Cloth)
                    continue;

                e.Display.DrawMeshShaded(go.displayMesh, materials[2]);
                e.Display.DrawMeshWires(go.displayMesh, System.Drawing.Color.DarkOrange, 1);
            }
        }
    }
}
