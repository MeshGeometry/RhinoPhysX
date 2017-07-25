using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

using Rhino;
using Rhino.Commands;
using Rhino.Display;
using Rhino.DocObjects;
using Rhino.Geometry;
using Rhino.Geometry.Intersect;
using Rhino.Input;
using Rhino.Input.Custom;
using Rhino.UI;

using PhysXEngineWrapper;

namespace RhinoPhysics
{
    public struct HitInfo
    {
        public Guid guid;
        public double distance;
        public Point3d hitPoint;

    }
    
    public static class Raycast
    {
        static RhinoDoc rDoc;

        [System.Runtime.InteropServices.DllImport("user32.dll")]
        public static extern bool GetCursorPos(out System.Drawing.Point pt);

        public static void CastRay(Ray3d ray, out HitInfo hitInfo)
        {
            
            
            
            hitInfo = new HitInfo();
        }

        public static Point3d ScreenToWorldPoint(System.Drawing.Point point, double depth, RhinoView view)
        {
            // Convert the screen coordinates to client coordinates
            point = view.ActiveViewport.ScreenToClient(point);

            // Obtain the view's screen-to-world transformation
            Rhino.Geometry.Transform screen_to_world = view.ActiveViewport.GetTransform(
              Rhino.DocObjects.CoordinateSystem.Screen,
              Rhino.DocObjects.CoordinateSystem.World);

            // Create a 3-D point
            Point3d pt = new Point3d(point.X, point.Y, 0.0);
            Point3d pt1 = new Point3d(point.X, point.Y, 1.0);

            // Transform it
            pt.Transform(screen_to_world);
            pt1.Transform(screen_to_world);

            var dir = pt1 - pt;
            dir.Unitize();

            return pt + depth * dir;
        }

        public static Vector3d ScreenPointToVector(System.Drawing.Point point, RhinoView view)
        {
            // Convert the screen coordinates to client coordinates
            point = view.ActiveViewport.ScreenToClient(point);

            // Obtain the view's screen-to-world transformation
            Rhino.Geometry.Transform screen_to_world = view.ActiveViewport.GetTransform(
              Rhino.DocObjects.CoordinateSystem.Screen,
              Rhino.DocObjects.CoordinateSystem.World
              );

            // Create a 3-D point
            Rhino.Geometry.Point3d pt = new Rhino.Geometry.Point3d(point.X, point.Y, 0.0);
            Rhino.Geometry.Point3d pt1 = new Rhino.Geometry.Point3d(point.X, point.Y, 1.0);

            // Transform it
            pt.Transform(screen_to_world);
            pt1.Transform(screen_to_world);

            var dir = pt1 - pt;
            dir.Unitize();

            return dir;
        }

    }
}
