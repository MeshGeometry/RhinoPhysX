using System;
using System.Collections.Generic;
using System.Windows.Threading;
using Rhino;
using Rhino.Commands;
using Rhino.Geometry;
using Rhino.Input;
using Rhino.Input.Custom;
using Rhino.DocObjects;

using PhysXEngineWrapper;

namespace RhinoPhysics
{
    [System.Runtime.InteropServices.Guid("a7562ea5-080b-42c2-a63b-ef45b3952d63")]
    public class rpRigidDynamic : Command
    {
        static rpRigidDynamic _instance;
        public rpRigidDynamic()
        {
            _instance = this;
        }

        ///<summary>The only instance of the rpRigidDynamic command.</summary>
        public static rpRigidDynamic Instance
        {
            get { return _instance; }
        }

        public override string EnglishName
        {
            get { return "rpRigidDynamic"; }
        }

        protected override Result RunCommand(RhinoDoc doc, RunMode mode)
        {
            if (!Global.active)
                return Result.Cancel;

            var go = new Rhino.Input.Custom.GetObject();
            go.SetCommandPrompt("Select Objects for RigidBody Simulation:");

            string[] listValues = new string[] { "Box", "Sphere", "Convex", "Compound", "Null"};
            var combineToggle = new OptionToggle(false, "Off", "On");
            var mass = new OptionDouble(1.0, true, 0.1);

            int listIndex = 0;
            int opList = go.AddOptionList("ColliderOptions", listValues, listIndex);
            int combine = go.AddOptionToggle("CombineShapes", ref combineToggle);
            go.AddOptionDouble("Mass", ref mass);
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
                    collisionType = CollisionType.Convex;
                    break;
                case "Null":
                    collisionType = CollisionType.Null;
                    break;
                default:
                    collisionType = CollisionType.Box;
                    break;
            }

            if (!combineToggle.CurrentValue)
            {
                for (int i = 0; i < go.ObjectCount; i++)
                {
                    var tmpRBody = new RigidBodyActor(go.Object(i).Object(), collisionType);
                    doc.Objects.AddRhinoObject(tmpRBody);
                    tmpRBody.AddPhysicsObject(go.Object(i).Object(), false);
                    Global.actors.Add(tmpRBody.Id);

                    doc.Objects.Hide(go.Object(i).ObjectId, true);
                }
            }
            else
            {
                var objList = new List<RhinoObject>();
                for (int i = 0; i < go.ObjectCount; i++)
                {
                    objList.Add(go.Object(i).Object());
                    doc.Objects.Hide(go.Object(i).ObjectId, true);
                }
                var multiBody = new RigidBodyActor(objList, CollisionType.Convex);
                doc.Objects.AddRhinoObject(multiBody);
                multiBody.CreateMultiBody();
                Global.actors.Add(multiBody.Id);
            }

            doc.Views.Redraw();

            return Result.Success;
        }
    }
}
