#pragma once

#include "co/all.h"
#include "coweb/http1.h"
#include "coweb/ws.h"
#include <functional>
namespace web
{
    class __coapi Server
    {
    public:
        bool isUTF8 = true; //默认自动转码utf8
        Server();
        ~Server();
        // Server& add_url(fastring url, std::function <bool(const REQ &) > );

        bool on_connect(REQ* req);
        bool on_header(REQ* req);
        bool on_body(REQ* req);
        bool on_close(REQ* req);
        bool on_err(REQ* req);
        bool on_wsbody(ws::REQ* req);

        /**
         * start a http server
         *   - It will not block the calling thread.
         *
         * @param ip    server ip, either an ipv4 or ipv6 address, default: "0.0.0.0".
         * @param port  server port, default: 80.
         */
        void start(const char* ip = "0.0.0.0", int port = 80);

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

        DISALLOW_COPY_AND_ASSIGN(Server);
    };
} // namespace web
