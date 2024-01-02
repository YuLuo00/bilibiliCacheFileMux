using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using System.Windows.Forms;
using CppAV;
namespace BiliBiliCacheMuxUI
{
    internal static class Program
    {
        /// <summary>
        /// 应用程序的主入口点。
        /// </summary>
        [STAThread]
        static void Main()
        {
            string vedioFile = @"D:\_project\vs2019\C++\000\bilibiliCache\932815842\2\120\video.m4s";
            string audioFile = @"D:\_project\vs2019\C++\000\bilibiliCache\932815842\2\120\audio.m4s";
            string outPutPath = @"C:\Users\Administrator\Desktop\output.mp4";
            importClassExample.TestMainFunc(vedioFile, audioFile, outPutPath);
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            Application.Run(new Form1());
        }
    }
}
