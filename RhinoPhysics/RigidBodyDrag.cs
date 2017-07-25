using System;
using System.Collections.Generic;
using System.Linq;
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
    public class RigidBodyDrag : MouseCallback
    {
        public RhinoDoc doc;
        public float depth = 0f;
        public int id = -1;
        public Point3d from;
        public Point3d to;
        public Vector3d delta;
        public bool start = false;
        public Guid guid;
        private Rhino.UI.Gumball.GumballObject base_gumball;
        private GetGumballXform gp;
        private Rhino.UI.Gumball.GumballDisplayConduit dc;
        private bool restoreDynamic = false;

        [System.Runtime.InteropServices.DllImport("user32.dll")]
        public static extern bool GetCursorPos(out System.Drawing.Point pt);

        protected override void OnMouseDown(MouseCallbackEventArgs e)
        {
            base.OnMouseDown(e);

            if (gp != null)
                return;

            if (e.Button.CompareTo(System.Windows.Forms.MouseButtons.Left) != 0)
                return;

            System.Drawing.Point point;
            if (GetCursorPos(out point))
            {
                var pt = ScreenToWorldPoint(point, 0.0, e.View);
                var dir = ScreenPointToVector(point, e.View);

                guid = EngineControl.Raycast(pt, dir, ref depth, ref from);

                if (guid != Guid.Empty)
                {
                    EngineControl.SetKinematic(guid, true);
                    Global.allActors[guid].type = ActorType.RigidKinematic;
                    Global.allActors[guid].selected = true;

                    base_gumball = new Rhino.UI.Gumball.GumballObject();

                    //this sets the initial frame
                    var plane = Plane.WorldXY;
                    plane.Transform(Global.allActors[guid].pose);
                    plane.Origin = Global.allActors[guid].currCenter;
                    base_gumball.SetFromPlane(plane);

                    dc = new Rhino.UI.Gumball.GumballDisplayConduit();
                    var appearance = new Rhino.UI.Gumball.GumballAppearanceSettings();

                    // turn off some of the scale appearance settings to have a slightly different gumball
                    appearance.ScaleXEnabled = false;
                    appearance.ScaleYEnabled = false;
                    appearance.ScaleZEnabled = false;

                    Rhino.Commands.Result cmdrc;

                    start = true;
                    while (true)
                    {
                        dc.SetBaseGumball(base_gumball, appearance);
                        dc.Enabled = true;
                        doc.Views.Redraw();

                        gp = new GetGumballXform(dc);

                        int copy_optindx = gp.AddOption("RestoreDynamic");
                        if (dc.PreTransform == Transform.Identity)
                            gp.SetCommandPrompt("Drag gumball");
                        else
                        {
                            gp.AcceptNothing(true);
                            gp.SetCommandPrompt("Drag gumball. Press Enter when done");
                        }
                        //gp.AddTransformObjects(list);
                        gp.MoveGumball();
                        dc.Enabled = false;
                        cmdrc = gp.CommandResult();

                        if (cmdrc != Rhino.Commands.Result.Success)
                            break;

                        var getpoint_result = gp.Result();
                        if (getpoint_result == Rhino.Input.GetResult.Point)
                        {
                            if (!dc.InRelocate)
                            {
                                Transform xform = dc.TotalTransform;
                                dc.PreTransform = xform;
                            }

                            // update location of base gumball
                            var gbframe = dc.Gumball.Frame;
                            var baseFrame = base_gumball.Frame;
                            baseFrame.Plane = gbframe.Plane;
                            baseFrame.ScaleGripDistance = gbframe.ScaleGripDistance;
                            base_gumball.Frame = baseFrame;

                            continue;
                        }
                        if (getpoint_result == Rhino.Input.GetResult.Option)
                        {
                            if (gp.OptionIndex() == copy_optindx)
                            {
                                EngineControl.SetKinematic(guid, false);
                                Global.allActors[guid].type = ActorType.RigidDynamic;
                                break;
                            }
                        }

                        break;
                    }

                    dc.Enabled = false;
                    dc = null;
                    doc.Views.Redraw();
                    gp = null;
                    Global.allActors[guid].selected = false;
                }

            }
        }
        protected override void OnMouseUp(MouseCallbackEventArgs e)
        {
            base.OnMouseUp(e);

            start = false;

        }

        public void Update()
        {

            if (dc != null)
            {
                //actually move object
                //TODO: modify this to act on transforms
                var orgPt = new Point3d(Global.allActors[guid].currCenter);
                var newPt = dc.Gumball.Frame.Plane.Origin;
               
                var moveVec = new Point3d(newPt - orgPt);
                EngineControl.SetKinematicPose(guid, moveVec);
            }

        }

        public Point3d ScreenToWorldPoint(System.Drawing.Point point, double depth, RhinoView view)
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

class GetGumballXform : Rhino.Input.Custom.GetTransform
{
    readonly Rhino.UI.Gumball.GumballDisplayConduit m_dc;
    public GetGumballXform(Rhino.UI.Gumball.GumballDisplayConduit dc)
    {
        m_dc = dc;
    }

    public override Transform CalculateTransform(Rhino.Display.RhinoViewport viewport, Point3d point)
    {
        if (m_dc.InRelocate)
        {
            // don't move objects while relocating gumball
            return m_dc.PreTransform;
        }

        return m_dc.TotalTransform;
    }

    protected override void OnMouseDown(Rhino.Input.Custom.GetPointMouseEventArgs e)
    {
        if (m_dc.PickResult.Mode != Rhino.UI.Gumball.GumballMode.None)
            return;
        m_dc.PickResult.SetToDefault();

        Rhino.Input.Custom.PickContext pick_context = new Rhino.Input.Custom.PickContext();
        pick_context.View = e.Viewport.ParentView;
        pick_context.PickStyle = Rhino.Input.Custom.PickStyle.PointPick;
        var xform = e.Viewport.GetPickTransform(e.WindowPoint);
        pick_context.SetPickTransform(xform);
        Rhino.Geometry.Line pick_line;
        e.Viewport.GetFrustumLine(e.WindowPoint.X, e.WindowPoint.Y, out pick_line);
        pick_context.PickLine = pick_line;
        pick_context.UpdateClippingPlanes();
        // pick gumball and, if hit, set getpoint dragging constraints.
        m_dc.PickGumball(pick_context, this);
    }

    protected override void OnMouseMove(Rhino.Input.Custom.GetPointMouseEventArgs e)
    {
        if (m_dc.PickResult.Mode == Rhino.UI.Gumball.GumballMode.None)
            return;

        m_dc.CheckShiftAndControlKeys();
        Rhino.Geometry.Line world_line;
        if (!e.Viewport.GetFrustumLine(e.WindowPoint.X, e.WindowPoint.Y, out world_line))
            world_line = Rhino.Geometry.Line.Unset;

        bool rc = m_dc.UpdateGumball(e.Point, world_line);
        if (rc)
            base.OnMouseMove(e);
    }

    protected override void OnDynamicDraw(Rhino.Input.Custom.GetPointDrawEventArgs e)
    {
        // Disable default GetTransform drawing by not calling the base class
        // implementation. All aspects of gumball display are handled by 
        // GumballDisplayConduit
        //base.OnDynamicDraw(e);
    }

    // lets user drag m_gumball around.
    public Rhino.Input.GetResult MoveGumball()
    {
        // Get point on a MouseUp event
        if (m_dc.PreTransform != Transform.Identity)
        {
            HaveTransform = true;
            Transform = m_dc.PreTransform;
        }
        SetBasePoint(m_dc.BaseGumball.Frame.Plane.Origin, false);

        // V5 uses a display conduit to provide display feedback
        // so shaded objects move
        ObjectList.DisplayFeedbackEnabled = true;
        if (Transform != Transform.Identity)
        {
           
        }
        //    ObjectList.UpdateDisplayFeedbackTransform(Transform);

        // Call Get with mouseUp set to true
        var rc = this.Get(true);

        // V5 uses a display conduit to provide display feedback
        // so shaded objects move
        ObjectList.DisplayFeedbackEnabled = false;
        return rc;
    }
}


