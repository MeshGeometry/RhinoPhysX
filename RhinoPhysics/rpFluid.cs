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
    [System.Runtime.InteropServices.Guid("146192fd-eab7-407c-979f-7d1b2fec1fe1")]
    public class rpFluid : Command
    {
        static rpFluid _instance;
        public rpFluid()
        {
            _instance = this;
        }

        ///<summary>The only instance of the rpFluid command.</summary>
        public static rpFluid Instance
        {
            get { return _instance; }
        }

        public override string EnglishName
        {
            get { return "rpFluid"; }
        }

        protected override Result RunCommand(RhinoDoc doc, RunMode mode)
        {
            if (!Global.active)
                return Result.Cancel;

            var opStiffness = new OptionDouble(1, 0, 10000);
            var opViscosity = new OptionDouble(1, 0, 10000);
            var opRate = new OptionInteger(0, 0, 1000);
            var opDensity = new OptionDouble(0.9, 0.001, 0.999);

            var go = new Rhino.Input.Custom.GetObject();
            go.GeometryFilter = Rhino.DocObjects.ObjectType.Mesh;
            go.SetCommandPrompt("Select Objects to Emmit Fluid Particles:");

            go.AddOptionDouble("Stiffness", ref opStiffness);
            go.AddOptionDouble("Viscosity", ref opViscosity);
            go.AddOptionInteger("EmitRate", ref opRate);
            go.AddOptionDouble("Density", ref opDensity);

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

            for (int i = 0; i < go.ObjectCount; i++)
            {
                Mesh m = go.Object(i).Mesh();

                //always initialize a new game object with the scene object id.
                var tmpGo = new GameObject(go.Object(i).ObjectId, m, Transform.Identity, ActorType.FluidEmmiter);
                //Create a new id for managing the game object <-> PhysX object lookup
                //since sometimes a PhysX object is created from same Rhino object.
                var id = Guid.NewGuid();

                //create some particles
                var rLength = 0.1;
                var particles = FillVolume(m, opDensity.CurrentValue, out rLength);

                if (particles.Count == 0)
                {
                    RhinoApp.WriteLine("No particles have been created. Your mesh is probably not closed.");
                    return Result.Failure;
                }

                var maxParticles = particles.Count;

                if (opRate.CurrentValue > 0)
                    maxParticles = 1000000;


                EngineControl.AddFluid(id, particles.ToArray(), maxParticles, 
                    0.1, opViscosity.CurrentValue, opStiffness.CurrentValue, 1, rLength, opRate.CurrentValue);

                tmpGo.physRef = id;
                Global.allActors.Add(id, tmpGo);
                //doc.Objects.Lock(go.Object(i).ObjectId, true);
            }

            return Result.Success;
        }

        List<Point3d> FillVolume(Mesh m, double density, out double restLength)
        {
            var l = m.GetBoundingBox(true).Diagonal.Length * (1.0 - Math.Pow(density, 0.33));
            restLength = l * Math.Pow(3.0, 0.5) * 0.5;
            return FillVolume(m, l);
        }

        List<Point3d> FillVolume(Mesh m, double restLength)
        {
            var ptsOut = new List<Point3d>();
            var bBox = m.GetBoundingBox(true);
            var div = (int)Math.Ceiling(bBox.Diagonal.Length / restLength);
            var offset = bBox.Diagonal.Length / div;
            var org = bBox.Min;

            for (int i = 0; i < div; i++)
            {
                for (int j = 0; j < div; j++)
                {
                    for (int k = 0; k < div; k++)
                    {
                        var tmpPt = org + new Vector3d(i * offset, j * offset, k * offset);

                        if (m.IsPointInside(tmpPt, 0.01, false))
                            ptsOut.Add(tmpPt);
                    }
                }
            }

            return ptsOut;
        }
    }
}
