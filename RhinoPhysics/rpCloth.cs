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
    [System.Runtime.InteropServices.Guid("9b43a656-e9ed-421a-9f77-8d5f627cb706")]
    public class rpCloth : Command
    {
        static rpCloth _instance;
        public rpCloth()
        {
            _instance = this;
        }

        ///<summary>The only instance of the rpCloth command.</summary>
        public static rpCloth Instance
        {
            get { return _instance; }
        }

        public override string EnglishName
        {
            get { return "rpCloth"; }
        }

        protected override Result RunCommand(RhinoDoc doc, RunMode mode)
        {
            if (!Global.active)
                return Result.Cancel;
            var opStiffness = new OptionDouble(0.7, 0.001, 1);
            var go = new Rhino.Input.Custom.GetObject();
            go.SetCommandPrompt("Select Objects for Cloth Simulation:");
            go.GeometryFilter = Rhino.DocObjects.ObjectType.Mesh | Rhino.DocObjects.ObjectType.Brep;
            go.AddOptionDouble("FabricStiffness", ref opStiffness);

            while (true)
            {
                // perform the get operation. This will prompt the user to input a point, but also
                // allow for command line options defined above
                Rhino.Input.GetResult get_rc = go.GetMultiple(1, 0);

                if (go.CommandResult() != Rhino.Commands.Result.Success)
                    return go.CommandResult();

                if (get_rc == Rhino.Input.GetResult.Option)
                    continue;

                break;
            }

            for (int i = 0; i < go.ObjectCount; i++)
            {

                var tmpCloth = new ClothActor(go.Object(i).Object());
                doc.Objects.AddRhinoObject(tmpCloth);
                tmpCloth.AddPhysicsObject(opStiffness.CurrentValue);
                Global.actors.Add(tmpCloth.Id);
                doc.Objects.Hide(go.Object(i).ObjectId, true);
            }

            doc.Views.Redraw();

            return Result.Success;
        }
    }
}
