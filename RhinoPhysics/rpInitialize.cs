using System;
using System.Collections.Generic;
using System.Windows.Threading;
using Rhino;
using Rhino.Commands;
using Rhino.Geometry;
using Rhino.Input;
using Rhino.Input.Custom;

using PhysXEngineWrapper;

namespace RhinoPhysics
{
    [System.Runtime.InteropServices.Guid("f5441cb4-ca6b-4edb-a5cf-509410d405bb")]
    public class rpInitialize : Command
    {
        static rpInitialize _instance;
        public rpInitialize()
        {
            _instance = this;
        }

        ///<summary>The only instance of the rpInitialize command.</summary>
        public static rpInitialize Instance
        {
            get { return _instance; }
        }

        public override string EnglishName
        {
            get { return "rpInitialize"; }
        }

        protected override Result RunCommand(RhinoDoc doc, RunMode mode)
        {
            var opSimMode = new OptionToggle(true, "CPU", "GPU");
            var opGravity = new OptionDouble(-9.81);
            var opGroundPlane = new OptionToggle(true, "Off", "On");
            var opClothInterCollision = new OptionDouble(1, true, 0);
            var opClothCollisionStiffness = new OptionDouble(1, true, 0);
            var opTimeStep = new OptionDouble(0.03, 0.0001, 0.1);
            var opSubSteps = new OptionInteger(6, 0, 100);

            GetOption go = new GetOption();
            go.SetCommandPrompt("Initializes a new Physics Scene:");
            go.AcceptNothing(true);

            go.AddOptionToggle("SimulationMode", ref opSimMode);
            go.AddOptionDouble("Gravity", ref opGravity);
            go.AddOptionToggle("GroundPlane", ref opGroundPlane);
            go.AddOptionDouble("TimeStep", ref opTimeStep);
            go.AddOptionInteger("SubSteps", ref opSubSteps);
            go.AddOptionDouble("ClothInterCollisionDistance", ref opClothInterCollision);
            go.AddOptionDouble("ClothInterCollisionStiffness", ref opClothCollisionStiffness);

            while (true)
            {
                // perform the get operation. This will prompt the user to input a point, but also
                // allow for command line options defined above
                Rhino.Input.GetResult get_rc = go.Get();

                if (go.CommandResult() != Rhino.Commands.Result.Success)
                    return go.CommandResult();

                if (get_rc == Rhino.Input.GetResult.Option)
                {
                    continue;
                }

                break;
            }

            bool success = false;

            RhinoApp.WriteLine("Current State? " + Global.active.ToString());

            if (!Global.active)
            {
                success = EngineControl.InitPhysics(opSimMode.CurrentValue, opGroundPlane.CurrentValue, opGravity.CurrentValue * Vector3d.ZAxis);
                Global.active = true;
                Global.timeStep = opTimeStep.CurrentValue;
                Global.substeps = opSubSteps.CurrentValue;
                Global.actors = new List<Guid>();
                Global.rDoc = doc;
                Global.Timer = new DispatcherTimer();
                Global.UIMonitor = new DispatcherTimer();
                Global.UIMonitor.Interval = TimeSpan.FromMilliseconds(16);
                Global.UIMonitor.Tick += new EventHandler(Global.UILoop);
                Global.UIMonitor.Start();
                Global.UIMonitor.IsEnabled = true;
                Global.dataDisplay = new DataDisplay();
                Global.dataDisplay.Enabled = true;
                Global.joints = new List<Guid>();
            }
            else
            {
                if (Global.Timer != null)
                    Global.Timer.Stop();
                EngineControl.ShutdownPhysX();
                Global.timeStep = opTimeStep.CurrentValue;
                Global.substeps = opSubSteps.CurrentValue;
                Global.actors = new List<Guid>();
                Global.rDoc = doc;
                Global.Timer = new DispatcherTimer();
                Global.UIMonitor = new DispatcherTimer();
                Global.UIMonitor.Interval = TimeSpan.FromMilliseconds(16);
                Global.UIMonitor.Tick += new EventHandler(Global.UILoop);
                Global.UIMonitor.Start();
                Global.UIMonitor.IsEnabled = true;
                Global.dataDisplay = new DataDisplay();
                Global.joints = new List<Guid>();
                Global.dataDisplay.Enabled = true;
                success = EngineControl.InitPhysics(opSimMode.CurrentValue, opGroundPlane.CurrentValue, opGravity.CurrentValue * Vector3d.ZAxis);
            }

            doc.Views.Redraw();

            RhinoApp.WriteLine("Has RhinoPhysics initialized? " + success.ToString());

            return Result.Success;
        }
    }
}
