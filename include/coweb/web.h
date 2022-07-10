#pragma once

#include "co/all.h"
#include "coweb/http1.h"
#include "coweb/ws.h"
#include "coweb/route.h"
#include <functional>
namespace web {
typedef bool (*web_callback)(REQ* req);
class __coapi Server {
   public:
    bool isUTF8 = true;  //默认自动转码utf8
    ROUTE route;
    Server();
    ~Server();
    // Server& add_url(fastring url, std::function <bool(const REQ &) > );
    


    /**
     * start a http server
     *   - It will not block the calling thread.
     *
     * @param ip    server ip, either an ipv4 or ipv6 address, default: "0.0.0.0".
     * @param port  server port, default: 80.
     */
    void start(const char* ip = "0.0.0.0", int port = 80);
    void hook_before_req(web_callback p) { before_req = p; };
    void hook_weberr(web_callback p) { weberr = p; };
    void hook_on_connected(web_callback p) { connected = p; };
    void hook_on_closed(web_callback p) { closed = p; };

    /**
     * start a https server
     *   - openssl required by this method.
     *   - It will not block the calling thread.
     *
     * @param ip    server ip, either an ipv4 or ipv6 address.
     * @param port  server port.
     * @param key   path of the private key file for ssl.
     * @param ca    path of the certificate file for ssl.
     */
    void start(const char* ip, int port, const char* key, const char* ca);

    /**
     * exit the server gracefully
     *   - Once `exit()` was called, the listening socket will be closed, and new
     *     connections will not be accepted. Since co v3.0, the server will reset
     *     previously established connections.
     */
    void exit();

   private:

    void* _p;
    web_callback before_req = NULL;
    web_callback weberr = NULL;
    web_callback connected   = NULL;
    web_callback closed  = NULL;
    DISALLOW_COPY_AND_ASSIGN(Server);

        bool on_connect(REQ* req);
    bool on_header(REQ* req);
    bool on_body(REQ* req);
    bool on_close(REQ* req);
    bool on_err(REQ* req);
    bool on_wsupgrade(REQ* req);
   
    bool on_wsbody(ws::REQ* req,void* callback_vector);
};
}  // namespace web
