using System;
using System.Windows.Threading;
using Rhino;
using Rhino.Commands;
using Rhino.Geometry;
using PhysXEngineWrapper;

namespace RhinoPhysics
{
    [System.Runtime.InteropServices.Guid("f9d96266-46fb-4a82-837c-314c5a6feb33")]
    public class rpTest : Command
    {
        static rpTest _instance;
        public DispatcherTimer Timer;
        public cMesh testMesh;
        public Mesh orgMesh;
        public RhinoDoc doc;
        public Guid currId;
        public rpTest()
        {
            _instance = this;
        }

        ///<summary>The only instance of the rpTest command.</summary>
        public static rpTest Instance
        {
            get { return _instance; }
        }

        public override string EnglishName
        {
            get { return "rpTest"; }
        }

        protected override Result RunCommand(RhinoDoc doc, RunMode mode)
        {
            this.doc = doc;

            var go = new Rhino.Input.Custom.GetObject();
            //go.GeometryFilter = Rhino.DocObjects.ObjectType.Mesh;
            go.SetCommandPrompt("Select Objects for Test:");

            while (true)
            {
                // perform the get operation. This will prompt the user to input a point, but also
                // allow for command line options defined above
                Rhino.Input.GetResult get_rc = go.GetMultiple(1, 0);

                if (go.CommandResult() != Rhino.Commands.Result.Success)
                    return go.CommandResult();

                break;
            }

            Brep brep = go.Object(0).Brep();

            //var orgMesh = new Mesh();
            //orgMesh.CopyFrom(go.Object(0).Mesh());
            //testMesh = new cMesh(go.Object(0).Mesh());
            testMesh = new cMesh(brep);

            doc.Objects.Delete(go.Object(0).Object(), true);

            testMesh.doc = doc;

            doc.Objects.AddRhinoObject(testMesh);
            testMesh.xform = Transform.Identity;
            testMesh.CreateMeshes(MeshType.Default, MeshingParameters.Default, false);
            //currId = doc.Objects.Transform(testMesh, Transform.Identity, true);

            doc.Views.Redraw();

            // Hook up the Elapsed event for the timer.
            Timer = new DispatcherTimer();
            Timer.Interval = TimeSpan.FromMilliseconds(16);
            Timer.Tick += new EventHandler(MainLoop);
            Timer.Start();
            Timer.IsEnabled = true;

            return Result.Success;
        }

        void MainLoop(Object source, EventArgs e)
        {
            //get results from engine
            var xform = Transform.Translation(new Vector3d(0, 0, -1));

            //update all objects
            //var guid = doc.Objects.Transform(testMesh.Id, xform, true);
            testMesh.xform = xform;
            testMesh.Update();

            //redraw
            doc.Views.Redraw();
        }
    }

    public class cMesh : Rhino.DocObjects.Custom.CustomMeshObject
    {
        public RhinoDoc doc;
        public Transform xform;
        public GeometryBase attachedGeo;

        public cMesh()
        {

        }

        public cMesh(Rhino.Geometry.Mesh mesh)
        {
            this.SetMesh(mesh);
            //create rigidbody/cloth/
        }

        public cMesh(Rhino.Geometry.Brep brep)
        {
            Mesh tmpMesh = new Mesh();
            if (brep != null)           
                tmpMesh = Mesh.CreateFromBox(new Box(brep.GetBoundingBox(true)), 1, 1, 1);
            
            this.SetMesh(tmpMesh);
            this.attachedGeo = brep.DuplicateBrep();
        }

        public void Update()
        {
            doc.Objects.Transform(this.Id, xform, true);
            attachedGeo.Transform(xform);
        }

        protected override void OnDraw(Rhino.Display.DrawEventArgs e)
        {
            var geo = this.attachedGeo;

            base.OnDraw(e);
        }

        protected override void OnAddToDocument(RhinoDoc doc)
        {
            base.OnAddToDocument(doc);
        }

        protected override void OnTransform(Transform transform)
        {
            base.OnTransform(transform);
            //update actor transform
        }

        protected override void OnPicked(Rhino.Input.Custom.PickContext context, System.Collections.Generic.IEnumerable<Rhino.DocObjects.ObjRef> pickedItems)
        {
            base.OnPicked(context, pickedItems);
            //freeze simulation
        }

        protected override System.Collections.Generic.IEnumerable<Rhino.DocObjects.ObjRef> OnPick(Rhino.Input.Custom.PickContext context)
        {
            return base.OnPick(context);
        }

        protected override void OnDeleteFromDocument(RhinoDoc doc)
        {
            //remove from engine
            base.OnDeleteFromDocument(doc);

        }
    }
}
