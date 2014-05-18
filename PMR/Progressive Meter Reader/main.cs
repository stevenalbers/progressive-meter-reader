using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Threading;
using System.Windows.Forms;
using System.Xml.Serialization;
using System.IO;
using System.Net.Sockets;
using System.Net;

namespace Progressive_Meter_Reader
{


	public partial class main : Form
	{
		public main()
		{
			InitializeComponent();
			XmlSerializer _serializer = new XmlSerializer(typeof(CabinetInfo));
			CabinetInfo myCabInfo;
			using (FileStream fs = new FileStream("CashBurst.xml", FileMode.Open))
			{
				using (StreamReader sr = new StreamReader(fs))
				{
					myCabInfo = (CabinetInfo)_serializer.Deserialize(sr);
				}
			}
		}

		private void label1_Click(object sender, EventArgs e)
		{

		}

		private void cashBurstDetails_Click(object sender, EventArgs e)
		{
			details cashBurstDetails = new details("Cashburst.xml");
			cashBurstDetails.ShowDialog();
		}

		private void instantRichesDetails_Click(object sender, EventArgs e)
		{
			details instantRichesDetails = new details("InstantRiches.xml");
			instantRichesDetails.ShowDialog();
		}

		private void button1_Click(object sender, EventArgs e)
		{
			string cwd = Directory.GetCurrentDirectory();
			cwd += @"\CashBurst\";
		}
	}

	public class CabinetInfo
	{
		public string GameName { get; set; }
		public int GameID { get; set; }
		public string IPAddress { get; set; }
		public RegionInfo[] SpecifiedRegions { get; set; }
	}
	public class RegionInfo
	{
		public string RegionName { get; set; }
		public int TopLeftX { get; set; }
		public int TopLeftY { get; set; }
		public int BottomRightX { get; set; }
		public int BottomRightY { get; set; }
	}

	public class RecognitionResult
	{
		public string RegionName { get; set; }
		public string MeterValue { get; set; }
	}

	public class Server
    {
		private void button1_Click(object sender, EventArgs e)
        {
	        this.createListener();
        }

        public void createListener()
{
    string output = "";

	// Create an instance of the TcpListener class.
	TcpListener tcpListener = null;
	IPAddress ipAddress = Dns.GetHostEntry("localhost").AddressList[0];
	try
	{
		// Set the listener on the local IP address 
		// and specify the port.
		tcpListener = new TcpListener(ipAddress, 8950);
		tcpListener.Start();
		output = "Waiting for a connection...";
	}
	catch (Exception e)
	{
		output = "Error: " + e.ToString();
		MessageBox.Show(output);
	}
	while (true)
	{
		// Always use a Sleep call in a while(true) loop 
		// to avoid locking up your CPU.
		Thread.Sleep(10);
		// Create a TCP socket. 
		// If you ran this server on the desktop, you could use 
		// Socket socket = tcpListener.AcceptSocket() 
		// for greater flexibility.
		TcpClient tcpClient = tcpListener.AcceptTcpClient();
		// Read the data stream from the client. 
		byte[] bytes = new byte[256];
		NetworkStream stream = tcpClient.GetStream();
		stream.Read(bytes, 0, bytes.Length);
		SocketHelper helper = new SocketHelper();
		helper.processMsg(tcpClient, stream, bytes);
	}
}

    }

    public class Client
       {

        static void Connect(string serverIP, string message)
        {
            string output = "";

            try
            {
                // Create a TcpClient. 
                // The client requires a TcpServer that is connected 
                // to the same address specified by the server and port 
                // combination.
                Int32 port = 8950;
                TcpClient client = new TcpClient(serverIP, port);

                // Translate the passed message into ASCII and store it as a byte array.
                Byte[] data = new Byte[256];
                data = System.Text.Encoding.ASCII.GetBytes(message);

                // Get a client stream for reading and writing. 
                // Stream stream = client.GetStream();
                NetworkStream stream = client.GetStream();

                // Send the message to the connected TcpServer. 
                stream.Write(data, 0, data.Length);

                output = "Sent: " + message;
                MessageBox.Show(output);

                // Buffer to store the response bytes.
                data = new Byte[256];

                // String to store the response ASCII representation.
                String responseData = String.Empty;

                // Read the first batch of the TcpServer response bytes.
                Int32 bytes = stream.Read(data, 0, data.Length);
                responseData = System.Text.Encoding.ASCII.GetString(data, 0, bytes);
                output = "Received: " + responseData;
                MessageBox.Show(output);

                // Close everything.
                stream.Close();
                client.Close();
            }
            catch (ArgumentNullException e)
            {
                output = "ArgumentNullException: " + e;
                MessageBox.Show(output);
            }
            catch (SocketException e)
            {
                output = "SocketException: " + e.ToString();
                MessageBox.Show(output);
            }
        }
       }

	




public class SocketHelper
{
    TcpClient mscClient;
    string mstrMessage;
    string mstrResponse;
    byte[] bytesSent;
    public void processMsg(TcpClient client, NetworkStream stream, byte[] bytesReceived)
    {
        // Handle the message received and  
        // send a response back to the client.
        mstrMessage = Encoding.ASCII.GetString(bytesReceived, 0, bytesReceived.Length);
        mscClient = client;
        mstrMessage = mstrMessage.Substring(0, 5);
        if (mstrMessage.Equals("Hello"))
        {
            mstrResponse = "Goodbye";
        }
        else
        {
            mstrResponse = "What?";
        }
        bytesSent = Encoding.ASCII.GetBytes(mstrResponse);
        stream.Write(bytesSent, 0, bytesSent.Length);
    }
}

}
