namespace Progressive_Meter_Reader
{
    partial class details
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.meterImage = new System.Windows.Forms.PictureBox();
            this.recognizeRegions = new System.Windows.Forms.Button();
            this.valueInput = new System.Windows.Forms.TextBox();
            this.correctValueLabel = new System.Windows.Forms.Label();
            this.selectedJackpotValue = new System.Windows.Forms.Label();
            this.label3 = new System.Windows.Forms.Label();
            this.updateMeter = new System.Windows.Forms.Button();
            this.historyDropdown = new System.Windows.Forms.ComboBox();
            this.label4 = new System.Windows.Forms.Label();
            this.label35 = new System.Windows.Forms.Label();
            this.screenTime = new System.Windows.Forms.Label();
            this.lblGameName = new System.Windows.Forms.Label();
            this.regionInfo = new System.Windows.Forms.DataGridView();
            this.statusStrip1 = new System.Windows.Forms.StatusStrip();
            this.historyInfo = new System.Windows.Forms.DataGridView();
            ((System.ComponentModel.ISupportInitialize)(this.meterImage)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.regionInfo)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.historyInfo)).BeginInit();
            this.SuspendLayout();
            // 
            // meterImage
            // 
            this.meterImage.BackgroundImageLayout = System.Windows.Forms.ImageLayout.Stretch;
            this.meterImage.Location = new System.Drawing.Point(112, 78);
            this.meterImage.Name = "meterImage";
            this.meterImage.Size = new System.Drawing.Size(640, 480);
            this.meterImage.SizeMode = System.Windows.Forms.PictureBoxSizeMode.StretchImage;
            this.meterImage.TabIndex = 0;
            this.meterImage.TabStop = false;
            // 
            // recognizeRegions
            // 
            this.recognizeRegions.Location = new System.Drawing.Point(112, 565);
            this.recognizeRegions.Name = "recognizeRegions";
            this.recognizeRegions.Size = new System.Drawing.Size(237, 32);
            this.recognizeRegions.TabIndex = 1;
            this.recognizeRegions.Text = "Update Meters";
            this.recognizeRegions.UseVisualStyleBackColor = true;
            this.recognizeRegions.Click += new System.EventHandler(this.recognizeRegions_Click);
            // 
            // valueInput
            // 
            this.valueInput.Location = new System.Drawing.Point(1114, 845);
            this.valueInput.Name = "valueInput";
            this.valueInput.Size = new System.Drawing.Size(180, 26);
            this.valueInput.TabIndex = 5;
            // 
            // correctValueLabel
            // 
            this.correctValueLabel.AutoSize = true;
            this.correctValueLabel.Font = new System.Drawing.Font("Microsoft Sans Serif", 18F);
            this.correctValueLabel.Location = new System.Drawing.Point(1128, 657);
            this.correctValueLabel.Name = "correctValueLabel";
            this.correctValueLabel.Size = new System.Drawing.Size(0, 40);
            this.correctValueLabel.TabIndex = 6;
            this.correctValueLabel.Click += new System.EventHandler(this.label1_Click);
            // 
            // selectedJackpotValue
            // 
            this.selectedJackpotValue.AutoSize = true;
            this.selectedJackpotValue.Font = new System.Drawing.Font("Microsoft Sans Serif", 18F);
            this.selectedJackpotValue.Location = new System.Drawing.Point(1124, 715);
            this.selectedJackpotValue.Name = "selectedJackpotValue";
            this.selectedJackpotValue.Size = new System.Drawing.Size(0, 40);
            this.selectedJackpotValue.TabIndex = 7;
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Font = new System.Drawing.Font("Microsoft Sans Serif", 12F);
            this.label3.Location = new System.Drawing.Point(1106, 812);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(214, 29);
            this.label3.TabIndex = 8;
            this.label3.Text = "Input correct value:";
            // 
            // updateMeter
            // 
            this.updateMeter.Location = new System.Drawing.Point(1130, 877);
            this.updateMeter.Name = "updateMeter";
            this.updateMeter.Size = new System.Drawing.Size(152, 32);
            this.updateMeter.TabIndex = 9;
            this.updateMeter.Text = "Update";
            this.updateMeter.UseVisualStyleBackColor = true;
            this.updateMeter.Click += new System.EventHandler(this.updateMeter_Click);
            // 
            // historyDropdown
            // 
            this.historyDropdown.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.historyDropdown.FormattingEnabled = true;
            this.historyDropdown.Location = new System.Drawing.Point(1052, 138);
            this.historyDropdown.Name = "historyDropdown";
            this.historyDropdown.Size = new System.Drawing.Size(366, 28);
            this.historyDropdown.TabIndex = 10;
            this.historyDropdown.SelectedIndexChanged += new System.EventHandler(this.historyDropdown_SelectedIndexChanged);
            // 
            // label4
            // 
            this.label4.AutoSize = true;
            this.label4.Font = new System.Drawing.Font("Microsoft Sans Serif", 18F);
            this.label4.Location = new System.Drawing.Point(1172, 78);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(129, 40);
            this.label4.TabIndex = 11;
            this.label4.Text = "History";
            // 
            // label35
            // 
            this.label35.AutoSize = true;
            this.label35.Font = new System.Drawing.Font("Microsoft Sans Serif", 9F);
            this.label35.Location = new System.Drawing.Point(434, 565);
            this.label35.Name = "label35";
            this.label35.Size = new System.Drawing.Size(180, 22);
            this.label35.TabIndex = 13;
            this.label35.Text = "Screenshot taken at: ";
            // 
            // screenTime
            // 
            this.screenTime.AutoSize = true;
            this.screenTime.Font = new System.Drawing.Font("Microsoft Sans Serif", 9F);
            this.screenTime.Location = new System.Drawing.Point(608, 565);
            this.screenTime.Name = "screenTime";
            this.screenTime.Size = new System.Drawing.Size(145, 22);
            this.screenTime.TabIndex = 14;
            this.screenTime.Text = "3/20/14 12:30:00";
            // 
            // lblGameName
            // 
            this.lblGameName.AutoSize = true;
            this.lblGameName.Font = new System.Drawing.Font("Microsoft Sans Serif", 24F);
            this.lblGameName.Location = new System.Drawing.Point(690, 9);
            this.lblGameName.Name = "lblGameName";
            this.lblGameName.Size = new System.Drawing.Size(328, 55);
            this.lblGameName.TabIndex = 15;
            this.lblGameName.Text = "Instant Riches";
            // 
            // regionInfo
            // 
            this.regionInfo.AllowUserToAddRows = false;
            this.regionInfo.AllowUserToDeleteRows = false;
            this.regionInfo.ColumnHeadersHeightSizeMode = System.Windows.Forms.DataGridViewColumnHeadersHeightSizeMode.AutoSize;
            this.regionInfo.Location = new System.Drawing.Point(33, 642);
            this.regionInfo.Name = "regionInfo";
            this.regionInfo.ReadOnly = true;
            this.regionInfo.RowTemplate.Height = 28;
            this.regionInfo.Size = new System.Drawing.Size(813, 268);
            this.regionInfo.TabIndex = 17;
            this.regionInfo.CellContentClick += new System.Windows.Forms.DataGridViewCellEventHandler(this.regionInfo_CellContentClick);
            // 
            // statusStrip1
            // 
            this.statusStrip1.Location = new System.Drawing.Point(0, 972);
            this.statusStrip1.Name = "statusStrip1";
            this.statusStrip1.Padding = new System.Windows.Forms.Padding(2, 0, 14, 0);
            this.statusStrip1.Size = new System.Drawing.Size(1658, 22);
            this.statusStrip1.TabIndex = 18;
            this.statusStrip1.Text = "statusStrip1";
            // 
            // historyInfo
            // 
            this.historyInfo.AllowUserToAddRows = false;
            this.historyInfo.AllowUserToDeleteRows = false;
            this.historyInfo.AllowUserToResizeColumns = false;
            this.historyInfo.AllowUserToResizeRows = false;
            this.historyInfo.ColumnHeadersHeightSizeMode = System.Windows.Forms.DataGridViewColumnHeadersHeightSizeMode.AutoSize;
            this.historyInfo.Location = new System.Drawing.Point(1052, 225);
            this.historyInfo.Name = "historyInfo";
            this.historyInfo.ReadOnly = true;
            this.historyInfo.RowHeadersWidthSizeMode = System.Windows.Forms.DataGridViewRowHeadersWidthSizeMode.AutoSizeToAllHeaders;
            this.historyInfo.RowTemplate.Height = 28;
            this.historyInfo.SelectionMode = System.Windows.Forms.DataGridViewSelectionMode.FullRowSelect;
            this.historyInfo.Size = new System.Drawing.Size(364, 334);
            this.historyInfo.TabIndex = 19;
            this.historyInfo.CellClick += new System.Windows.Forms.DataGridViewCellEventHandler(this.historyInfo_CellClick);
            // 
            // details
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(9F, 20F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(1658, 994);
            this.Controls.Add(this.historyInfo);
            this.Controls.Add(this.statusStrip1);
            this.Controls.Add(this.regionInfo);
            this.Controls.Add(this.lblGameName);
            this.Controls.Add(this.screenTime);
            this.Controls.Add(this.label35);
            this.Controls.Add(this.label4);
            this.Controls.Add(this.historyDropdown);
            this.Controls.Add(this.updateMeter);
            this.Controls.Add(this.label3);
            this.Controls.Add(this.selectedJackpotValue);
            this.Controls.Add(this.correctValueLabel);
            this.Controls.Add(this.valueInput);
            this.Controls.Add(this.recognizeRegions);
            this.Controls.Add(this.meterImage);
            this.Name = "details";
            this.Text = "Details";
            ((System.ComponentModel.ISupportInitialize)(this.meterImage)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.regionInfo)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.historyInfo)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.PictureBox meterImage;
        private System.Windows.Forms.Button recognizeRegions;
        private System.Windows.Forms.TextBox valueInput;
        private System.Windows.Forms.Label correctValueLabel;
        private System.Windows.Forms.Label selectedJackpotValue;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.Button updateMeter;
        private System.Windows.Forms.ComboBox historyDropdown;
        private System.Windows.Forms.Label label4;
        private System.Windows.Forms.Label label35;
        private System.Windows.Forms.Label screenTime;
        private System.Windows.Forms.Label lblGameName;
        private System.Windows.Forms.StatusStrip statusStrip1;
        public System.Windows.Forms.DataGridView regionInfo;
        private System.Windows.Forms.DataGridView historyInfo;
    }
}

