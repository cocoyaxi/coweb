#include "co/flag.h"
#include "co/http.h"
#include "co/log.h"
#include "co/time.h"
#include "coweb/web.h"
#include "coweb/route.h"
DEF_string(ip, "0.0.0.0", "http server ip");
DEF_int32(port, 80, "http server port");
DEF_string(key, "", "private key file");
DEF_string(ca, "", "certificate file");

bool fn1(web::REQ*req) {
    DLOG << "fn1 "
         << "检查通过";
    return true;
}
bool fn2(web::REQ*req) {
    DLOG << "fn2 "
         << "检查不通过";
    req->send_msg("<p>一段文本<\p>", 200);
    return false;
}
bool fn3(web::REQ*req) {
    DLOG << "fn3"
         << "不会被执行";
    return true;
}
bool weberr(web::REQ* req) {
    DLOG << "err hook";
    return true;
}
bool before_req(web::REQ* req) {
    DLOG << "before_request hook";
    return true;
}
bool connected(web::REQ* req) {
    DLOG << "connected hook";
    return true;
}
bool closed(web::REQ* req) {
    DLOG << "closed hook";
    return true;
}

bool ws_fn1(ws::REQ* req) {
    DLOG << "ws_fn1";
    req->send_msg("ws_fn1");
    return true;
}
bool ws_fn2(ws::REQ* req) {
    DLOG << "ws_fn2";
    req->send_msg("ws_fn2");
    return true;
}
bool ws_fn3(ws::REQ* req) {
    DLOG << "ws_fn3";
    req->send_msg("ws_fn3");
    return true;
}
int main(int argc, char** argv) {
    flag::init(argc, argv);
    FLG_cout = true;
    web::Server w;
    w.route.reg("/", {"GET", "POST"}, {fn1, fn2,fn3});
    w.hook_before_req(before_req);
    w.hook_weberr(weberr);
    w.hook_on_connected(connected);
    w.hook_on_closed(closed);
    //websocket
    w.route.reg_ws("/ws", {"GET", "POST"}, {ws_fn1,ws_fn2,ws_fn3});
    if (!FLG_key.empty() && !FLG_ca.empty()) {
        if (FLG_port == 80) FLG_port = 443;
    }
    w.start(FLG_ip.c_str(), FLG_port, FLG_key.c_str(), FLG_ca.c_str());

    while (true) sleep::sec(1024);
    return 0;
}