using System;
using System.Collections.Generic;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Nova3DMapEditor
{
    public class Portal : Wall
    {
        public int NodeIndex { get; set; }

        public Portal(int x1, int y1, int x2, int y2, int nodeIndex)
            : base(x1, y1, x2, y2)
        {
            this.NodeIndex = nodeIndex;
            this.Colour = Color.Red;
        }

        public Portal(Wall wall, int nodeIndex)
            : this(wall.Point1.X, wall.Point1.Y, wall.Point2.X, wall.Point2.Y, nodeIndex)
        {
        }

        public override string ToString()
        {
            return base.ToString() + $" NODE: {this.NodeIndex}";
        }
    }
}
