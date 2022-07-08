#include "co/flag.h"
#include "co/http.h"
#include "co/log.h"
#include "co/time.h"
#include "coweb/web.h"

DEF_string(ip, "0.0.0.0", "http server ip");
DEF_int32(port, 80, "http server port");
DEF_string(key, "", "private key file");
DEF_string(ca, "", "certificate file");

int main(int argc, char** argv) {
    flag::init(argc, argv);
    FLG_cout = true;
    if (!FLG_key.empty() && !FLG_ca.empty()) {
        if (FLG_port == 80) FLG_port = 443;
    }
    web::Server().start(FLG_ip.c_str(), FLG_port, FLG_key.c_str(), FLG_ca.c_str());

    while (true) sleep::sec(1024);
    return 0;
}