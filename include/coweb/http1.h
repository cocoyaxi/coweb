#pragma once
#include <mutex>
#include "co/hash/base64.h"
#include "co/json.h"
#include "co/tcp.h"
#include "ws_define.h"
namespace web {
#define max_headers 100

class REQ {
   public:
    fastring body_bin;
    fastring ip;
    fastring err;
    Json     req;
    REQ();
    ~REQ();
    void set(tcp::Connection& conn) { conn_ = &conn; }
    void send_msg(fastring& body, int code = 200, bool utf8 = true);
    void send_msg(Json& body, int code = 200, bool utf8 = true);
    void send(fastring msg);

   private:
    tcp::Connection* conn_ = 0;
    fastring         sendbuf;
    std::mutex       mtx;
    char             exit = 1;
    //		REQ(tcp::Connection* _conn_, fastring _sendbuf, std::mutex _mtx)
    //: conn_(_conn_),sendbuf(_sendbuf),mtx(_mtx)
    //{}
};
class RES {
   public:
    fastring version = "HTTP/1.1";
    fastring body;
    int      status = 200;
    RES(){};
    ~RES(){};
    void     add_header(fastring k, fastring v);
    fastring get(void);

   private:
    fastring header;
};
int  parse_web_headers(fastring* buf, Json* req);
Json parse_form_data(fastring& body, bool utf8 = true);
}  // namespace web
