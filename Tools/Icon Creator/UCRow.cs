using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace Icon_Creator {

  public partial class UCRow : UserControl {

    private const int PixelSize = 30; // Size of each pixel/button
    private Button[] pixelButtons = new Button[8];

    private bool[] pixelColors = new bool[8];

    public static Color[] TMS9918aPalettes = new Color[] {
    Color.Silver, // Transparent (Black)
    Color.Black, // 1
    Color.Green, // 2
    Color.LightGreen, // 3
    Color.DarkBlue, // 4
    Color.LightBlue, // 5
    Color.DarkRed, // 6
    Color.Cyan, // 7
    Color.Red, // 8
    Color.OrangeRed, // 9
    Color.GreenYellow, // 10
    Color.Yellow, // 11
    Color.DarkGreen, // 12
    Color.Magenta, // 13
    Color.Gray, // 14
    Color.White // 15
    };

    public event EventHandler ImageUpdated;

    public static bool SketchDrawingEnabled = false;
    public static List<string> SketchDrawedIds = new List<string>();

    public int ForeGroundColor = 1;
    public int BackgroundColor = 15;

    public UCRow() {

      InitializeComponent();

      this.ClientSize = new Size(16 * PixelSize + 10, 16);

      for (int col = 0; col < 8; col++) {

        var button = new Button {
          Size = new Size(PixelSize, PixelSize),
          Location = new Point(col * PixelSize, 0),
          BackColor = Color.White,
          Tag = col, // Store row and column for reference           
          Name = Guid.NewGuid().ToString(),
        };

        button.MouseMove += Button_MouseMove;

        button.Click += PixelButton_Click;

        pixelButtons[col] = button;

        this.Controls.Add(button);
      }

      UpdateRowColors();
    }

    private void Button_MouseMove(object sender, MouseEventArgs e) {

      if (!SketchDrawingEnabled)
        return;

      if (SketchDrawedIds.Contains((sender as Button).Name))
        return;

      SketchDrawedIds.Add((sender as Button).Name);

      PixelButton_Click(sender, e);
    }

    public void UpdateRowColors() {

      for (int i = 0; i < pixelButtons.Length; i++)
        if (pixelColors[i])
          pixelButtons[i].BackColor = TMS9918aPalettes[ForeGroundColor];
        else
          pixelButtons[i].BackColor = TMS9918aPalettes[BackgroundColor];

      ImageUpdated?.Invoke(this, EventArgs.Empty);
    }

    public byte GetColorForRow() {

      int b = 0;

      b += (ForeGroundColor << 4);
      b += BackgroundColor;

      return (byte)b;
    }

    public void SetPatternForRow(byte pattern) {

      for (int i = 0; i < 8; i++) {

        bool isPixelOn = ((pattern >> (7 - i)) & 1) == 1;
        pixelColors[i] = isPixelOn;
      }

      UpdateRowColors();
    }

    public byte GetPatternForRow() {

      int b = 0;

      for (int i = 0; i < pixelColors.Length; i++)
        if (pixelColors[i])
          b += 1 << 7 - i;

      return (byte)b;
    }

    private void PixelButton_Click(object sender, EventArgs e) {

      var button = sender as Button;

      if (button == null)
        return;

      int col = (int)button.Tag;

      pixelColors[col] = !pixelColors[col];

      UpdateRowColors();
    }
  }
}
