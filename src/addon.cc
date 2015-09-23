#include <nan.h>
#include "bgpdump2.h"

static void init(v8::Local<v8::Object> exports) {
  BGPDump2::Init(exports);
}

NODE_MODULE(bgpdump2, init);
