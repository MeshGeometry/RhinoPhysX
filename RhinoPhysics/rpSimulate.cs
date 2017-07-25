using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Windows.Threading;
using Rhino;
using Rhino.Commands;
using Rhino.Geometry;
using Rhino.Input;
using Rhino.Input.Custom;

using PhysXEngineWrapper;

namespace RhinoPhysics
{
    [System.Runtime.InteropServices.Guid("ad431710-9ed3-49f1-98db-493e4365cfd0")]
    public class rpSimulate : Command
    {
        static rpSimulate _instance;
        public DispatcherTimer Timer;
        public RhinoDoc doc;
        public Dragger dragger;
        public ClothDragger clothDragger;
        Stopwatch frameTimer;
        double maxFrame = 0;
        public rpSimulate()
        {
            _instance = this;
        }

        ///<summary>The only instance of the rpSimulate command.</summary>
        public static rpSimulate Instance
        {
            get { return _instance; }
        }

        public override string EnglishName
        {
            get { return "rpSimulate"; }
        }
        protected override Result RunCommand(RhinoDoc doc, RunMode mode)
        {

            if (!Global.active || Global.readyToSimulate)
                return Result.Cancel;

            this.doc = doc;
            Global.rDoc = doc;

            // Hook up the Elapsed event for the timer.
            Timer = new DispatcherTimer(DispatcherPriority.Send);
            Timer.Interval = TimeSpan.FromMilliseconds(1);
            Timer.Tick += new EventHandler(MainLoop);
            Timer.Start();
            Timer.IsEnabled = true;

            frameTimer = new Stopwatch();

            RhinoApp.WriteLine("Initialized the counters");

            //Hook up escape key handler
            RhinoApp.EscapeKeyPressed += new EventHandler(OnEscEvent);
            Global.readyToSimulate = true;
            Global.Timer = Timer;
            dragger = new Dragger();
            dragger.Enabled = true;
            clothDragger = new ClothDragger();
            clothDragger.Enabled = true;

            //lock objects
            foreach (var a in Global.actors)
                this.doc.Objects.Lock(a, true);

            return Result.Success;
        }



        private void OnEscEvent(object source, EventArgs e)
        {
            if (Timer.IsEnabled)
                Timer.IsEnabled = false;
            else
                Timer.IsEnabled = true;

            //lock or unlock objects
            foreach (var a in Global.actors)
            {
                if (Timer.IsEnabled)
                    this.doc.Objects.Lock(a, true);
                else
                    this.doc.Objects.Unlock(a, true);
            }

            this.doc.Views.Redraw();
        }

        private void MainLoop(object source, EventArgs arg)
        {
            //if any simulation object is selected, pause
            frameTimer.Start();
            //export results and update objects in Rhino
            for(int i = 0; i < Global.actors.Count; i++)
            {
                var id = Global.actors[i];

                //check the draggers
                if (dragger.doDrag)
                    dragger.Update();
                if (clothDragger.doDrag)
                    clothDragger.Update();

                var a = doc.Objects.Find(id) as ActorBase;

                if (a == null) //TODO: workaround until Rhino fixes the CommitChanges bug.
                {
                    var mObj = doc.Objects.Find(id) as Rhino.DocObjects.MeshObject;
                    if (mObj.IsLocked)
                        doc.Objects.Unlock(id, true);
                    var oldMesh = mObj.MeshGeometry;
                    EngineControl.ReturnActorResults(id, ref oldMesh);
                    mObj.CommitChanges();
                    doc.Objects.Lock(id, true);
                }
                else if (a.actorType == ActorType.Rigid)
                {
                    ((RigidBodyActor)a).Update();
                    if (!((RigidBodyActor)a).kinematic)
                        doc.Objects.Transform(((RigidBodyActor)a).Id, ((RigidBodyActor)a).stepPose, true);
                }
                else if (a.actorType == ActorType.Cloth)
                {
                    if (a.IsLocked)
                        doc.Objects.Unlock(id, true);
                    ((ClothActor)a).Update();
                    doc.Objects.Lock(id, true);
                }
            }

            //step the simulation
            EngineControl.StepPhysics((float)Global.timeStep, Global.substeps);

            doc.Views.Redraw();

            //log times
            frameTimer.Stop();
            if (frameTimer.Elapsed.TotalMilliseconds > maxFrame)
                maxFrame = frameTimer.Elapsed.TotalMilliseconds;
            maxFrame -= (int)(0.01 * maxFrame);
            maxFrame = Math.Max(0, maxFrame);
            Global.dataDisplay.hudMessages[0] = "Frame: " + frameTimer.Elapsed.TotalMilliseconds;
            Global.dataDisplay.hudMessages[1] = "Max Frame: " + maxFrame;
            frameTimer.Reset();
        }
    }
}
