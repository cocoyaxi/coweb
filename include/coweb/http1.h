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
    fastring   sendbuf;
    std::mutex mtx;
    fastring body_bin;
    std::atomic_bool exit = false;
    std::atomic_int  count = 0;
    
    int socket = 0;

    fastring ip;
    fastring err;
    Json     req;
    void*    p_callback_vector=NULL;//callback向量
    void*    p_wscalback_vector = NULL;//ws回调向量
    REQ();
    ~REQ();
   // void set(tcp::Connection& conn) { conn_ = &conn; }
    void send_msg(const char* body, int code = 200, bool utf8 = true);
    void send_msg(fastring& body, int code, bool utf8) { 
        send_msg(body.c_str(), code, utf8);
    }
    void send_msg(Json& body, int code = 200, bool utf8 = true);
    void send(fastring msg);

   private:
    
    
    
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
Json parse_form_query(Json& json, bool isUTF8);
Json parse_form_urlencoded(fastring& body, bool isUTF8);
}  // namespace web
