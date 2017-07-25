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
    [System.Runtime.InteropServices.Guid("da4d4b71-cd31-4449-a80b-b09343bb6985")]
    public class rpRevoluteJoint : Command
    {
        static rpRevoluteJoint _instance;
        public rpRevoluteJoint()
        {
            _instance = this;
        }

        ///<summary>The only instance of the rpRevoluteJoint command.</summary>
        public static rpRevoluteJoint Instance
        {
            get { return _instance; }
        }

        public override string EnglishName
        {
            get { return "rpRevoluteJoint"; }
        }

        protected override Result RunCommand(RhinoDoc doc, RunMode mode)
        {
            var opMaxD = new OptionDouble(0, true, 0);
            var opMinD = new OptionDouble(0, true, 0);
            var opStiffness = new OptionDouble(0, 0, 10000);
            var opBreakForce = new OptionDouble(0, 0, 10000);
            var opDriveVel = new OptionDouble(0, 0, 1000);

            var go = new Rhino.Input.Custom.GetObject();
            go.SetCommandPrompt("Select Objects for Joint Creation:");

            go.AddOptionDouble("MaxAngle", ref opMaxD);
            go.AddOptionDouble("MinAngle", ref opMinD);
            go.AddOptionDouble("Stiffness", ref opStiffness);
            go.AddOptionDouble("BreakForce", ref opBreakForce);
            go.AddOptionDouble("DriveVel", ref opDriveVel);

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
            var line = new Line();
            var gl = new GetLine();
            gl.Get(out line);
            gl.FeedbackColor = System.Drawing.Color.CadetBlue;

            var maxAngle = (opMaxD.CurrentValue / 180) * Math.PI;
            var minAngle = (opMinD.CurrentValue / 180) * Math.PI;
            var newID = Guid.NewGuid();
            EngineControl.AddRevoluteJoint(newID, go.Object(0).ObjectId, go.Object(1).ObjectId, line.From, line.To, maxAngle, minAngle, opStiffness.CurrentValue, opBreakForce.CurrentValue, opDriveVel.CurrentValue);
            Global.joints.Add(newID);
            doc.Views.Redraw();

            return Result.Success;
        }
    }
}
