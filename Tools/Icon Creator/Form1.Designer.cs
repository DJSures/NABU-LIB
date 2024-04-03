namespace Icon_Creator {
  partial class Form1 {
    /// <summary>
    ///  Required designer variable.
    /// </summary>
    private System.ComponentModel.IContainer components = null;

    /// <summary>
    ///  Clean up any resources being used.
    /// </summary>
    /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
    protected override void Dispose(bool disposing) {
      if (disposing && (components != null)) {
        components.Dispose();
      }
      base.Dispose(disposing);
    }

    #region Windows Form Designer generated code

    /// <summary>
    ///  Required method for Designer support - do not modify
    ///  the contents of this method with the code editor.
    /// </summary>
    private void InitializeComponent() {
      menuStrip1 = new MenuStrip();
      fileToolStripMenuItem = new ToolStripMenuItem();
      nEwToolStripMenuItem = new ToolStripMenuItem();
      openToolStripMenuItem = new ToolStripMenuItem();
      saveToolStripMenuItem1 = new ToolStripMenuItem();
      toolStripMenuItem1 = new ToolStripSeparator();
      exitToolStripMenuItem = new ToolStripMenuItem();
      drawToolStripMenuItem = new ToolStripMenuItem();
      sketchToolStripMenuItem = new ToolStripMenuItem();
      getPatternJSONStringToolStripMenuItem = new ToolStripMenuItem();
      getByteArrayToolStripMenuItem = new ToolStripMenuItem();
      menuStrip1.SuspendLayout();
      SuspendLayout();
      // 
      // menuStrip1
      // 
      menuStrip1.Items.AddRange(new ToolStripItem[] { fileToolStripMenuItem, drawToolStripMenuItem, getPatternJSONStringToolStripMenuItem, getByteArrayToolStripMenuItem });
      menuStrip1.Location = new Point(0, 0);
      menuStrip1.Name = "menuStrip1";
      menuStrip1.Size = new Size(936, 24);
      menuStrip1.TabIndex = 0;
      menuStrip1.Text = "menuStrip1";
      // 
      // fileToolStripMenuItem
      // 
      fileToolStripMenuItem.DropDownItems.AddRange(new ToolStripItem[] { nEwToolStripMenuItem, openToolStripMenuItem, saveToolStripMenuItem1, toolStripMenuItem1, exitToolStripMenuItem });
      fileToolStripMenuItem.Name = "fileToolStripMenuItem";
      fileToolStripMenuItem.Size = new Size(37, 20);
      fileToolStripMenuItem.Text = "&File";
      // 
      // nEwToolStripMenuItem
      // 
      nEwToolStripMenuItem.Name = "nEwToolStripMenuItem";
      nEwToolStripMenuItem.ShortcutKeys = Keys.Control | Keys.N;
      nEwToolStripMenuItem.Size = new Size(146, 22);
      nEwToolStripMenuItem.Text = "&New";
      nEwToolStripMenuItem.Click += nEwToolStripMenuItem_Click;
      // 
      // openToolStripMenuItem
      // 
      openToolStripMenuItem.Name = "openToolStripMenuItem";
      openToolStripMenuItem.ShortcutKeys = Keys.Control | Keys.O;
      openToolStripMenuItem.Size = new Size(146, 22);
      openToolStripMenuItem.Text = "&Open";
      openToolStripMenuItem.Click += loadToolStripMenuItem_Click;
      // 
      // saveToolStripMenuItem1
      // 
      saveToolStripMenuItem1.Name = "saveToolStripMenuItem1";
      saveToolStripMenuItem1.ShortcutKeys = Keys.Control | Keys.S;
      saveToolStripMenuItem1.Size = new Size(146, 22);
      saveToolStripMenuItem1.Text = "S&ave";
      saveToolStripMenuItem1.Click += saveToolStripMenuItem_Click;
      // 
      // toolStripMenuItem1
      // 
      toolStripMenuItem1.Name = "toolStripMenuItem1";
      toolStripMenuItem1.Size = new Size(143, 6);
      // 
      // exitToolStripMenuItem
      // 
      exitToolStripMenuItem.Name = "exitToolStripMenuItem";
      exitToolStripMenuItem.Size = new Size(146, 22);
      exitToolStripMenuItem.Text = "E&xit";
      // 
      // drawToolStripMenuItem
      // 
      drawToolStripMenuItem.DropDownItems.AddRange(new ToolStripItem[] { sketchToolStripMenuItem });
      drawToolStripMenuItem.Name = "drawToolStripMenuItem";
      drawToolStripMenuItem.Size = new Size(46, 20);
      drawToolStripMenuItem.Text = "Tools";
      // 
      // sketchToolStripMenuItem
      // 
      sketchToolStripMenuItem.Name = "sketchToolStripMenuItem";
      sketchToolStripMenuItem.ShortcutKeys = Keys.Control | Keys.D;
      sketchToolStripMenuItem.Size = new Size(151, 22);
      sketchToolStripMenuItem.Text = "Sketch";
      sketchToolStripMenuItem.ToolTipText = "When enabled, sketching on the grid is possible.";
      sketchToolStripMenuItem.Click += sketchToolStripMenuItem_Click;
      // 
      // getPatternJSONStringToolStripMenuItem
      // 
      getPatternJSONStringToolStripMenuItem.Name = "getPatternJSONStringToolStripMenuItem";
      getPatternJSONStringToolStripMenuItem.Size = new Size(102, 20);
      getPatternJSONStringToolStripMenuItem.Text = "Get JSON String";
      getPatternJSONStringToolStripMenuItem.Click += getPatternJSONStringToolStripMenuItem_Click;
      // 
      // getByteArrayToolStripMenuItem
      // 
      getByteArrayToolStripMenuItem.Name = "getByteArrayToolStripMenuItem";
      getByteArrayToolStripMenuItem.Size = new Size(94, 20);
      getByteArrayToolStripMenuItem.Text = "Get Byte Array";
      getByteArrayToolStripMenuItem.Click += getByteArrayToolStripMenuItem_Click;
      // 
      // Form1
      // 
      AutoScaleDimensions = new SizeF(7F, 15F);
      AutoScaleMode = AutoScaleMode.Font;
      ClientSize = new Size(936, 521);
      Controls.Add(menuStrip1);
      MainMenuStrip = menuStrip1;
      Name = "Form1";
      Text = "Internet Adapter Icon Creator";
      menuStrip1.ResumeLayout(false);
      menuStrip1.PerformLayout();
      ResumeLayout(false);
      PerformLayout();
    }

    #endregion

    private MenuStrip menuStrip1;
    private ToolStripMenuItem getPatternJSONStringToolStripMenuItem;
    private ToolStripMenuItem fileToolStripMenuItem;
    private ToolStripMenuItem nEwToolStripMenuItem;
    private ToolStripMenuItem openToolStripMenuItem;
    private ToolStripMenuItem saveToolStripMenuItem1;
    private ToolStripSeparator toolStripMenuItem1;
    private ToolStripMenuItem exitToolStripMenuItem;
    private ToolStripMenuItem getByteArrayToolStripMenuItem;
    private ToolStripMenuItem drawToolStripMenuItem;
    private ToolStripMenuItem sketchToolStripMenuItem;
  }
}
