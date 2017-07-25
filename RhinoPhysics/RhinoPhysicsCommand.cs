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
    [System.Runtime.InteropServices.Guid("c48f3a37-852c-403b-93f1-9d0c0268be20")]
    public class RhinoPhysicsCommand : Command
    {
        public DispatcherTimer Timer;
        public RhinoDoc doc;

        public RigidBodyDrag dragger;

        public RhinoPhysicsCommand()
        {
            // Rhino only creates one instance of each command class defined in a
            // plug-in, so it is safe to store a refence in a static property.
            Instance = this;
        }

        ///<summary>The only instance of this command.</summary>
        public static RhinoPhysicsCommand Instance
        {
            get;
            private set;
        }

        ///<returns>The command name as it appears on the Rhino command line.</returns>
        public override string EnglishName
        {
            get { return "RhinoPhysicsTest"; }
        }

        protected override Result RunCommand(RhinoDoc doc, RunMode mode)
        {
            this.doc = doc;

            var rDisplay = new RigidDynamicDisplay();
            rDisplay.Enabled = true;


            // Hook up the Elapsed event for the timer.
            Timer = new DispatcherTimer();
            Timer.Interval = TimeSpan.FromMilliseconds(10);
            Timer.Tick += new EventHandler(MainLoop);
            Timer.IsEnabled = false;
            

            RhinoApp.WriteLine("Initialized the counters");

            //create the mouse dragger
            dragger = new RigidBodyDrag();

            //Hook up escape key handler
            RhinoApp.EscapeKeyPressed += new EventHandler(OnEscEvent);

            //create joint display
            var jDisplay = new JointLine();
            jDisplay.Enabled = true;

            var go = new Rhino.Input.Custom.GetObject();
            go.SetCommandPrompt("Select Objects for RigidBody Simulation:");

            string[] listValues = new string[] { "Box", "Sphere", "Capsule", "Convex" };

            int listIndex = 0;
            int opList = go.AddOptionList("ColliderOptions", listValues, listIndex);

            while (true)
            {
                // perform the get operation. This will prompt the user to input a point, but also
                // allow for command line options defined above
                Rhino.Input.GetResult get_rc = go.GetMultiple(1, 0);

                if (go.CommandResult() != Rhino.Commands.Result.Success)
                    return go.CommandResult();

                if (get_rc == Rhino.Input.GetResult.Option)
                {
                    if (go.OptionIndex() == opList)
                        listIndex = go.Option().CurrentListOptionIndex;
                    continue;
                }

                break;
            }

            for (int i = 0; i < go.ObjectCount; i++)
            {
                Mesh m = go.Object(i).Mesh();
                var tmpGo = new GameObject(go.Object(i).ObjectId, m, Transform.Identity, ActorType.RigidDynamic);
                EngineControl.AddConvexRigidDynamic(tmpGo.objRef, m, tmpGo.oPos, true);

                //Global.allActors.Add(tmpGo);
            }

            return Result.Success;
        }

        private void MainLoop(object source, EventArgs arg)
        {
            if (Global.allActors.Count > 0)
            {

            }
        }

        private void OnEscEvent(object source, EventArgs e)
        {
            if (Timer.IsEnabled)
                Timer.IsEnabled = false;
            else
                Timer.IsEnabled = true;

        }
    }
}
