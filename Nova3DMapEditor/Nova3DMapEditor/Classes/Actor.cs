using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Nova3DMapEditor
{
    public class Actor
    {
        public int X;
        public int Y;
        public int Z;
        public int ActorTypeIndex;
        public int Angle;

        public Actor(int x, int y, int actorTypeIndex, int angle)
        {
            this.X = x;
            this.Y = y;
            this.ActorTypeIndex = actorTypeIndex;
            this.Angle = angle;
        }

        public override string ToString()
        {
            return $"[{this.X}, {this.Y}] Index: {this.ActorTypeIndex} A: {this.Angle}";
        }
    }
}
