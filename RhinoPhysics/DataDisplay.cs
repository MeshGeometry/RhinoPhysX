﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Rhino;
using Rhino.Display;
using Rhino.Geometry;
namespace RhinoPhysics
{
    public class DataDisplay : DisplayConduit
    {
        public string[] hudMessages = new string[4] { "", "", "", "" };

        private int leftOffset = 200;
        private int textHeight = 16;
        private int lineDiff = 8;
        private int bottomOffset = 15;

        public Mesh container;

        protected override void DrawForeground(DrawEventArgs e)
        {
            var bounds = e.Viewport.Bounds;

            for (int i = 0; i < hudMessages.Length; i++)
            {
                var tmpPt = new Rhino.Geometry.Point2d(bounds.Right - leftOffset, bounds.Bottom - (i * (textHeight + lineDiff) + bottomOffset));
                e.Display.Draw2dText(hudMessages[i], System.Drawing.Color.Black, tmpPt, false, textHeight);
            }
        }

        protected override void PostDrawObjects(DrawEventArgs e)
        {
            base.PostDrawObjects(e);

            var mat = new DisplayMaterial(System.Drawing.Color.LawnGreen, 0.5);

            if (container != null)
                e.Display.DrawMeshShaded(container, mat);
        }
    }
}
