using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace MID2NT {

  public partial class Form1 : Form {

    public Form1() {

      InitializeComponent();
    }

    private void listBox1_DragEnter(object sender, DragEventArgs e) {

      if (e.Data.GetDataPresent(DataFormats.FileDrop))
        e.Effect = DragDropEffects.Copy;
    }

    private void listBox1_DragDrop(object sender, DragEventArgs e) {

      string[] files = (string[])e.Data.GetData(DataFormats.FileDrop);

      addFiles(files);
    }

    void addFiles(string[] files) {

      foreach (var file in files) {

        if (File.Exists(file)) {

          if (!listBox1.Items.Contains(file))
            listBox1.Items.Add(file);
        }

        if (!file.EndsWith(".mid", StringComparison.InvariantCultureIgnoreCase))
          continue;

        if (Directory.Exists(file)) {

          string [] nFiles = Directory.GetFiles(file);

          addFiles(nFiles);
        }
      }
    }

    void log(string txt) {

      textBox1.AppendText(txt);
      textBox1.AppendText(Environment.NewLine);
    }

    private void button1_Click(object sender, EventArgs e) {

      textBox1.Clear();

      foreach (var inFile in listBox1.Items.Cast<string>()) {

        string folder = Path.GetDirectoryName(inFile);
        string filename = Path.GetFileNameWithoutExtension(inFile);

        string outFile = Path.Combine(folder, filename + "-nt.h");

        doFile(inFile, outFile);
      }
    }

    enum NoteCommandEnum {

      NT_SND_CHNG = 0,
      NT_NOTE_OFF = 10,
      NT_NOTE_ON  = 20,
      NT_LOOP     = 99,
    }

    class NoteStruct {

      public int Time;
      public int MidiChannel;
      public NoteCommandEnum Cmd;
      public int[] Data;

      public NoteStruct(int midiChannel, int time, NoteCommandEnum cmd, params int[] data) {

        MidiChannel = midiChannel;
        Time = time;
        Cmd = cmd;
        Data = data;
      }

      public override string ToString() {

        StringBuilder sb = new StringBuilder();

        sb.Append($"{Time}, ");

        sb.Append($"{Cmd}, ");

        foreach (var d in Data)
          sb.Append($"{d}, ");

        return sb.ToString();
      }
    }

    // Channel
    // -------
    // 0) noise (drum)
    // 1) notes
    // 2) notes
    //
    // Commands       Parameters
    // -----          ----------
    // NT_NOTE_OFF    channel
    // NT_NOTE_ON     channel, note, volume (0x00-0x0f)
    // NT_SND_CHNG    TRUE = note | FALSE = noise
    // NT_LOOP        (Note: a loop MUST be placed at the end of your track)

    void doFile(string inFile, string outFile) {

      log($"Processing: {inFile}");

      using (var inStream = File.Open(inFile, FileMode.Open)) {

        var midiFile = new MidiParser.MidiFile(inStream);

        List<NoteStruct> notes = new List<NoteStruct>();

        log($"Ticks per quarter note: {midiFile.TicksPerQuarterNote}");

        foreach (var t in midiFile.Tracks) {

          foreach (var n in t.MidiEvents.Where(r => (r.MidiEventType == MidiParser.MidiEventType.NoteOn || r.MidiEventType == MidiParser.MidiEventType.NoteOff) && r.Note >= 36 && r.Note <= 107)) {

            int trackerNote = (n.Note - 36);

            int trackerChannel = n.Channel - 1;

            if (n.Channel == 10)
              trackerChannel = 0;

            log($"Track: {t.Index}, MidiChannel: {n.Channel}, Tracker Channel: {trackerChannel}, Time: {n.Time}, Event: {n.MidiEventType}, Note: {n.Note}, New: {trackerNote}");

            switch (n.MidiEventType) {

              case MidiParser.MidiEventType.NoteOn: {

                  notes.Add(
                    new NoteStruct(
                      n.Channel,
                      n.Time / (midiFile.TicksPerQuarterNote / 4),
                      NoteCommandEnum.NT_NOTE_ON,
                      trackerChannel,
                      trackerNote,
                      n.Velocity.Map(0, 127, 0, 0x0f)));

                  break;
                }
              case MidiParser.MidiEventType.NoteOff: {

                  notes.Add(
                    new NoteStruct(
                      n.Channel,
                      n.Time / (midiFile.TicksPerQuarterNote / 4),
                      NoteCommandEnum.NT_NOTE_OFF,
                      trackerChannel
                      ));

                  break;
                }
            }

          }
        }

        using (var outSream = File.CreateText(outFile)) {

          string arrayName = Path.GetFileNameWithoutExtension(outFile).Replace(" ", "_").Replace("-", "_");

          outSream.WriteLine($"uint8_t {arrayName}[] = {{");

          var sorted = notes.OrderBy(x => x.Time).ThenBy(x => x.Cmd).ToArray();

          bool noteMode = true;

          for (int i = 0; i < sorted.Length; i++) {

            var note = sorted[i];

            if (i == sorted.Length - 1 || i == 0) {

              if (note.MidiChannel == 10 && noteMode) {

                outSream.WriteLine(new NoteStruct(0, note.Time, NoteCommandEnum.NT_SND_CHNG, 0));

                noteMode = false;
              } else if (note.MidiChannel == 0 && noteMode) {

                outSream.WriteLine(new NoteStruct(0, note.Time, NoteCommandEnum.NT_SND_CHNG, 1));

                noteMode = true;
              }

              outSream.WriteLine(note);

              continue;
            }

            var next = sorted[i + 1];
            var last = sorted[i - 1];

            //if (note.Cmd == NoteCommandEnum.NT_NOTE_OFF &&
            //    next.Cmd == NoteCommandEnum.NT_NOTE_ON &&
            //    next.Time == note.Time &&
            //    next.MidiChannel == note.MidiChannel)
            //  continue;

            if (note.MidiChannel == 10 && noteMode) {

              outSream.WriteLine(new NoteStruct(note.MidiChannel, note.Time, NoteCommandEnum.NT_SND_CHNG, 0));

              noteMode = false;
            } else if (note.MidiChannel == 0 && noteMode) {

              outSream.WriteLine(new NoteStruct(note.MidiChannel, note.Time, NoteCommandEnum.NT_SND_CHNG, 1));

              noteMode = true;
            }

            outSream.WriteLine(note);
          }

          outSream.WriteLine(new NoteStruct(0, sorted[sorted.Length - 1].Time, NoteCommandEnum.NT_LOOP));

          outSream.WriteLine("};");
        }
      }

      log("Done");
    }

  }
}
