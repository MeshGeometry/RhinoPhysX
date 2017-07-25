using System;
using System.Collections;
using System.Collections.Generic;
using Rhino;
using Rhino.Commands;
using Rhino.Display;
using Rhino.DocObjects;
using Rhino.Geometry;
using Rhino.Geometry.Intersect;
using Rhino.Input;
using Rhino.Input.Custom;
using Rhino.UI;

using PhysXEngineWrapper;

namespace RhinoPhysics
{
    public class ClothDragger : MouseCallback
    {
        public bool doDrag;
        public double depth = 0f;
        public int id = -1;
        public Point3d from;
        public Point3d to;
        public Guid guid;
        private MouseCallbackEventArgs mouseArgs;
        [System.Runtime.InteropServices.DllImport("user32.dll")]
        public static extern bool GetCursorPos(out System.Drawing.Point pt);
        protected override void OnMouseDown(MouseCallbackEventArgs e)
        {

            if (e.Button != System.Windows.Forms.MouseButtons.Left)
                return;

            System.Drawing.Point point;
            if (GetCursorPos(out point))
            {
                mouseArgs = e;
                var pt = Dragger.ScreenToWorldPoint(point, 0.0, e.View);
                var dir = Dragger.ScreenPointToVector(point, e.View);

                if (Raycast(pt, dir, out from, out guid))
                {
                    this.doDrag = true;
                    depth = from.DistanceTo(pt);
                    id = EngineControl.FindClosestClothParticle(guid, from);
                }

            }
            base.OnMouseDown(e);
        }

        bool Raycast(Point3d o, Vector3d dir, out Point3d ptOut, out Guid id)
        {
            var d = double.MaxValue;
            var ray = new Ray3d(o, dir);
            var success = false;
            var currId = Guid.Empty;
            foreach (var a in Global.actors)
            {
                var obj = mouseArgs.View.Document.Objects.Find(a);
                if (obj.GetType() == typeof(ClothActor) || obj.GetType() == typeof(MeshObject))
                {
                    //intersect
                    var dTest = Intersection.MeshRay((Mesh)obj.Geometry, ray);
                    if (dTest < d && dTest > 0)
                    {
                        d = dTest;
                        currId = a;
                    }
                }
            }

            if (d > 0)
            {
                success = true;
                ptOut = ray.PointAt(d);
            }
            else
            {
                ptOut = Point3d.Origin;
            }
            id = currId;
            return success;
        }

        protected override void OnMouseDoubleClick(MouseCallbackEventArgs e)
        {
            //if (guid != Guid.Empty && e.Button == System.Windows.Forms.MouseButtons.Left && id >= 0)
            //{

            //}
            //base.OnMouseDoubleClick(e);
        }

        protected override void OnMouseUp(MouseCallbackEventArgs e)
        {

            if (e.Button != System.Windows.Forms.MouseButtons.Left)
                return;
            if (guid != Guid.Empty)
            {
                EngineControl.SetClothParticlePosition(guid, id, to, true);
                id = -1;
                guid = Guid.Empty;
            }
            base.OnMouseUp(e);
            this.doDrag = false;

        }
        public void Update()
        {
            if (doDrag)
            {
                System.Drawing.Point point;
                if (GetCursorPos(out point) && this.mouseArgs != null)
                {
                    to = Dragger.ScreenToWorldPoint(point, depth, this.mouseArgs.View);
                    if (id >= 0)
                        EngineControl.SetClothParticlePosition(guid, id, to, false);
                }
            }

        }
    }
}

