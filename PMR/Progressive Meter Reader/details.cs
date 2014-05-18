using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Xml.Serialization;
using System.Text.RegularExpressions;

namespace Progressive_Meter_Reader
{
    public partial class details : Form
    {
        System.IO.StreamWriter fileWriter;
        CabinetInfo myCabInfo;

        static int sequence = 0;


        public details(string profileFileName)
        {
            InitializeComponent();

            XmlSerializer _serializer = new XmlSerializer(typeof(CabinetInfo));
            using (FileStream fs = new FileStream(profileFileName, FileMode.Open))
            {
                using (StreamReader sr = new StreamReader(fs))
                {
                    myCabInfo = (CabinetInfo)_serializer.Deserialize(sr);
                }
            }

            lblGameName.Text = myCabInfo.GameName;
            regionInfo.DataSource = myCabInfo.SpecifiedRegions;

            
            // read the last image of the game and set it to the Image
            if (myCabInfo.GameName == "Cash Burst")
            {

                // Get most recent picture
                int[] mySequence = {0};
                int i = 0;
                string cwd = Directory.GetCurrentDirectory() + @"\CashBurst\";
                string[] imgFiles = Directory.GetFiles(cwd, "*.jpg", SearchOption.TopDirectoryOnly);

                int maxSequenceNumber = 0;
                string lastCapture = "";                

                // get an array of image files

                // find sequence number in each image file

                // get max sequence number

                // assign max seq number image to image field

                foreach (string imageFile in imgFiles)
                {
                    // break into tokens

                    string[] fileParts = imageFile.Split('_');

                    // create array of sequence numbers, don't forget original paths
                    mySequence[i] = Convert.ToInt32(fileParts[1]);


                    maxSequenceNumber = mySequence.Max();

                    if (Convert.ToInt32(fileParts[1]) == maxSequenceNumber)
                    {
                        lastCapture = imageFile;
                    }
                }

                meterImage.Image = Image.FromFile(lastCapture);




                // Compile history
                string[] historyFiles = Directory.GetFiles(cwd, "*.txt", SearchOption.TopDirectoryOnly);

                foreach (string textFile in historyFiles)
                {
                    historyDropdown.Items.Add(Path.GetFileNameWithoutExtension(textFile));
                }

            }

            else if (myCabInfo.GameName == "Instant Riches")
            {
                int[] mySequence = { 0 };
                int i = 0;
                string cwd = Directory.GetCurrentDirectory() + @"\InstantRiches\";
                string[] files = Directory.GetFiles(cwd, "*.jpg", SearchOption.TopDirectoryOnly);

                int maxSequenceNumber = 0;
                string lastCapture = "";

                // get an array of image files

                // find sequence number in each image file

                // get max sequence number

                // assign max seq number image to image field

                foreach (string imageFile in files)
                {
                    // break into tokens
                    string[] fileParts = imageFile.Split('_');

                    // create array of sequence numbers, don't forget original paths
                    mySequence[i] = Convert.ToInt32(fileParts[1]);

                    maxSequenceNumber = mySequence.Max();

                    if (Convert.ToInt32(fileParts[1]) == maxSequenceNumber)
                    {
                        lastCapture = imageFile;
                    }
                }

                meterImage.Image = Image.FromFile(lastCapture);
            }

            
            // Get all .jpg files from the game profile folder
            // Find the jpg file with the largest sequence number

            //string lastCaptureDateTimeString = string.Format("{0}/{1}/{2} {3}:{4}:ss", 3, 19, 2014);
            // read the last recognition result and set UI data to it
            //DateTime lastCaptureDateTime = DateTime.Parse( lastCaptureDateTimeString);
        }

        private void label1_Click(object sender, EventArgs e)
        {

        }

        private void label9_Click(object sender, EventArgs e)
        {

        }

        private void regionInfo_CellContentClick(object sender, DataGridViewCellEventArgs e)
        {

        }

        private void historyDropdown_SelectedIndexChanged(object sender, EventArgs e)
        {
            var historyFile = Directory.GetCurrentDirectory() + @"\CashBurst\" + historyDropdown.Text + ".txt";
            var historyImg = Directory.GetCurrentDirectory() + @"\CashBurst\" + historyDropdown.Text + ".jpg";
            var rows = System.IO.File.ReadAllLines(historyFile);
            Char[] delimiter = new Char[] { ',' };
            DataTable historyTable = new DataTable(historyFile);
            if (rows.Length != 0)
            {
                foreach (string headerCol in rows[0].Split(delimiter))
                {
                    historyTable.Columns.Add(new DataColumn(headerCol));
                }
                if (rows.Length > 1)
                {
                    for (int rowIndex = 1; rowIndex < rows.Length; rowIndex++)
                    {
                        var newRow = historyTable.NewRow();
                        var cols = rows[rowIndex].Split(delimiter);
                        for (int colIndex = 0; colIndex < cols.Length; colIndex++)
                        {
                            newRow[colIndex] = cols[colIndex];
                        }
                        historyTable.Rows.Add(newRow);
                    }
                }
            }
            historyInfo.DataSource = historyTable;
            meterImage.Image = Image.FromFile(historyImg);
        }


        private void historyInfo_CellClick(object sender, DataGridViewCellEventArgs e)
        {
            correctValueLabel.Text = historyInfo.CurrentRow.Cells[0].Value.ToString() + " Value: ";
            selectedJackpotValue.Text = historyInfo.CurrentRow.Cells[1].Value.ToString();

        }

        private void updateMeter_Click(object sender, EventArgs e)
        {
            historyInfo.CurrentRow.Cells[1].Value = valueInput.Text;
            selectedJackpotValue.Text = valueInput.Text;

            //string textFile = historyDropdown.Text + ".txt";
            
            var textFile = Directory.GetCurrentDirectory() + @"\CashBurst\" + historyDropdown.Text + ".txt";
            fileWriter = new System.IO.StreamWriter(textFile);

            int count = historyInfo.Rows.Count;




            fileWriter.WriteLine("Region Name,Meter Value");

            for (int row = 0; row < count; row++)
            {
                int colCount = historyInfo.Rows[row].Cells.Count;

                string lines = "";

                for (int col = 0; col < colCount; col++)
                {
                    lines += (string.IsNullOrEmpty(lines) ? " " : ", ") + historyInfo.Rows[row].Cells[col].Value.ToString();
                }

                fileWriter.WriteLine(lines);
            }

            fileWriter.Close();

        }

        private void testButton_Click(object sender, EventArgs e)
        {
        }

        static void requestScreenshot(string outputfile)
        {
            // For the example

            // Use ProcessStartInfo class
            ProcessStartInfo startInfo = new ProcessStartInfo();
            startInfo.CreateNoWindow = false;
            startInfo.UseShellExecute = false;

            // NICK NICK NICK NICK NICK NICK NICK NICK NICK NICK
            // Drop your executable here =]
            startInfo.FileName = @"\src\inputImages\server.exe";
            startInfo.WindowStyle = ProcessWindowStyle.Hidden;
            startInfo.Arguments = outputfile;

            try
            {
                // Start the process with the info we specified.
                // Call WaitForExit and then the using statement will close.
                using (Process exeProcess = Process.Start(startInfo))
                {
                    exeProcess.WaitForExit();
                }
            }
            catch
            {
                // Log error.
            }
        }

        static void classifyRegions(string outputFile)
        {
            // For the example

            // Use ProcessStartInfo class
            ProcessStartInfo startInfo = new ProcessStartInfo();
            startInfo.CreateNoWindow = false;
            startInfo.UseShellExecute = false;

            // ANDREY ANDREY ANDREY ANDREY ANDREY ANDREY
            // Drop your executable here =]
            startInfo.FileName = @"\opencvTest4.exe";
            startInfo.WindowStyle = ProcessWindowStyle.Hidden;

            // This is where the arguments go
            startInfo.Arguments = outputFile;

            try
            {
                // Start the process with the info we specified.
                // Call WaitForExit and then the using statement will close.
                using (Process exeProcess = Process.Start(startInfo))
                {
                    exeProcess.WaitForExit();
                }
            }
            catch
            {
                // Log error.
            }
        }

        private void recognizeRegions_Click(object sender, EventArgs e)
        {
            // get datetime
            Regex symbols = new Regex("[,/: ]");
            Regex characters = new Regex("[AMP]");

            string cwd = Directory.GetCurrentDirectory();
            //test.Text = cwd;

            String[] timeDelimiter = new String[] { ",", "/", ":", " " };
            DateTime currentTime = DateTime.Now;

            screenTime.Text = currentTime.ToString();
          
            String timeString = "";

            timeString = currentTime.ToString();

            // replace any delimiter with _
            timeString = symbols.Replace(timeString, "_");
            timeString = characters.Replace(timeString, "");
            timeString = timeString.TrimEnd('_');

            // get sequence number
            details.sequence++;
            
            // first, find most current sequence number
            string fileName = "cb_" + sequence.ToString() + "_" + timeString;

            // call client app (parameter is output file name)
            requestScreenshot(fileName);

            // wait until screenshot is taken
            System.Threading.Thread.Sleep(1000);

            // call recognition (parameters are input image, output text names, game id)
            classifyRegions(fileName);

            // win!
            Refresh();
        }
    }
}
