using System;
using System.Collections.Generic;
using Rhino;
using Rhino.Commands;
using Rhino.Display;
using Rhino.DocObjects;
using Rhino.Geometry;
using Rhino.Geometry.Intersect;
using Rhino.Input;
using Rhino.Input.Custom;
using Rhino.UI;

namespace RhinoPhysics
{
    public class JointLine : DisplayConduit
    {

        public JointLine()
        {

        }

        protected override void CalculateBoundingBox(CalculateBoundingBoxEventArgs e)
        {
            base.CalculateBoundingBox(e);

            var box = new BoundingBox(new Point3d(-1000.0, -1000.0, -1000.0), new Point3d(1000.0, 1000.0, 1000.0));
            e.IncludeBoundingBox(box);
        }

        protected override void PostDrawObjects(DrawEventArgs e)
        {
            base.PostDrawObjects(e);

            foreach (var j in Global.joints)
            {
                e.Display.DrawLine(j.from.currCenter, j.to.currCenter, System.Drawing.Color.Cyan, 2);
            }
        }
    }
}
