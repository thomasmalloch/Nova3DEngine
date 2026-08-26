using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace Nova3DMapEditor
{
    public partial class Form1 : Form
    {
        [DllImport("user32.dll")]
        public static extern int SendMessage(IntPtr hWnd, Int32 wMsg, bool wParam, Int32 lParam);
        private const int WM_SETREDRAW = 11;

        public static Random Random = new Random();
        private Pen Pen = new Pen(Color.Gray);
        private List<Node> Nodes = new List<Node>();
        private List<Actor> Actors = new List<Actor>();
        private const int Multiplier = 2;
        private const int Offset = 0;
        private const int Step = 10;

        private Point MousePoint = new Point(-100,-100);
        private Point LastPoint = new Point(-100, -100);

        enum FileParseModeEnum
        {
            NULL,
            Portal,
            Wall,
            Player,
            Node,
            Actor,
            Map
        }

        public Form1()
        {
            this.InitializeComponent();            
        }

        private void Form1_Load(object sender, EventArgs e)
        {
            Node node = new Node();
            node.Floor = 0;
            node.Ceiling = 32;
            this.Nodes.Add(node);
            this.cboNodes.Items.Add(0);
            this.cboNodes.SelectedIndex = 0;
            this.CboNodes_SelectionChangeCommitted(null, null);

            new Thread(new ThreadStart(() => 
            {
                try
                {
                    while (!this.IsDisposed)
                    {
                        this.pnlMap.Invalidate();
                        Thread.Sleep(50);
                    }
                }
                catch
                {
                }
            })).Start();
        }

        protected override void Dispose(bool disposing)
        {
            if (disposing)
            {
                this.components?.Dispose();
                this.Pen.Dispose();
            }

            base.Dispose(disposing);
        }

        private void PnlMap_Paint(object sender, PaintEventArgs e)
        {
            Graphics g = e.Graphics;
            this.Pen.Color = Color.FromArgb(0x20, 0x20, 0x20);
            this.Pen.Width = 1;

            g.Clear(Color.Black);
            for (int x = 0; x < this.pnlMap.Width; x += (Step * Multiplier))
                g.DrawLine(this.Pen, x + Offset, Offset, x + Offset, this.Height);
            
            for (int y = 0; y < this.pnlMap.Height; y += (Step * Multiplier))
                g.DrawLine(this.Pen, Offset, y + Offset, this.Width, y + Offset);            

            this.Pen.Width = 1;
            Wall selected = null;
            if (this.lvWalls.SelectedItems.Count > 0)
                selected = this.lvWalls.SelectedItems[0].Tag as Wall;


            foreach (Node node in this.Nodes)
            {
                if (!node.IsVisible)
                    continue;

                if (this.Nodes.IndexOf(node) == this.cboNodes.SelectedIndex)
                    continue;

                foreach (Wall wall in node.Walls)
                {
                    this.Pen.Color = Color.FromArgb(200, wall.Colour);
                    g.DrawLine(this.Pen,
                        wall.Point1.X * Step * Multiplier + Offset,
                        wall.Point1.Y * Step * Multiplier + Offset,
                        wall.Point2.X * Step * Multiplier + Offset,
                        wall.Point2.Y * Step * Multiplier + Offset);
                }

                this.DrawIndexInCenterOfNode(g, node);
            }

            this.Pen.Width = 3;
            foreach (Wall wall in this.Nodes[this.cboNodes.SelectedIndex].Walls)
            {
                bool isSelected = false;
                if (selected != null && selected == wall)
                {
                    this.Pen.Color = Color.FromArgb(Random.Next(255), Random.Next(255), Random.Next(255));
                    isSelected = true;
                }
                else
                {
                    this.Pen.Color = wall.Colour;
                }

                g.DrawLine(this.Pen,
                    wall.Point1.X * Step * Multiplier + Offset,
                    wall.Point1.Y * Step * Multiplier + Offset,
                    wall.Point2.X * Step * Multiplier + Offset,
                    wall.Point2.Y * Step * Multiplier + Offset);

                if (!isSelected)
                    continue;

                this.Pen.Color = Color.White;
                g.DrawString("Point 1", this.Font, this.Pen.Brush, wall.Point1.X * Step * Multiplier + Offset, wall.Point1.Y * Step * Multiplier + Offset);
                g.DrawString("Point 2", this.Font, this.Pen.Brush, wall.Point2.X * Step * Multiplier + Offset, wall.Point2.Y * Step * Multiplier + Offset);
            }

            // player
            this.Pen.Color = Color.Cyan;
            this.Pen.Width = 1;
            int playerX = (int)this.numPlayerX.Value * Multiplier;
            int playerY = (int)this.numPlayerY.Value * Multiplier;
            g.FillRectangle(
                this.Pen.Brush,
                playerX - 4,
                playerY - 4,
                8,
                8);

            g.DrawLine(
                this.Pen,
                playerX,
                playerY,
                playerX + (int)(Math.Cos((double)this.numPlayerAngle.Value * Math.PI / 180d) * 15),
                playerY + (int)(Math.Sin((double)this.numPlayerAngle.Value * Math.PI / 180d) * 15));

            // actors
            this.Pen.Color = Color.BlueViolet;
            this.Pen.Width = 1;
            Actor selectedActor = null;
            if ((this.lvActors.Focused || this.numActorX.Focused || this.numActorY.Focused || this.numActorTypeIndex.Focused) && this.lvActors.SelectedItems.Count > 0)
                selectedActor = this.lvActors.SelectedItems[0].Tag as Actor;

            foreach (Actor actor in this.Actors)
            {
                int actorX = actor.X * Multiplier;
                int actorY = actor.Y * Multiplier;
                if (selectedActor != null && selectedActor == actor)
                    this.Pen.Color = Color.FromArgb(Random.Next(255), Random.Next(255), Random.Next(255));
                else
                    this.Pen.Color = Color.BlueViolet;

                g.FillRectangle(
                    this.Pen.Brush,
                    actorX - 4,
                    actorY - 4,
                    8,
                    8);

                g.DrawLine(
                    this.Pen,
                    actorX,
                    actorY,
                    actorX + (int)(Math.Cos((double)actor.Angle * Math.PI / 180d) * 15),
                    actorY + (int)(Math.Sin((double)actor.Angle * Math.PI / 180d) * 15));
            }

            // mouse
            this.DrawIndexInCenterOfNode(g, this.Nodes[this.cboNodes.SelectedIndex]);
            this.Pen.Color = Color.Yellow;
            this.Pen.Width = 1;

            g.FillRectangle(
                this.Pen.Brush,
                this.MousePoint.X * Step * Multiplier - 3,
                this.MousePoint.Y * Step * Multiplier - 3,
                6,
                6 );

            if (this.LastPoint.X > 0)
            {
                g.FillRectangle(
                    this.Pen.Brush,
                    this.LastPoint.X * Step * Multiplier - 3,
                    this.LastPoint.Y * Step * Multiplier - 3,
                    6,
                    6);

                g.DrawLine(
                    this.Pen,
                    this.LastPoint.X * Step * Multiplier + Offset,
                    this.LastPoint.Y * Step * Multiplier + Offset,
                    this.MousePoint.X * Step * Multiplier + Offset,
                    this.MousePoint.Y * Step * Multiplier + Offset);
            }
        }

        private void DrawIndexInCenterOfNode(Graphics g, Node node)
        {
            int top = int.MaxValue;
            int bottom = int.MinValue;
            int left = int.MaxValue;
            int right = int.MinValue;
            foreach (Wall wall in node.Walls)
            {
                top = Math.Min(top, Math.Min(wall.Point1.Y, wall.Point2.Y));
                bottom = Math.Max(bottom, Math.Max(wall.Point1.Y, wall.Point2.Y));
                left = Math.Min(left, Math.Min(wall.Point1.X, wall.Point2.X));
                right = Math.Max(right, Math.Max(wall.Point1.X, wall.Point2.X));
            }

            int middleX = (left != right) ? left + ((right - left) / 2) : left;
            int middleY = (top != bottom) ? top + ((bottom - top) / 2) : bottom;
            this.Pen.Color = Color.FromArgb(150, 150, 150);
            string s = $"Node: {this.Nodes.IndexOf(node)}";
            SizeF size = g.MeasureString(s, this.Font);
            g.DrawString(s, this.Font, this.Pen.Brush, middleX * Step * Multiplier + Offset - size.Width / 2, middleY * Step * Multiplier + Offset - size.Height / 2);
        }

        private void PnlMap_MouseMove(object sender, MouseEventArgs e)
        {
            int x = (int)Math.Round(e.X / ((double)Step * Multiplier));
            int y = (int)Math.Round(e.Y / ((double)Step * Multiplier));
            this.MousePoint = new Point(x,y);
            this.toolTip1.Show($"{x}, {y}", this.pnlMap, e.Location.X + 20, e.Y);
        }

        private void PnlMap_MouseLeave(object sender, EventArgs e)
        {
            this.toolTip1.Hide(this.pnlMap);
        }

        private void PnlMap_MouseClick(object sender, MouseEventArgs e)
        {
            if (e.Button == MouseButtons.Left)
            {
                if (this.numPlayerX.Focused || this.numPlayerY.Focused)
                {
                    this.numPlayerX.Value = this.MousePoint.X * Step;
                    this.numPlayerY.Value = this.MousePoint.Y * Step;
                    return;
                }

                if (this.numActorX.Focused || this.numActorY.Focused)
                {
                    this.numActorX.Value = this.MousePoint.X * Step;
                    this.numActorY.Value = this.MousePoint.Y * Step;
                    this.BtnAddActor_Click(null, null);
                    return;
                }

                if (this.numX1.Focused || this.numY1.Focused)
                {
                    this.numX1.Value = this.MousePoint.X;
                    this.numY1.Value = this.MousePoint.Y;
                    this.numX2.Focus();
                    this.LastPoint = new Point(this.MousePoint.X, this.MousePoint.Y);
                    return;
                }

                if (this.numX2.Focused || this.numY2.Focused)
                {
                    this.numX2.Value = this.MousePoint.X;
                    this.numY2.Value = this.MousePoint.Y;
                    this.BtnAddLine_Click(null, null);

                    bool found = false;
                    foreach (Wall wall in this.Nodes[this.cboNodes.SelectedIndex].Walls)
                    {
                        if (wall.Point1 == this.MousePoint)
                        {
                            found = true;
                            break;
                        }
                    }

                    if (found)
                    {
                        this.LastPoint = new Point(-1, -1);
                        return;
                    }

                    this.numX1.Value = this.MousePoint.X;
                    this.numY1.Value = this.MousePoint.Y;
                    this.LastPoint = new Point(this.MousePoint.X, this.MousePoint.Y);
                    this.numX2.Focus();
                }
                else
                {
                    this.tabControl1.SelectedTab = this.tabWalls;
                    this.numX1.Focus();
                    this.LastPoint = new Point(-1, -1);
                    this.PnlMap_MouseClick(sender, e);
                    return;
                }



            }

            if (e.Button == MouseButtons.Right)
            {
                this.LastPoint = new Point(-1, -1);
                this.numX1.Focus();
            }
        }

        private void BtnAddNode_Click(object sender, EventArgs e)
        {
            Node node = new Node();
            node.Floor = 0;
            node.Ceiling = 32;
            this.Nodes.Add(node);
            this.cboNodes.Items.Add(this.Nodes.Count - 1);
            this.cboNodes.SelectedIndex = this.Nodes.Count - 1;
            this.CboNodes_SelectionChangeCommitted(null, null);
        }

        private void UpdateWalls()
        {
            SendMessage(this.lvWalls.Handle, WM_SETREDRAW, false, 0);
            this.lvWalls.Items.Clear();
            foreach (Wall wall in this.Nodes[this.cboNodes.SelectedIndex].Walls)
            {
                ListViewItem item = new ListViewItem((wall is Portal portal) ? portal.ToString() : wall.ToString());
                item.Tag = wall;
                this.lvWalls.Items.Add(item);
            }

            SendMessage(this.lvWalls.Handle, WM_SETREDRAW, true, 0);
        }

        private void BtnAddLine_Click(object sender, EventArgs e)
        {
            Wall wall = new Wall(
                (int)this.numX1.Value,
                (int)this.numY1.Value,
                (int)this.numX2.Value,
                (int)this.numY2.Value);

            this.Nodes[this.cboNodes.SelectedIndex].Walls.Add(wall);
            this.UpdateWalls();
            this.LstWalls_SelectedIndexChanged(null, null);
        }

        private void LstWalls_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (this.lvWalls.SelectedItems.Count == 0)
            {
                this.btnWallEdit.Enabled = false;
                return;
            }
            
            Wall wall = this.lvWalls.SelectedItems[0].Tag as Wall;
            if (wall == null)
            {
                MessageBox.Show("Wall is NULL");
                return;
            }

            this.btnMakeRemovePortal.Text = (wall is Portal) ? "Make Wall" : "Make Portal";
            this.UpdatePointControlValues(wall);
        }

        private void UpdatePointControlValues(Wall wall)
        {
            this.numWallR.Enabled = true;
            this.numWallG.Enabled = true;
            this.numWallB.Enabled = true;

            Portal portal = wall as Portal;

            this.numX1.Value = wall.Point1.X;
            this.numY1.Value = wall.Point1.Y;
            this.numX2.Value = wall.Point2.X;
            this.numY2.Value = wall.Point2.Y;
            this.numWallR.Value = wall.Colour.R;
            this.numWallG.Value = wall.Colour.G;
            this.numWallB.Value = wall.Colour.B;

            this.numWallR.Enabled = (portal == null);
            this.numWallG.Enabled = (portal == null);
            this.numWallB.Enabled = (portal == null);
            this.btnWallEdit.Enabled = (portal == null);
            this.numIndex.Value = portal?.NodeIndex ?? 0;
        }

        private void Form1_Shown(object sender, EventArgs e)
        {
            this.numX1.Focus();
        }

        private void CboNodes_SelectionChangeCommitted(object sender, EventArgs e)
        {
            this.UpdateWalls();
            this.numX1.Focus();
            this.numNodeCeiling.Value = this.Nodes[this.cboNodes.SelectedIndex].Ceiling;
            this.numNodeFloor.Value = this.Nodes[this.cboNodes.SelectedIndex].Floor;
        }

        private void BtnRemoveLine_Click(object sender, EventArgs e)
        {
            if (this.lvWalls.SelectedItems.Count == 0)
                return;

            Wall wall = this.lvWalls.SelectedItems[0].Tag as Wall;
            if (wall == null)
                return;

            this.Nodes[this.cboNodes.SelectedIndex].Walls.Remove(wall);
            this.UpdateWalls();
            this.numX1.Focus();
            this.LstWalls_SelectedIndexChanged(null, null);
        }

        private void BtnMakeRemovePortal_Click(object sender, EventArgs e)
        {
            if (this.lvWalls.SelectedItems.Count == 0)
                return;

            Portal portal = this.lvWalls.SelectedItems[0].Tag as Portal;
            if (portal == null)
            {
                Wall wall = this.lvWalls.SelectedItems[0].Tag as Wall;
                if(wall == null)
                    return;

                int index = this.Nodes[this.cboNodes.SelectedIndex].Walls.IndexOf(wall);
                if (index == -1)
                    return;

                this.Nodes[this.cboNodes.SelectedIndex].Walls[index] = new Portal(wall, (int)this.numIndex.Value);
                this.UpdatePointControlValues(wall);
            }
            else
            {
                int index = this.Nodes[this.cboNodes.SelectedIndex].Walls.IndexOf(portal);
                if (index == -1)
                    return;

                this.Nodes[this.cboNodes.SelectedIndex].Walls[index] = new Wall(portal);
                this.UpdatePointControlValues(portal);
            }

            this.UpdateWalls();
            this.numX1.Focus();
        }

        private bool IsPointInNode(Node node, Point point)
        {
            if (node.Walls.Count < 3)
                return false;

            int pos = 0;
            int neg = 0;
            foreach (Wall wall in node.Walls)
            {
                if (wall.Point1 == point || wall.Point2 == point)
                    return true;

                int d = (point.X - wall.Point1.X) * (wall.Point2.Y - wall.Point1.Y) - (point.Y - wall.Point1.Y) * (wall.Point2.X - wall.Point1.X);
                if (d > 0)
                    pos++;
                if (d < 0)
                    neg++;

                if (pos > 0 && neg > 0)
                    return false;
            }

            return true;
        }

        private int FindPlayerNodeIndex()
        {
            Point player = new Point((int)this.numPlayerX.Value / Step, (int)this.numPlayerY.Value / Step);
            for (int i = 0; i < this.Nodes.Count; i++)
            {
                if (this.IsPointInNode(this.Nodes[i], player))
                    return i;
            }

            return -1;
        }

        #region Save

        private void MniSave_Click(object sender, EventArgs e)
        {
            // check player and actor nodes
            int playerNodeIndex = this.FindPlayerNodeIndex();
            if (playerNodeIndex == -1)
            {
                MessageBox.Show("Unable to find player node. Must be outside of level.");
                return;
            }

            Dictionary<int, List<Actor>> actors = new Dictionary<int, List<Actor>>();
            int added = 0;
            for (int i = 0; i < this.Nodes.Count; i++)
            {
                actors.Add(i, new List<Actor>());
                foreach (Actor actor in this.Actors)
                {
                    Point p = new Point(actor.X / Step, actor.Y / Step);
                    if (!this.IsPointInNode(this.Nodes[i], p))
                        continue;

                    actors[i].Add(actor);
                    added++;
                }
            }

            if (this.Actors.Count != added)
            {
                MessageBox.Show("Some actors are outside of the level");
                return;
            }

            double angle = ((double)this.numPlayerAngle.Value * Math.PI / 180d);
            while (angle > 2 * Math.PI)
                angle -= (2 * Math.PI);
            while (angle < 0)
                angle += (2 * Math.PI);            

            StringBuilder builder = new StringBuilder();
            builder.AppendWhitespace("player")
                .AppendWhitespace((int)this.numPlayerX.Value) // x
                .AppendWhitespace((int)this.numPlayerY.Value) // y
                .AppendWhitespace((int)this.numPlayerZ.Value) // z
                .AppendWhitespace((float)angle) // angle (degrees to rad)
                .AppendWhitespace(playerNodeIndex) // player node
                .AppendLine();

            builder.AppendWhitespace("map")
                .AppendWhitespace((float)(this.numMapAmbientLight.Value / 100m)) // light
                .AppendLine();

            for (int i = 0; i < this.Nodes.Count; i++)
            {
                Node node = this.Nodes[i];
                builder.AppendWhitespace("node");
                builder.AppendWhitespace(node.Floor);        // floor
                builder.Append(node.Ceiling);                // ceiling
                builder.AppendLine();

                // copy portals first. this ensures that furthest away room is drawn first, making it so sprites will appear properly
                foreach (Wall wall in node.Walls)
                {
	                if (wall is Portal portal)
                    {

                        builder.AppendWhitespace("portal");
                        builder.AppendWhitespace(portal.Point1.X);
                        builder.AppendWhitespace(portal.Point1.Y);
                        builder.AppendWhitespace(portal.Point2.X);
                        builder.AppendWhitespace(portal.Point2.Y);
                        builder.AppendWhitespace(portal.NodeIndex);
                        builder.AppendLine();
                    }
                    else
                    {
	                    builder.AppendWhitespace("wall");
	                    builder.AppendWhitespace(wall.Point1.X);
	                    builder.AppendWhitespace(wall.Point1.Y);
	                    builder.AppendWhitespace(wall.Point2.X);
	                    builder.AppendWhitespace(wall.Point2.Y);
	                    builder.AppendWhitespace(wall.Colour.ToArgb());
	                    builder.AppendLine();
                    }
                }

                foreach (Actor actor in actors[i])
                {
                    double actorAngle = ((double)actor.Angle * Math.PI / 180d);
                    while (actorAngle > 2 * Math.PI)
                        actorAngle -= (2 * Math.PI);
                    while (actorAngle < 0)
                        actorAngle += (2 * Math.PI);

                    builder.AppendWhitespace("actor");
                    builder.AppendWhitespace(actor.X);
                    builder.AppendWhitespace(actor.Y);
                    builder.AppendWhitespace(actor.Z);
                    builder.AppendWhitespace(actor.ActorTypeIndex);
                    builder.AppendWhitespace((float)actorAngle);
                    builder.AppendLine();
                }
            }

            using (SaveFileDialog dialog = new SaveFileDialog())
            {
                dialog.Filter = "Nova3D Map File (*.map)|*.map|All Files (*.*)|*.*";
                dialog.AddExtension = true;
                if (dialog.ShowDialog() != DialogResult.OK)
                    return;

                using (FileStream file = File.Open(dialog.FileName, FileMode.Create, FileAccess.Write, FileShare.ReadWrite))
                using (StreamWriter writer = new StreamWriter(file))
                    writer.Write(builder.ToString());
            }
        }

#endregion

        #region Parse & Load

        private void MniOpen_Click(object sender, EventArgs e)
        {
            string filename = "";
            using (OpenFileDialog dialog = new OpenFileDialog())
            {
                dialog.Multiselect = false;
                dialog.Filter = "Nova3D Map File (*.map)|*.map|All Files (*.*)|*.*";
                if (dialog.ShowDialog() != DialogResult.OK)
                    return;

                filename = dialog.FileName;
            }

            if (string.IsNullOrEmpty(filename))
                return;

            string text = "";
            using (FileStream file = File.Open(filename, FileMode.Open, FileAccess.Read, FileShare.ReadWrite))
            using (StreamReader reader = new StreamReader(file))
                text = reader.ReadToEnd();

            if (string.IsNullOrEmpty(text))
            {
                MessageBox.Show("File is empty");
                return;
            }

            List<Node> nodes = new List<Node>();
            List<Actor> actors = new List<Actor>();
            Node current = null;
            FileParseModeEnum mode = FileParseModeEnum.NULL;
            foreach (string line in text.Split(new[] { Environment.NewLine}, StringSplitOptions.RemoveEmptyEntries))
            {
                string[] words = line.Split(new[] {' '}, StringSplitOptions.RemoveEmptyEntries);

                // process 
                for (int i = 0; i < words.Length; i++)
                {
                    //string word = words[i];
                    bool isFinishedWithLine = false;
                    if (i == 0)
                    {
                        switch (words[i].ToLower())
                        {
                            case "node":
                                nodes.Add(new Node());
                                current = nodes[nodes.Count - 1];
                                mode = FileParseModeEnum.Node;
                                break;
                            case "portal":
                                mode = FileParseModeEnum.Portal;
                                break;
                            case "wall":
                                mode = FileParseModeEnum.Wall;
                                break;
                            case "player":
                                mode = FileParseModeEnum.Player;
                                break;
                            case "actor":
                                mode = FileParseModeEnum.Actor;
                                break;
                            case "map":
                                mode = FileParseModeEnum.Map;
                                break;
                            default:
                                MessageBox.Show($"Unknown line: {words[i]}");
                                return;
                        }
                    }
                    else
                    {


                        if (current == null && (mode != FileParseModeEnum.Player && mode != FileParseModeEnum.Map))
                        {
                            MessageBox.Show($"Error: No Nodes have been created");
                            return;
                        }

                        switch (mode)
                        {
                            case FileParseModeEnum.Node:
                                current.Floor = Convert.ToInt32(words[i + 0]);
                                current.Ceiling = Convert.ToInt32(words[i + 1]);
                                isFinishedWithLine = true;
                                break;
                            case FileParseModeEnum.Portal:
                                Portal portal = new Portal(
                                    Convert.ToInt32(words[i + 0]),
                                    Convert.ToInt32(words[i + 1]),
                                    Convert.ToInt32(words[i + 2]),
                                    Convert.ToInt32(words[i + 3]),
                                    Convert.ToInt32(words[i + 4]));

                                current.Walls.Add(portal);
                                isFinishedWithLine = true;
                                break;
                            case FileParseModeEnum.Wall:
                                Wall wall = new Wall(
                                    Convert.ToInt32(words[i + 0]),
                                    Convert.ToInt32(words[i + 1]),
                                    Convert.ToInt32(words[i + 2]),
                                    Convert.ToInt32(words[i + 3]));

                                wall.Colour = Color.FromArgb(Convert.ToInt32(words[i + 4]));
                                current.Walls.Add(wall);
                                isFinishedWithLine = true;
                                break;
                            case FileParseModeEnum.Player:
                                this.numPlayerX.Value = Convert.ToInt32(words[i + 0]);
                                this.numPlayerY.Value = Convert.ToInt32(words[i + 1]);
                                this.numPlayerZ.Value = Convert.ToInt32(words[i + 2]);
                                int angle = (int)Math.Round((Convert.ToDouble(words[i + 3]) * 180d / Math.PI));
                                while (angle < 360)
                                    angle += 360;
                                while (angle > 360)
                                    angle -= 360;

                                this.numPlayerAngle.Value = angle;
                                isFinishedWithLine = true;
                                break;
                            case FileParseModeEnum.Actor:
                                int x = Convert.ToInt32(words[i + 0]);
                                int y = Convert.ToInt32(words[i + 1]);
                                int z = Convert.ToInt32(words[i + 2]);
                                int index = Convert.ToInt32(words[i + 3]);
                                double actorAngle = 0;
                                if(words.Length > i + 4)
                                    actorAngle = Convert.ToDouble(words[i + 4]);

                                actors.Add(new Actor(x, y, index, (int)Math.Round(actorAngle * 180d / Math.PI)));
                                isFinishedWithLine = true;
                                break;
                            case FileParseModeEnum.Map:
                                if (words.Length > i)
                                    this.numMapAmbientLight.Value = Math.Round(Convert.ToDecimal(words[i]) * 100m);

                                isFinishedWithLine = true;
                                break;
                            case FileParseModeEnum.NULL:
                            default:                            
                                MessageBox.Show("Error: Second word and mode is NULL");
                                return;
                        }
                    }

                    if (isFinishedWithLine)
                        break;
                }
            }

            // reload map
            this.Nodes = nodes;
            this.Actors = actors;
            this.MousePoint = new Point(-100, -100);
            this.LastPoint = new Point(-100, -100);

            this.cboNodes.Items.Clear();
            for (int i = 0; i < nodes.Count; i++)
                this.cboNodes.Items.Add(i);

            this.cboNodes.SelectedIndex = 0;
            this.UpdateWalls();
            this.UpdateActors();
        }

#endregion

        private void MniClear_Click(object sender, EventArgs e)
        {
            this.Nodes = new List<Node>();
            this.Actors = new List<Actor>();            
            this.cboNodes.Items.Clear();
            this.Form1_Load(null, null);
            this.Form1_Shown(null, null);
            this.UpdateWalls();
            this.UpdateActors();
        }

        private void BtnClearWalls_Click(object sender, EventArgs e)
        {
            this.Nodes[this.cboNodes.SelectedIndex].Walls.Clear();
            this.UpdateWalls();
            this.LstWalls_SelectedIndexChanged(null, null);
        }

        private void UpdateActors()
        {
            SendMessage(this.lvActors.Handle, WM_SETREDRAW, false, 0);
            this.lvActors.Items.Clear();
            foreach (Actor actor in this.Actors)
            {
                ListViewItem item = new ListViewItem(actor.ToString());
                item.Tag = actor;
                this.lvActors.Items.Add(item);
            }

            SendMessage(this.lvActors.Handle, WM_SETREDRAW, true, 0);
        }

        private void BtnAddActor_Click(object sender, EventArgs e)
        {
            Actor actor = new Actor((int)this.numActorX.Value, (int)this.numActorY.Value, (int)this.numActorTypeIndex.Value, (int)this.numActorAngle.Value);
            this.Actors.Add(actor);
            this.UpdateActors();
            this.numActorX.Focus();
        }

        private void BtnRemoveActor_Click(object sender, EventArgs e)
        {
            if (this.lvActors.SelectedItems.Count == 0)
                return;

            Actor actor = this.lvActors.SelectedItems[0].Tag as Actor;
            if (actor == null)
                return;

            this.Actors.Remove(actor);
            this.UpdateActors();
            this.numActorX.Focus();
        }

        private void BtnEditActor_Click(object sender, EventArgs e)
        {
            if (this.lvActors.SelectedItems.Count == 0)
                return;

            int selectedIndex = this.lvActors.Items.IndexOf(this.lvActors.SelectedItems[0]);
            Actor actor = this.lvActors.SelectedItems[0].Tag as Actor;
            if (actor == null)
                return;

            int index = this.Actors.IndexOf(actor);
            if (index == -1)
                return;

            this.Actors[index] = new Actor((int)this.numActorX.Value, (int)this.numActorY.Value, (int)this.numActorTypeIndex.Value, (int)this.numActorAngle.Value);
            this.UpdateActors();

            if (selectedIndex == -1)
            {
                this.numActorX.Focus();
                return;
            }

            this.lvActors.Focus();
            this.lvActors.Items[selectedIndex].Selected = true;
            this.LvActors_SelectedIndexChanged(null, null);
        }

        private void BtnClearActors_Click(object sender, EventArgs e)
        {
            this.Actors.Clear();
            this.UpdateActors();
            this.numActorX.Focus();
        }

        private void LvActors_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (this.lvActors.SelectedItems.Count == 0)
                return;

            Actor actor = this.lvActors.SelectedItems[0].Tag as Actor;
            if (actor == null)
                return;

            this.numActorX.Value = actor.X;
            this.numActorY.Value = actor.Y;
            this.numActorTypeIndex.Value = actor.ActorTypeIndex;
            this.numActorAngle.Value = actor.Angle;
        }

        private void NumWallR_MouseDoubleClick(object sender, MouseEventArgs e)
        {
            Color colour = Color.FromArgb((int)this.numWallR.Value, (int)this.numWallG.Value, (int)this.numWallB.Value);
            using (ColorDialog dialog = new ColorDialog())
            {
                dialog.Color = colour;
                if (dialog.ShowDialog() != DialogResult.OK)
                    return;

                this.numWallR.Value = dialog.Color.R;
                this.numWallG.Value = dialog.Color.G;
                this.numWallB.Value = dialog.Color.B;
            }

            this.BtnWallEdit_Click(null, null);
        }

        private void BtnWallEdit_Click(object sender, EventArgs e)
        {
            if (this.lvWalls.SelectedItems.Count == 0)
                return;

            Wall wall = this.lvWalls.SelectedItems[0].Tag as Wall;
            if (wall == null)
                return;

            int index = this.Nodes[this.cboNodes.SelectedIndex].Walls.IndexOf(wall);
            if (index == -1)
                return;

            Wall edit = new Wall(
                (int)this.numX1.Value,
                (int)this.numY1.Value,
                (int)this.numX2.Value,
                (int)this.numY2.Value);

            edit.Colour = Color.FromArgb((int)this.numWallR.Value, (int)this.numWallG.Value, (int)this.numWallB.Value);
            this.Nodes[this.cboNodes.SelectedIndex].Walls[index] = edit;
            this.UpdateWalls();
        }

        private void NumActorAngle_ValueChanged(object sender, EventArgs e)
        {
            this.BtnEditActor_Click(null, null);
        }
    }
}
