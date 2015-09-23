#include <iostream>
#ifndef BGPDUMP2_H
#define BGPDUMP2_H

#include <nan.h>

extern "C" {
#include "bgpdump_data.h"
#include "bgpdump_route.h"
}

#define BGPDUMP_BUFSIZ_DEFAULT "16MiB"

class BGPDump2 : public Nan::ObjectWrap {
 public:
  std::string mrtPath;
  static void Init(v8::Local<v8::Object> exports);

 private:
  BGPDump2(const std::string mrtPath);
  ~BGPDump2();

  static void New(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void Lookup(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static v8::Local<v8::Object> CreateObject(bgp_route* route);
};

#endif
