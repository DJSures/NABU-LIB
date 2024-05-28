using System;
using System.Text.Json;

namespace Icon_Creator {

  public partial class Form1 : Form {

    UCRow[,] ucRows = new UCRow[2, 16];
    UCColorSelector[,] uCColorSelectors = new UCColorSelector[2, 16];
    Bitmap _bmPreview;

    public Form1() {

      InitializeComponent();

      _bmPreview = new Bitmap(32, 32);
      pictureBox1.Image = _bmPreview;

      this.DoubleBuffered = true;

      int ucRowWidth = 240; // Assuming the width of UCRow for layout
      int ucRowHeight = 30; // Assuming the height of UCRow for layout

      int yOffset = 20;

      for (int x = 0; x < 2; x++) {

        for (int y = 0; y < 16; y++) {

          uCColorSelectors[x, y] = new UCColorSelector {
            Width = 190,
            Height = ucRowHeight, // Assuming height for UCColorSelector
            Location = x == 0 ? new Point(0, (y * ucRowHeight) + yOffset) : new Point(ucRowWidth * 2 + 190, (y * ucRowHeight) + yOffset)
          };
          gbEditor.Controls.Add(uCColorSelectors[x, y]);

          var ucRow = new UCRow {
            Width = ucRowWidth,
            Height = ucRowHeight,
            Tag = new Point(x, y),
            Location = x == 0 ? new Point(uCColorSelectors[x, y].Width, (y * ucRowHeight) + yOffset) : new Point(ucRowWidth + uCColorSelectors[x, y].Width, (y * ucRowHeight) + yOffset)
          };

          ucRow.ImageUpdated += UcRow_ImageUpdated;

          gbEditor.Controls.Add(ucRow);

          ucRows[x, y] = ucRow;

          uCColorSelectors[x, y]._ucRow = ucRow;

          uCColorSelectors[x, y].UpdateColorRows();
        }
      }
    }

    private void UcRow_ImageUpdated(object sender, EventArgs e) {

      UCRow ucRow = (UCRow)sender;

      var pattern = ucRow.GetPatternForRow();
      var colorPattern = ucRow.GetColorForRow();

      var foreGroundColor = (colorPattern >> 4) & 0xF;

      var backgroundColor = colorPattern & 0xF;

      Point p = (Point) ucRow.Tag;

      using (Graphics g = Graphics.FromImage(_bmPreview))
        for (int i = 0; i < 8; i++) {

          bool isPixelOn = ((pattern >> (7 - i)) & 1) == 1;

          _bmPreview.SetPixel((p.X * 8) + i, p.Y, isPixelOn ? UCRow.TMS9918aPalettes[foreGroundColor] : UCRow.TMS9918aPalettes[backgroundColor]);
        }

      pictureBox1.Invalidate();
    }

    private void getPatternJSONStringToolStripMenuItem_Click(object sender, EventArgs e) {

      // PATERN

      var bPattern = new List<byte>();

      for (int x = 0; x < 2; x++)
        for (int y = 0; y < 16; y++) {

          var ucRow = ucRows[x, y];

          var b = ucRow.GetPatternForRow();

          System.Diagnostics.Debug.WriteLine($"0x{b:X02}");

          bPattern.Add(b);
        }

      var bArrayPattern = bPattern.ToArray();

      var strSerializedPattern = JsonSerializer.Serialize(bArrayPattern);

      // COLOR

      var bColor = new List<byte>();

      for (int x = 0; x < 2; x++)
        for (int y = 0; y < 16; y++) {

          var ucRow = ucRows[x, y];

          var b = ucRow.GetColorForRow();

          System.Diagnostics.Debug.WriteLine($"0x{b:X02}");

          bColor.Add(b);
        }

      var bArrayColor = bColor.ToArray();

      var strSerializedColor = JsonSerializer.Serialize(bArrayColor);

      // DO IT

      string s = $"          \"IconTileColor\": {strSerializedColor},\r\n" +
        $"          \"IconTilePattern\": {strSerializedPattern},\r\n";

      Clipboard.SetText(s);

      MessageBox.Show($"{s}  ({bArrayColor.Length})", "Copied to clipboard");
    }

    private void saveToolStripMenuItem_Click(object sender, EventArgs e) {

      try {

        using var sfd = new SaveFileDialog {
          DefaultExt = "nabuIcon", // Note: "DefaultExt" should not include the dot prefix
          Filter = "Nabu Icon files (*.nabuIcon)|*.nabuIcon|All files (*.*)|*.*", // Add a filter for .nabuIcon files
          Title = "Save Nabu Icon"
        };

        if (sfd.ShowDialog() != DialogResult.OK)
          return;

        var pi = new ProjectItem();

        for (int x = 0; x < 2; x++)
          for (int y = 0; y < 16; y++) {

            pi.Patterns.Add(ucRows[x, y].GetPatternForRow());
            pi.Colors.Add(ucRows[x, y].GetColorForRow());
          }

        using (var f = File.Create(sfd.FileName))
          JsonSerializer.Serialize(f, pi, pi.GetType(), new JsonSerializerOptions() { WriteIndented = true });

      } catch (Exception ex) {

        MessageBox.Show($"Failed to save file: {ex.Message}", "Save Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
      }
    }

    private void loadToolStripMenuItem_Click(object sender, EventArgs e) {

      try {

        using var ofd = new OpenFileDialog {
          DefaultExt = "nabuIcon", // Note: "DefaultExt" should not include the dot prefix
          Filter = "Nabu Icon files (*.nabuIcon)|*.nabuIcon|All files (*.*)|*.*", // Add a filter for .nabuIcon files
          Title = "Open Nabu Icon"
        };

        if (ofd.ShowDialog() != DialogResult.OK)
          return;

        ProjectItem pi;

        using (var f = File.OpenRead(ofd.FileName))
          pi = JsonSerializer.Deserialize<ProjectItem>(f);

        int index = 0;
        for (int x = 0; x < 2; x++) {

          for (int y = 0; y < 16; y++) {

            ucRows[x, y].SetPatternForRow(pi.Patterns[index]);

            var colorPattern = pi.Colors[index];

            var foreGroundColor = (colorPattern >> 4) & 0xF;

            var backgroundColor = colorPattern & 0xF;

            uCColorSelectors[x, y].SetColor(foreGroundColor, backgroundColor);

            index++;
          }
        }

      } catch (Exception ex) {

        MessageBox.Show($"Failed to save file: {ex.Message}", "Save Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
      }
    }

    private void nEwToolStripMenuItem_Click(object sender, EventArgs e) {

      if (MessageBox.Show("New?", string.Empty, MessageBoxButtons.YesNo) != DialogResult.Yes)
        return;

      for (int x = 0; x < 2; x++) {

        for (int y = 0; y < 16; y++) {

          ucRows[x, y].SetPatternForRow(0x00);

          uCColorSelectors[x, y].SetColor(15, 1);
        }
      }
    }

    private void getByteArrayToolStripMenuItem_Click(object sender, EventArgs e) {


      // PATERN

      var bPattern = new List<byte>();

      for (int x = 0; x < 2; x++)
        for (int y = 0; y < 16; y++) {

          var ucRow = ucRows[x, y];

          var b = ucRow.GetPatternForRow();

          System.Diagnostics.Debug.WriteLine($"0x{b:X02}");

          bPattern.Add(b);
        }

      // COLOR

      var bColor = new List<byte>();

      for (int x = 0; x < 2; x++)
        for (int y = 0; y < 16; y++) {

          var ucRow = ucRows[x, y];

          var b = ucRow.GetColorForRow();

          System.Diagnostics.Debug.WriteLine($"0x{b:X02}");

          bColor.Add(b);
        }

      // DO IT

      string patternString = String.Join(", ", bPattern.Select(b => $"0x{b:X2}"));
      string colorString = String.Join(", ", bColor.Select(b => $"0x{b:X2}"));

      string s = $"byte[] PatternBytes = new byte[32] {{ {patternString} }}; \r\n" +
        $"byte[] ColorBytes = new byte[32] {{ {colorString} }}; \r\n";

      Clipboard.SetText(s);

      MessageBox.Show($"{s}");
    }

    private void sketchToolStripMenuItem_Click(object sender, EventArgs e) {

      UCRow.SketchDrawedIds.Clear();

      UCRow.SketchDrawingEnabled = !UCRow.SketchDrawingEnabled;

      sketchToolStripMenuItem.Checked = UCRow.SketchDrawingEnabled;
    }
  }
}