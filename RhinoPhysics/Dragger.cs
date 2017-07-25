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
    public class Dragger : MouseCallback
    {
        public bool doDrag;
        public float depth = 0f;
        public int id = -1;
        public Point3d from;
        public Point3d to;
        public Guid guid;
        public Guid jointId;
        private MouseCallbackEventArgs mouseArgs;
        [System.Runtime.InteropServices.DllImport("user32.dll")]
        public static extern bool GetCursorPos(out System.Drawing.Point pt);
        protected override void OnMouseDown(MouseCallbackEventArgs e)
        {

            if (e.Button != System.Windows.Forms.MouseButtons.Left)
                return;

            this.doDrag = true;
            System.Drawing.Point point;
            if (GetCursorPos(out point) && jointId == Guid.Empty)
            {
                var pt = ScreenToWorldPoint(point, 0.0, e.View);
                var dir = ScreenPointToVector(point, e.View);
                guid = EngineControl.Raycast(pt, dir, ref depth, ref from);
                if (guid != Guid.Empty)
                {
                    //create joint at pt + depth
                    to = from;
                    jointId = Guid.NewGuid();
                    var interval = new Interval(0, 1);
                    var box = new Box(new Plane(from, Vector3d.ZAxis), interval, interval, interval);
                    var mbox = Mesh.CreateFromBox(box, 1, 1, 1);
                    EngineControl.AddEmpty(jointId, mbox, from);
                    EngineControl.SetKinematic(jointId, true);
                    EngineControl.AddDistanceJoint(jointId, guid, jointId, from, to, 0, 0, 100000, 0);
                    //EngineControl.SetRigidDynamicMass(guid, 5);
                    EngineControl.SetRigidDynamicDrag(guid, 10);
                    this.mouseArgs = e;
                }
            }
            base.OnMouseDown(e);
        }

        protected override void OnMouseDoubleClick(MouseCallbackEventArgs e)
        {
            //if (guid != Guid.Empty && e.Button == System.Windows.Forms.MouseButtons.Left)
            //{
            //    var ra = e.View.Document.Objects.Find(guid) as RigidBodyActor;
            //    ra.kinematic = !ra.kinematic;
            //    EngineControl.SetKinematic(guid, ra.kinematic);
            //    if (ra.kinematic)
            //        e.View.Document.Objects.Unlock(guid, true);
            //    else
            //        e.View.Document.Objects.Lock(guid, true);
            //}
            //base.OnMouseDoubleClick(e);
        }

        protected override void OnMouseUp(MouseCallbackEventArgs e)
        {

            if (e.Button != System.Windows.Forms.MouseButtons.Left)
                return;
            base.OnMouseUp(e);
            this.doDrag = false;
            if (jointId != Guid.Empty)
            {
                //EngineControl.RemoveJoint(jointId);
                EngineControl.RemoveActor(jointId);
                //EngineControl.SetRigidDynamicMass(guid, 10);
                EngineControl.SetRigidDynamicDrag(guid, 0.5);
                jointId = Guid.Empty;
                this.mouseArgs = null;
            }
        }
        public void Update()
        {
            if (doDrag)
            {
                System.Drawing.Point point;
                if (GetCursorPos(out point) && this.mouseArgs != null)
                {
                    to = ScreenToWorldPoint(point, depth, this.mouseArgs.View);
                    EngineControl.SetKinematicPose(jointId, Transform.Translation(new Vector3d(to)));
                    //EngineControl.SetJointEndPoint(jointId, 1, to);
                }
            }

        }

        public static Point3d ScreenToWorldPoint(System.Drawing.Point point, double depth, RhinoView view)
        {
            // Convert the screen coordinates to client coordinates
            point = view.ActiveViewport.ScreenToClient(point);

            // Obtain the view's screen-to-world transformation
            Rhino.Geometry.Transform screen_to_world = view.ActiveViewport.GetTransform(
              Rhino.DocObjects.CoordinateSystem.Screen,
              Rhino.DocObjects.CoordinateSystem.World);

            // Create a 3-D point
            Point3d pt = new Point3d(point.X, point.Y, 0.0);
            Point3d pt1 = new Point3d(point.X, point.Y, 1.0);

            // Transform it
            pt.Transform(screen_to_world);
            pt1.Transform(screen_to_world);

            var dir = pt1 - pt;
            dir.Unitize();

            return pt + depth * dir;
        }

        public static Vector3d ScreenPointToVector(System.Drawing.Point point, RhinoView view)
        {
            // Convert the screen coordinates to client coordinates
            point = view.ActiveViewport.ScreenToClient(point);

            // Obtain the view's screen-to-world transformation
            Rhino.Geometry.Transform screen_to_world = view.ActiveViewport.GetTransform(
              Rhino.DocObjects.CoordinateSystem.Screen,
              Rhino.DocObjects.CoordinateSystem.World
              );

            // Create a 3-D point
            Rhino.Geometry.Point3d pt = new Rhino.Geometry.Point3d(point.X, point.Y, 0.0);
            Rhino.Geometry.Point3d pt1 = new Rhino.Geometry.Point3d(point.X, point.Y, 1.0);

            // Transform it
            pt.Transform(screen_to_world);
            pt1.Transform(screen_to_world);

            var dir = pt1 - pt;
            dir.Unitize();

            return dir;
        }
    }
}
