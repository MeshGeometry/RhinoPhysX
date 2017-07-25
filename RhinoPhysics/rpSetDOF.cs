using System;
using Rhino;
using Rhino.Commands;
using Rhino.Input.Custom;
using Rhino.Geometry;

namespace RhinoPhysics
{
    [System.Runtime.InteropServices.Guid("b7157083-166e-45a7-8257-8ec61c97744b")]
    public class rpSetDOF : Command
    {
        static rpSetDOF _instance;
        public rpSetDOF()
        {
            _instance = this;
        }

        ///<summary>The only instance of the rpSetDOF command.</summary>
        public static rpSetDOF Instance
        {
            get { return _instance; }
        }

        public override string EnglishName
        {
            get { return "rpSetDOF"; }
        }

        protected override Result RunCommand(RhinoDoc doc, RunMode mode)
        {
            if (!Global.active)
                return Result.Cancel;

            var go = new Rhino.Input.Custom.GetObject();
            go.SetCommandPrompt("Select Objects for DOF modification:");
            var dX = new OptionToggle(false, "Free", "Lock");
            var dY = new OptionToggle(false, "Free", "Lock");
            var dZ = new OptionToggle(false, "Free", "Lock");
            var rX = new OptionToggle(false, "Free", "Lock");
            var rY = new OptionToggle(false, "Free", "Lock");
            var rZ = new OptionToggle(false, "Free", "Lock");
            go.AddOptionToggle("dX", ref dX);
            go.AddOptionToggle("dY", ref dY);
            go.AddOptionToggle("dZ", ref dZ);
            go.AddOptionToggle("rX", ref rX);
            go.AddOptionToggle("rY", ref rY);
            go.AddOptionToggle("rZ", ref rZ);
            while (true)
            {
                // perform the get operation. This will prompt the user to input a point, but also
                // allow for command line options defined above
                Rhino.Input.GetResult get_rc = go.GetMultiple(1, 0);

                if (go.CommandResult() != Rhino.Commands.Result.Success)
                    return go.CommandResult();

                if (get_rc == Rhino.Input.GetResult.Option)
                {
                    continue;
                }

                break;
            }

            //get the origin point
            var gp = new Rhino.Input.Custom.GetPoint();
            gp.SetCommandPrompt("Select the point to fix");
            gp.AcceptNothing(true);
            gp.Get();

            Point3d org = gp.Point();
            if (org == null)
                org = Point3d.Origin;

            //create the flags
            int flags = 0;
            if (dX.CurrentValue)
                flags |= 1;
            if (dY.CurrentValue)
                flags |= 2;
            if (dZ.CurrentValue)
                flags |= 4;
            if (rX.CurrentValue)
                flags |= 8;
            if (rY.CurrentValue)
                flags |= 16;
            if (rZ.CurrentValue)
                flags |= 32;

            for (int i = 0; i < go.ObjectCount; i++)
            {
                if (Global.actors.Contains(go.Object(i).ObjectId))
                {
                    var obj = go.Object(i).Object() as RigidBodyActor;
                    obj.AddConstraint(flags, org);
                }
            }

            doc.Views.Redraw();
            return Result.Success;
        }
    }
}
