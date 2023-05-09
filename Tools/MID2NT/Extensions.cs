using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace MID2NT {

  public static class Extensions {

    public static int Map(this int value, int fromLow, int fromHigh, int toLow, int toHigh) {

      return (value - fromLow) * (toHigh - toLow) / (fromHigh - fromLow) + toLow;
    }
  }
}
