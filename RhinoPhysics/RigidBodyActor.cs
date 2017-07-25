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
    public class RigidBodyActor : ActorBase
    {
        public RhinoDoc doc;
        public Guid attachedGeo;
        public Mesh colliderMesh;
        public bool kinematic = false;
        public bool isStatic = false;
        public Mesh vizMesh;
        public CollisionType collisionType;

        public Transform orgPose;
        public Transform lastPose;
        public Transform newPose;
        public Transform tmpPose;
        public Transform stepPose;
        public DisplayMaterial mat;

        public RigidBodyActor()
        {

        }

        public RigidBodyActor(IEnumerable<Rhino.DocObjects.RhinoObject> rhinoObjList, CollisionType collision)
        {
            var fullMesh = new Mesh();
            foreach (var rhinoObj in rhinoObjList)
            {
                var objMesh = rhinoObj.GetMeshes(MeshType.Default);
                for (int i = 0; i < objMesh.Length; i++)
                    fullMesh.Append(objMesh[i]);
            }

            fullMesh.Weld(3.0);
            vizMesh = fullMesh;

            this.mat = new DisplayMaterial(System.Drawing.Color.CadetBlue, 0.2);
            this.SetMesh(fullMesh);
            //this.attachedGeo = rhinoObj.Id;
            this.lastPose = Transform.Translation(new Vector3d(vizMesh.GetBoundingBox(true).Center));
            this.newPose = lastPose;
            this.orgPose = new Transform();
            this.tmpPose = Transform.Identity;
            this.stepPose = Transform.Identity;
            this.collisionType = collision;
            lastPose.TryGetInverse(out this.orgPose);
            //this.Attributes.MaterialSource = ObjectMaterialSource.MaterialFromObject;
            //this.Attributes.ColorSource = ObjectColorSource.ColorFromObject;
            //this.Attributes.ObjectColor = System.Drawing.Color.OrangeRed;
            //this.Attributes.SetDisplayModeOverride(DisplayModeDescription.FindByName("Shaded"));
            base.actorType = ActorType.Rigid;

        }

        public RigidBodyActor(Rhino.DocObjects.RhinoObject rhinoObj, CollisionType collision)
        {
            var objMesh = rhinoObj.GetMeshes(MeshType.Default);
            var fullMesh = new Mesh();
            for (int i = 0; i < objMesh.Length; i++)
                fullMesh.Append(objMesh[i]);
            vizMesh = fullMesh;

            this.mat = new DisplayMaterial(System.Drawing.Color.CadetBlue, 0.2);
            this.SetMesh(fullMesh);
            this.attachedGeo = rhinoObj.Id;
            this.lastPose = Transform.Translation(new Vector3d(vizMesh.GetBoundingBox(true).Center));
            this.newPose = lastPose;
            this.orgPose = new Transform();
            this.tmpPose = Transform.Identity;
            this.stepPose = Transform.Identity;
            this.collisionType = collision;
            lastPose.TryGetInverse(out this.orgPose);
            //this.Attributes.MaterialSource = ObjectMaterialSource.MaterialFromObject;
            //this.Attributes.ColorSource = ObjectColorSource.ColorFromObject;
            //this.Attributes.ObjectColor = System.Drawing.Color.OrangeRed;
            //this.Attributes.SetDisplayModeOverride(DisplayModeDescription.FindByName("Shaded"));
            base.actorType = ActorType.Rigid;

        }

        public void AddPhysicsObject(Rhino.DocObjects.RhinoObject rhinoObj, bool isStatic)
        {
            if (isStatic)
            {
                EngineControl.AddRigidStatic(this.Id, vizMesh, rhinoObj.Geometry.GetBoundingBox(true).Center);
                colliderMesh = vizMesh;
                this.isStatic = isStatic;
                return;
            }
            
            //submit mesh and guid to engine
            switch (collisionType)
            {
                case CollisionType.Compound:
                    EngineControl.AddCompoundConvex(this.Id, vizMesh, rhinoObj.Geometry.GetBoundingBox(true).Center);
                    colliderMesh = EngineControl.ExportCollisionMesh(this.Id);
                    break;
                case CollisionType.Box:
                    EngineControl.AddBox(this.Id, rhinoObj.Geometry.GetBoundingBox(true));
                    colliderMesh = Mesh.CreateFromBox(rhinoObj.Geometry.GetBoundingBox(true), 1, 1, 1);
                    break;
                case CollisionType.Convex:
                    EngineControl.AddConvexRigidDynamic(this.Id, vizMesh, rhinoObj.Geometry.GetBoundingBox(true).Center, true);
                    colliderMesh = EngineControl.ExportCollisionMesh(this.Id);
                    break;
                case CollisionType.Sphere:
                    var bbox = rhinoObj.Geometry.GetBoundingBox(true);
                    var sphere = new Sphere(bbox.Center, bbox.Diagonal.Length * 0.5);
                    EngineControl.AddSphere(this.Id, sphere.Center, sphere.Radius);
                    colliderMesh = Mesh.CreateFromSphere(sphere, 64, 128);
                    break;
                case CollisionType.Null:
                    EngineControl.AddEmpty(this.Id, vizMesh, rhinoObj.Geometry.GetBoundingBox(true).Center);
                    colliderMesh = EngineControl.ExportCollisionMesh(this.Id);
                    break;
            }
        }

        public void CreateMultiBody()
        {
            var meshList = vizMesh.SplitDisjointPieces();
            EngineControl.AddMultiShapeRigidDynamic(this.Id, meshList, true);
            colliderMesh = EngineControl.ExportCollisionMesh(this.Id);
        }

        public void AddConstraint(int flags, Point3d org)
        {
            EngineControl.LockDOF(this.Id, org, flags);
        }

        public override void Update()
        {
            EngineControl.ReturnActorResults(this.Id, ref this.newPose);
            Transform inverse = new Transform();
            this.lastPose.TryGetInverse(out inverse);
            this.stepPose = Transform.Multiply(this.newPose, inverse);

            if (this.kinematic)
            {
                Transform dPose = Transform.Identity;
                if (this.HasDynamicTransform)
                {
                    this.GetDynamicTransform(out dPose);
                    var delPose = Transform.Multiply(dPose, this.tmpPose);
                    EngineControl.IncrementKinematicPose(this.Id, delPose);
                    //EngineControl.StepPhysics((float)Global.timeStep, Global.substeps);
                    dPose.TryGetInverse(out this.tmpPose);
                }
                else
                {
                    this.tmpPose = Transform.Identity;
                }
            }
            //let the document handle transform the object if not kinematic
            else
            {
                this.lastPose = this.newPose;
                //doc.Objects.Transform(this.Id, xform, true);
            }
        }

        protected override void OnDraw(Rhino.Display.DrawEventArgs e)
        {
            //e.Display.PushModelTransform(Transform.Multiply(this.newPose, this.orgPose));
            if (colliderMesh != null)
                //e.Display.DrawMeshShaded(this.colliderMesh, mat);
                e.Display.DrawMeshWires(this.colliderMesh, mat.Diffuse);
            //e.Display.PopModelTransform();
        }

        protected override void OnAddToDocument(RhinoDoc doc)
        {
            base.OnAddToDocument(doc);
        }

        protected override void OnDuplicate(Rhino.DocObjects.RhinoObject source)
        {
            var obj = (RigidBodyActor)source;
            this.attachedGeo = obj.attachedGeo;
            this.doc = obj.doc;
            this.lastPose = obj.lastPose;
            this.orgPose = obj.orgPose;
            this.newPose = obj.newPose;
            this.tmpPose = obj.tmpPose;
            this.colliderMesh = obj.colliderMesh;
            this.kinematic = obj.kinematic;
            this.vizMesh = obj.vizMesh;
            this.Attributes = obj.Attributes;
            this.collisionType = obj.collisionType;
            this.mat = obj.mat;
            this.stepPose = obj.stepPose;
            this.isStatic = obj.isStatic;
            base.OnDuplicate(source);
        }

        protected override void OnTransform(Transform transform)
        {
            base.OnTransform(transform);
            colliderMesh.Transform(transform);

            if (this.kinematic)
            {
                EngineControl.IncrementKinematicPose(this.Id, transform);
                EngineControl.StepPhysics((float)Global.timeStep, Global.substeps);
            }
        }

        protected override void OnPicked(Rhino.Input.Custom.PickContext context, System.Collections.Generic.IEnumerable<Rhino.DocObjects.ObjRef> pickedItems)
        {
            base.OnPicked(context, pickedItems);
        }

        protected override void OnSelectionChanged()
        {
            base.OnSelectionChanged();

            if (this.IsSelected(false) > 0 && !this.kinematic)
            {
                EngineControl.SetKinematic(this.Id, true);
                this.kinematic = true; ;
            }
            else if (this.IsSelected(false) <= 0 && this.kinematic)
            {
                EngineControl.SetKinematic(this.Id, false);
                EngineControl.ReturnActorResults(this.Id, ref this.lastPose);
                this.kinematic = false;
            }
        }

        protected override System.Collections.Generic.IEnumerable<Rhino.DocObjects.ObjRef> OnPick(Rhino.Input.Custom.PickContext context)
        {
            return base.OnPick(context);
        }

    }
}
