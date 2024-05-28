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

  public partial class UCColorSelector : UserControl {

    public UCRow _ucRow { get; set; } = new UCRow();

    public UCColorSelector() {

      InitializeComponent();

      foreach (var color in UCRow.TMS9918aPalettes) {

        cbBackgroundColor.Items.Add(color);
        cbForegroundColor.Items.Add(color);
      }

      cbBackgroundColor.SelectedIndex = 1;
      cbForegroundColor.SelectedIndex = 15;
    }

    public void UpdateColorRows() {

      _ucRow.ForeGroundColor = cbForegroundColor.SelectedIndex;
      _ucRow.BackgroundColor = cbBackgroundColor.SelectedIndex;
      _ucRow.UpdateRowColors();
    }

    private void cbForegroundColor_SelectedIndexChanged(object sender, EventArgs e) {

      _ucRow.ForeGroundColor = cbForegroundColor.SelectedIndex;
      _ucRow.UpdateRowColors();
    }

    private void cbBackgroundColor_SelectedIndexChanged(object sender, EventArgs e) {

      _ucRow.BackgroundColor = cbBackgroundColor.SelectedIndex;
      _ucRow.UpdateRowColors();
    }

    public void SetColor(int foreGround, int backGround) {

      cbForegroundColor.SelectedIndex = foreGround;
      cbBackgroundColor.SelectedIndex = backGround; 

      _ucRow.UpdateRowColors();
    }
  }
}
