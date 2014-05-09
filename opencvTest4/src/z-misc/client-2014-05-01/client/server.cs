using System;
using System.Runtime.InteropServices;
using System.Drawing;
using System.Drawing.Imaging;
using System.Text;
using System.Net;
using System.Net.Sockets;

namespace myServer
{
	public class serverTest
	{
		public static void Main(string[] args)
		{
			string fileName = null;
			if(args.Length == 0)
				fileName = "default";
			else
				fileName = args[0];
			
			string fileType = ".jpg";
			string fullName = fileName + fileType;

			Socket s = null;
			s = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
			s.Connect(new IPEndPoint(IPAddress.Parse("127.0.0.1"), 9988));

			Socket client = s;
			Image img = Image.FromStream(new NetworkStream(client));
			img.Save(fullName, System.Drawing.Imaging.ImageFormat.Jpeg);

			s.Close();
		}
	}
}