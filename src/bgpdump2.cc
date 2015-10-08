#include <iostream>
#include "bgpdump2.h"

extern "C" {
#include "queue.h"
#include "ptree.h"

#include "bgpdump_parse.h"
#include "bgpdump_file.h"
#include "bgpdump_route.h"
#include "bgpdump_query.h"
#include "bgpdump_ptree.h"
#include "bgpdump_peer.h"
#include "bgpdump_peerstat.h"
}

extern "C" {
  uint16_t peer_index;
  int qaf = AF_INET;
  int safi = AF_INET;
  uint32_t timestamp;
  unsigned long autnums[AUTLIM];
  int autsiz = 0;
  uint16_t mrt_type;
  char *progname = NULL;

  struct bgp_route *diff_table[2];
  struct ptree *diff_ptree[2];
  struct ptree *ptree[AF_INET6 + 1];

  void bgpdump_process (char *buf, size_t *data_len) {
    char *p;
    struct mrt_header *h;
    int hsize = sizeof (struct mrt_header);
    char *data_end = buf + *data_len;
    unsigned long len;
    int rest;
    struct mrt_info info;

    p = buf;
    h = (struct mrt_header *) p;
    len = ntohl (h->length);

    /* Process as long as entire MRT message is in the buffer */
    while (len && p + hsize + len <= data_end)
      {
        bgpdump_process_mrt_header (h, &info);

        switch (mrt_type)
          {
          case BGPDUMP_TYPE_TABLE_DUMP_V2:
            bgpdump_process_table_dump_v2 (h, &info, p + hsize + len);
            break;
          default:
            printf ("Not supported: mrt type: %d\n", mrt_type);
            printf ("discarding %lu bytes data.\n", hsize + len);
            break;
          }

        p += hsize + len;

        len = 0;
        if (p + hsize < data_end)
          {
            h = (struct mrt_header *) p;
            len = ntohl (h->length);
          }
      }

    /* move the partial, last-part data
       to the beginning of the buffer. */
    rest = data_end - p;
    if (rest)
      memmove (buf, p, rest);
    *data_len = rest;
  }

  bgp_route* ptree_query2 (struct ptree *ptree, struct query *query_table, uint64_t query_size) {
    int i;
    struct ptree_node *x;

    for (i = 0; i < query_size; i++)
      {
        char *query = query_table[i].destination;
        char *answer = query_table[i].nexthop;
        int plen = (qaf == AF_INET ? 32 : 128);
        x = ptree_search (query, plen, ptree);
        if (x)
          {
            struct bgp_route *route = (bgp_route *)x->data;
            memcpy (answer, route->nexthop, MAX_ADDR_LENGTH);
            return route;
          }
        else
          {
            char buf[64];
            inet_ntop (qaf, query, buf, sizeof (buf));
            printf ("%s: no route found.\n", buf);
            return NULL;
          }
      }
  }
}

using namespace v8;

BGPDump2::BGPDump2(const std::string path) : mrtPath(path) {
  extern unsigned long long nroutes;
  nroutes = resolv_number (ROUTE_LIMIT_DEFAULT, NULL);
}

BGPDump2::~BGPDump2() {
}

void BGPDump2::Init(Local<Object> exports) {
  Nan::HandleScope scope;
  Local<FunctionTemplate> t = Nan::New<FunctionTemplate>(New);

  t->InstanceTemplate()->SetInternalFieldCount(1);

  Nan::SetPrototypeMethod(t, "lookup", Lookup);

  exports->Set(Nan::New("BGPDump2").ToLocalChecked(), t->GetFunction());
}

void BGPDump2::New(const Nan::FunctionCallbackInfo<v8::Value>& info) {
  if(info.Length()!=1 || !info[0]->IsString()) {
    Nan::ThrowError(Nan::TypeError("Wrong number of arguments"));
    return;
  }

  BGPDump2 *bgpdump = new BGPDump2(std::string(*String::Utf8Value(info[0]->ToString())));
  bgpdump->Wrap(info.This());
  info.GetReturnValue().Set(info.This());
}

#include <vector>

void BGPDump2::Lookup(const Nan::FunctionCallbackInfo<v8::Value>& info) {
  BGPDump2* bgpdump = ObjectWrap::Unwrap<BGPDump2>(info.Holder());

  std::vector<std::string> qprefixes;

  switch(info.Length()) {
  case 0:
    Nan::ThrowError(Nan::TypeError("Wrong number of arguments"));
    return;
  case 1:
    if(info[0]->IsArray()) {
      Handle<Array> array = Handle<Array>::Cast(info[0]);

      for (int i = 0; i < array->Length(); i++) {
        if(!array->Get(i)->IsString()) {
          Nan::ThrowError(Nan::TypeError("Unknown type of arguments"));
          return;
        }
        qprefixes.push_back(std::string(*String::Utf8Value(array->Get(i)->ToString())));
      }
    } else if(!info[0]->IsString()) {
      Nan::ThrowError(Nan::TypeError("Unknown type of arguments"));
      return;
    } else {
      qprefixes.push_back(std::string(*String::Utf8Value(info[0]->ToString())));
    }
    break;
  default:
    for (int i = 0; i < info.Length(); i++) {
      if(!info[i]->IsString()) {
        Nan::ThrowError(Nan::TypeError("Unknown type of arguments"));
        return;
      }
      qprefixes.push_back(std::string(*String::Utf8Value(info[i]->ToString())));
    }
    break;
  }

  extern int lookup;
  lookup = 1;

  char filename[256];
  sprintf(filename, "%s", bgpdump->mrtPath.c_str());
  unsigned long long bufsiz = resolv_number (BGPDUMP_BUFSIZ_DEFAULT, NULL);

  char *buf;
  buf = (char*) malloc (bufsiz);
  if (! buf)
    {
      printf ("can't malloc %lluB-size buf: %s\n",
              bufsiz, strerror (errno));
      exit (-1);
    }

  peer_table_init ();
  route_init ();
  extern int route_size;
  route_size = 0;
  ptree[AF_INET] = ptree_create ();
  ptree[AF_INET6] = ptree_create ();

  file_format_t format;
  struct access_method *method;
  void *file;
  size_t ret;

  format = get_file_format (filename);
  method = get_access_method (format);
  file = method->fopen (filename, "r");
  if (! file)
    fprintf (stderr, "# could not open file: %s\n", filename);

  size_t datalen = 0;

  while (1) {
    ret = method->fread (buf + datalen, bufsiz - datalen, 1, file);
    datalen += ret;

    /* end of file. */
    if (ret == 0 && method->feof (file))
      break;

    bgpdump_process(buf, &datalen);
  }

  if (datalen)
    printf ("warning: %lu bytes unprocessed data remains: %s\n", datalen, filename);

  method->fclose (file);

  /* For each end of the processing of files. */


  /* query_table construction. */
  char prefix[256];
  extern uint64_t query_size;
  query_limit = 1;

  if(qprefixes.size()==1) {
    query_size = 0;
    query_init ();
    strcpy(prefix, qprefixes.at(0).c_str());

    query_addr (prefix);
    bgp_route* route = ptree_query2 (ptree[qaf], query_table, query_size);

    if(route==NULL)
      info.GetReturnValue().SetNull();
    else
      info.GetReturnValue().Set(CreateObject(route));
  } else {
    Local<Array> result = Nan::New<Array>(qprefixes.size());
    for(int i = 0; i < qprefixes.size(); i++) {
      query_size = 0;
      query_init ();
      strcpy(prefix, qprefixes.at(i).c_str());

      query_addr (prefix);
      bgp_route* route = ptree_query2 (ptree[qaf], query_table, query_size);

      if(route==NULL)
        result->Set(i, Nan::Null());
      else
        result->Set(i, CreateObject(route));
    }

    info.GetReturnValue().Set(result);
  }

  free (query_table);
  ptree_delete (ptree[AF_INET]);
  ptree_delete (ptree[AF_INET6]);
  route_finish ();

  free (buf);
}

Local<Object> BGPDump2::CreateObject(bgp_route* route) {
  Local<Object> obj = Nan::New<Object>();

  char buf[64], buf2[64], buf3[68];
  inet_ntop (qaf, route->prefix, buf, sizeof (buf));
  sprintf(buf3, "%s/%d", buf, route->prefix_length);
  obj->Set(Nan::New("prefix").ToLocalChecked(), Nan::New<String>(buf3).ToLocalChecked());

  inet_ntop (qaf, route->nexthop, buf2, sizeof (buf2));
  Local<String> nexthop = Nan::New<String>(buf2).ToLocalChecked();
  obj->Set(Nan::New("nexthop").ToLocalChecked(), nexthop);

  obj->Set(Nan::New("origin_as").ToLocalChecked(), Nan::New<Integer>(route->origin_as));

  char buf4[10];
  sprintf (buf4, "%lu", (unsigned long) route->path_list[0]);
  std::string as_path(buf4);

  for (int i = 1; i < route->path_size; i++) {
    sprintf (buf4, " %lu", (unsigned long) route->path_list[i]);
    as_path += std::string(buf4);
  }
  obj->Set(Nan::New("as_path").ToLocalChecked(), Nan::New<String>(as_path).ToLocalChecked());

  return obj;
}
