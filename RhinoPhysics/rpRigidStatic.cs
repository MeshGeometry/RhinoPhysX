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
    [System.Runtime.InteropServices.Guid("43b9a362-e04a-4dcf-b516-a0150d34b966")]
    public class rpRigidStatic : Command
    {
        static rpRigidStatic _instance;
        public rpRigidStatic()
        {
            _instance = this;
        }

        ///<summary>The only instance of the rpRigidStatic command.</summary>
        public static rpRigidStatic Instance
        {
            get { return _instance; }
        }

        public override string EnglishName
        {
            get { return "rpRigidStatic"; }
        }

        protected override Result RunCommand(RhinoDoc doc, RunMode mode)
        {
            if (!Global.active)
                return Result.Cancel;

            var go = new Rhino.Input.Custom.GetObject();
            go.GeometryFilter = Rhino.DocObjects.ObjectType.Mesh | Rhino.DocObjects.ObjectType.Brep;
            go.SetCommandPrompt("Select Objects to add as Rigid Static:");

            string[] listValues = new string[] { "Box", "Sphere", "Convex", "Triangle" };

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

            var collisionType = CollisionType.Box;
            switch (listValues[listIndex])
            {
                case "Box":
                    collisionType = CollisionType.Box;
                    break;
                case "Sphere":
                    collisionType = CollisionType.Sphere;
                    break;
                case "Convex":
                    collisionType = CollisionType.Convex;
                    break;
                case "Compound":
                    collisionType = CollisionType.Compound;
                    break;
                case "Null":
                    collisionType = CollisionType.Null;
                    break;
                default:
                    collisionType = CollisionType.Box;
                    break;
            }

            for (int i = 0; i < go.ObjectCount; i++)
            {
                Mesh m = go.Object(i).Mesh();

                //always initialize a new game object with the scene object id.
                var tmpRBody = new RigidBodyActor(go.Object(i).Object(), collisionType);
                doc.Objects.AddRhinoObject(tmpRBody);
                tmpRBody.AddPhysicsObject(go.Object(i).Object(), true);
                Global.actors.Add(tmpRBody.Id);
                doc.Objects.Hide(go.Object(i).ObjectId, true);
            }

            return Result.Success;
        }
    }
}
