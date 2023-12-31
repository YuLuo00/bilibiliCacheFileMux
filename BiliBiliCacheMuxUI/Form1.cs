using CppAV;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace BiliBiliCacheMuxUI
{
    public partial class Form1 : Form
    {
        public Form1()
        {
            InitializeComponent();
        }

        private void button1_Click(object sender, EventArgs e)
        {
            string vedioFile = @"D:\_project\vs2019\C++\000\bilibiliCache\932815842\2\120\video.m4s";
            string audioFile = @"D:\_project\vs2019\C++\000\bilibiliCache\932815842\2\120\audio.m4s";
            string outPutPath = @"D:\_project\vs2019\C++\000\bilibiliCache\932815842\2\120\output.mp4";
            importClassExample.TestMainFunc(vedioFile, audioFile, outPutPath);
        }
    }
}
