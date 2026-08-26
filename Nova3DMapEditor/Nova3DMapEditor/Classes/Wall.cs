using System;
using System.Collections.Generic;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Nova3DMapEditor
{
    public class Wall
    {
        public Point Point1 { get; set; }
        public Point Point2 { get; set; }

        public Color Colour;

        public Wall(int x1, int y1, int x2, int y2)
        {            
            this.Point1 = new Point(x1, y1);
            this.Point2 = new Point(x2, y2);
            this.Colour = Color.FromArgb(Form1.Random.Next(255), Form1.Random.Next(255), Form1.Random.Next(255));
        }

        public Wall(Portal portal)
        {
            this.Point1 = portal.Point1;
            this.Point2 = portal.Point2;
            this.Colour = Color.FromArgb(Form1.Random.Next(255), Form1.Random.Next(255), Form1.Random.Next(255));
        }

        public Wall(Wall copy)
        {
            this.Point1 = copy.Point1;
            this.Point2 = copy.Point2;
            this.Colour = copy.Colour;
        }

        public override string ToString()
        {
            return $"[{this.Point1.X}, {this.Point1.Y}], [{this.Point2.X}, {this.Point2.Y}]";
        }
    }
}
