#include "trace.h"

namespace kagami {
  void AppendMessage(string msg, StateLevel level, 
    StandardLogger *std_logger, size_t index) {
    string buf;

    if (index != 0) {
      buf.append("(Line:" + to_string(index) + ")");
    }

    switch (level) {
    case kStateError:  buf.append("Error:"); break;
    case kStateWarning:buf.append("Warning:"); break;
    case kStateNormal: buf.append("Info:"); break;
    default:break;
    }

    buf.append(msg);
    std_logger->WriteLine(buf);
  }

  void AppendMessage(string msg, StandardLogger *std_logger) {
    std_logger->WriteLine(msg);
  }
}