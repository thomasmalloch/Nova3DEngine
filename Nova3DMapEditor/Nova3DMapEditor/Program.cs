using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace Nova3DMapEditor
{
    static class Program
    {
        #region StringBuilder Extensions

        public static StringBuilder AppendWhitespace(this StringBuilder builder, string text)
        {
            return builder.Append(text).Append(" ");
        }

        public static StringBuilder AppendWhitespace(this StringBuilder builder, int num)
        {
            return builder.Append(num).Append(" ");
        }

        public static StringBuilder AppendWhitespace(this StringBuilder builder, float num)
        {
            return builder.Append(num).Append(" ");
        }

        #endregion

        /// <summary>
        /// The main entry point for the application.
        /// </summary>
        [STAThread]
        static void Main()
        {
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            Application.Run(new Form1());
        }
    }
}
