using System;
using System.Collections.Generic;
using System.Windows.Threading;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Rhino.Geometry;
using Rhino.Input;
using Rhino.DocObjects;
using PhysXEngineWrapper;

namespace RhinoPhysics
{

    public enum CollisionType
    {
        Null,
        Box,
        Sphere,
        Capsule,
        Convex,
        Compound
    }
    //some global properties and classes
    public static class Global
    {
        public static bool active = false;
        public static double timeStep = 0.1;
        public static int substeps = 2;
        public static DispatcherTimer Timer;
        public static DispatcherTimer UIMonitor;
        public static Rhino.RhinoDoc rDoc;
        public static bool readyToSimulate = false;
        public static DataDisplay dataDisplay;

        public static List<Guid> actors;
        public static List<Guid> joints;
        public static Rhino.RhinoDoc doc;

        public static void UILoop(object source, EventArgs arg)
        {
            for (int i = 0; i < actors.Count; i++)
            {
                //try to find the doc object
                var obj = rDoc.Objects.Find(actors[i]);

                if (obj == null)
                {
                    EngineControl.RemoveActor(actors[i]);
                    actors.RemoveAt(i);
                    break;
                }
            }
        }
    }
}
