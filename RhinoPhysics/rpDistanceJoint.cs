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
    [System.Runtime.InteropServices.Guid("0dc9a92d-4872-4c42-8a8a-ff71ebb1adfa")]
    public class rpDistanceJoint : Command
    {
        static rpDistanceJoint _instance;
        public rpDistanceJoint()
        {
            _instance = this;
        }

        ///<summary>The only instance of the rpJoint command.</summary>
        public static rpDistanceJoint Instance
        {
            get { return _instance; }
        }

        public override string EnglishName
        {
            get { return "rpDistanceJoint"; }
        }

        protected override Result RunCommand(RhinoDoc doc, RunMode mode)
        {
            var opMaxD = new OptionDouble(0, true, -1);
            var opMinD = new OptionDouble(0, true, -1);
            var opStiffness = new OptionDouble(0, 0, 10000);
            var opBreakForce = new OptionDouble(0, 0, 10000);

            var go = new Rhino.Input.Custom.GetObject();
            go.SetCommandPrompt("Select Objects for Joint Creation:");

            go.AddOptionDouble("MaxDistance", ref opMaxD);
            go.AddOptionDouble("MinDistance", ref opMinD);
            go.AddOptionDouble("Stiffness", ref opStiffness);
            go.AddOptionDouble("BreakForce", ref opBreakForce);

            while (true)
            {
                // perform the get operation. This will prompt the user to input a point, but also
                // allow for command line options defined above
                Rhino.Input.GetResult get_rc = go.GetMultiple(1, 2);

                if (go.CommandResult() != Rhino.Commands.Result.Success)
                    return go.CommandResult();

                if (go.ObjectCount >= 2)
                    break;

                if (get_rc == Rhino.Input.GetResult.Option)
                    continue;

                break;
            }

            if (!Global.actors.Contains(go.Object(0).ObjectId) || go.Object(0).ObjectId == go.Object(1).ObjectId)
            {
                RhinoApp.WriteLine("Could not create joint. The first object must always be a valid rigid body actor.");
                return Result.Failure;
            }

            //get the points
            var pt1 = new Point3d();
            var gp = new GetPoint();
            gp.SetCommandPrompt("Select first point");
            gp.Get();
            pt1 = gp.Point();

            var pt2 = new Point3d();
            gp.SetCommandPrompt("Select second point");
            gp.Get();
            pt2 = gp.Point();
            var newID = Guid.NewGuid();
            EngineControl.AddDistanceJoint(newID, go.Object(0).ObjectId, go.Object(1).ObjectId, pt1, pt2, opMaxD.CurrentValue, opMinD.CurrentValue, opStiffness.CurrentValue, opBreakForce.CurrentValue);
            Global.joints.Add(newID);
            doc.Views.Redraw();

            return Result.Success;
        }
    }
}
