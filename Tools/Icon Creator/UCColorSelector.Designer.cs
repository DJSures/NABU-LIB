namespace Icon_Creator {
  partial class UCColorSelector {
    /// <summary> 
    /// Required designer variable.
    /// </summary>
    private System.ComponentModel.IContainer components = null;

    /// <summary> 
    /// Clean up any resources being used.
    /// </summary>
    /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
    protected override void Dispose(bool disposing) {
      if (disposing && (components != null)) {
        components.Dispose();
      }
      base.Dispose(disposing);
    }

    #region Component Designer generated code

    /// <summary> 
    /// Required method for Designer support - do not modify 
    /// the contents of this method with the code editor.
    /// </summary>
    private void InitializeComponent() {
      cbForegroundColor = new ComboBox();
      cbBackgroundColor = new ComboBox();
      SuspendLayout();
      // 
      // cbForegroundColor
      // 
      cbForegroundColor.Dock = DockStyle.Left;
      cbForegroundColor.DropDownStyle = ComboBoxStyle.DropDownList;
      cbForegroundColor.FormattingEnabled = true;
      cbForegroundColor.Location = new Point(0, 0);
      cbForegroundColor.Name = "cbForegroundColor";
      cbForegroundColor.Size = new Size(80, 23);
      cbForegroundColor.TabIndex = 0;
      cbForegroundColor.SelectedIndexChanged += cbForegroundColor_SelectedIndexChanged;
      // 
      // cbBackgroundColor
      // 
      cbBackgroundColor.Dock = DockStyle.Left;
      cbBackgroundColor.DropDownStyle = ComboBoxStyle.DropDownList;
      cbBackgroundColor.FormattingEnabled = true;
      cbBackgroundColor.Location = new Point(80, 0);
      cbBackgroundColor.Name = "cbBackgroundColor";
      cbBackgroundColor.Size = new Size(80, 23);
      cbBackgroundColor.TabIndex = 1;
      cbBackgroundColor.SelectedIndexChanged += cbBackgroundColor_SelectedIndexChanged;
      // 
      // UCColorSelector
      // 
      AutoScaleDimensions = new SizeF(7F, 15F);
      AutoScaleMode = AutoScaleMode.Font;
      Controls.Add(cbBackgroundColor);
      Controls.Add(cbForegroundColor);
      Name = "UCColorSelector";
      Size = new Size(170, 29);
      ResumeLayout(false);
    }

    #endregion

    private ComboBox cbForegroundColor;
    private ComboBox cbBackgroundColor;
  }
}
