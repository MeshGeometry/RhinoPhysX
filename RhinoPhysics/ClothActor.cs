using System;
using System.Collections.Generic;
using Rhino;
using Rhino.Commands;
using Rhino.Geometry;
using Rhino.DocObjects;
using Rhino.Display;
using PhysXEngineWrapper;

namespace RhinoPhysics
{
    public class ClothActor : ActorBase
    {
        public Mesh vizMesh;
        public DisplayMaterial mat;
        public Transform orgPose;

        public ClothActor()
        {
        }

        public ClothActor(RhinoObject obj)
        {
            var objMesh = obj.GetMeshes(MeshType.Default);
            var fullMesh = new Mesh();
            for (int i = 0; i < objMesh.Length; i++)
                fullMesh.Append(objMesh[i]);

            //fullMesh.Faces.ConvertQuadsToTriangles();
            fullMesh.Weld(3.0);

            var bbox = fullMesh.GetBoundingBox(true);
            vizMesh = fullMesh;

            this.SetMesh(fullMesh);
            this.mat = new DisplayMaterial(System.Drawing.Color.CadetBlue, 0.9);
            //this.orgPose = Transform.Translation(new Vector3d(bbox.Center));
            this.orgPose = Transform.Identity;
        
            base.actorType = ActorType.Cloth;
        }

        public void AddPhysicsObject(double stiffness)
        {
            //EngineControl.AddCloth(this.Id, vizMesh, vizMesh.GetBoundingBox(false).Center, stiffness);
            EngineControl.AddCloth(this.Id, vizMesh, Point3d.Origin, stiffness);
            
        }

        public override void Update()
        {
            EngineControl.ReturnActorResults(this.Id, ref vizMesh);
            vizMesh.Transform(orgPose);
            var oldMesh = this.MeshGeometry;
            for (int i = 0; i < oldMesh.Vertices.Count; i++)
                oldMesh.Vertices[i] += new Vector3f(0, 0, 0.1f);
            this.SetMesh(oldMesh);
            var success = this.CommitChanges();
        }
        protected override void OnDraw(DrawEventArgs e)
        {
            e.Display.DrawMeshShaded(vizMesh, this.mat);
            e.Display.DrawMeshWires(vizMesh, this.mat.Diffuse);
            base.OnDraw(e);
        }

        protected override void OnDuplicate(RhinoObject source)
        {
            var obj = (ClothActor)source;
            this.vizMesh = obj.vizMesh;
            this.mat = obj.mat;
            base.OnDuplicate(source);
        }

        protected override void OnSelectionChanged()
        {
            base.OnSelectionChanged();
        }

        protected override void OnTransform(Transform transform)
        {
            base.OnTransform(transform);
        }
    }
}
