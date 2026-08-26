using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Nova3DMapEditor
{
    public class Node
    {
        public List<Wall> Walls = new List<Wall>();
        public int Floor { get; set; }
        public int Ceiling { get; set; }

        public bool IsVisible = true;
    }
}
