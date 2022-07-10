#include "./http.h"
#include "co/http.h"
#include "co/tcp.h"
#include "co/co.h"
#include "co/god.h"
#include "co/fastream.h"
#include "co/stl.h"
#include "co/time.h"
#include "co/fs.h"
#include "co/path.h"
#include "co/lru_map.h"
#include "co/all.h"
#include "coweb/web.h"
#include "coweb/Encodec.h"
#include "picohttpparser/picohttpparser.h"

DEF_uint32(web_max_header_size, 4096, ">>#2 max size of http header");
DEF_uint32(web_max_body_size, 8 << 20, ">>#2 max size of http body, default: 8M");
DEF_uint32(web_timeout, 3000, ">>#2 send or recv timeout in ms for http client");
DEF_uint32(web_conn_timeout, 3000, ">>#2 connect timeout in ms for http client");
DEF_uint32(web_recv_timeout, 60000, ">>#2 recv timeout in ms for http server");
DEF_uint32(web_send_timeout, 3000, ">>#2 send timeout in ms for http server");
DEF_uint32(web_conn_idle_sec, 180, ">>#2 if a connection was idle for this seconds, the server may reset it");
DEF_uint32(web_max_idle_conn, 128, ">>#2 max idle connections for http server");
DEF_bool(web_log, true, ">>#2 enable http server log if true");

#define WEBLOG LOG_IF(FLG_web_log)

namespace web {
/**
 * ===========================================================================
 * HTTP server
 *   - openssl required for https.
 * ===========================================================================
 */
// @x  beginning of http header

#define web_CALLBACK std::function<bool(REQ*)>

class ServerImpl {
   public:
    ServerImpl() : _started(false), _stopped(false) {}
    ~ServerImpl() = default;

    void on_connect(web_CALLBACK&& f) { _on_connect = std::move(f); }
    void on_header(web_CALLBACK&& f) { _on_header = std::move(f); }
    void on_body(web_CALLBACK&& f) { _on_body = std::move(f); }
    void on_close(web_CALLBACK&& f) { _on_close = std::move(f); }
    void on_err(web_CALLBACK&& f) { _on_err = std::move(f); }
    void on_wsupgrade(web_CALLBACK&& f) { _on_wsupgrade = std::move(f); }

    void on_wsbody(std::function<bool(ws::REQ*,void*)>&& f) { _on_wsbody = std::move(f); }
    void start(const char* ip, int port, const char* key, const char* ca);

    void on_connection(tcp::Connection conn);

    void exit() {
        atomic_store(&_stopped, true);
        _serv.exit();
    }
    bool started() const { return _started; }

   private:
    bool                          _started;
    bool                          _stopped;
    tcp::Server                   _serv;
    web_CALLBACK                  _on_connect;
    web_CALLBACK                  _on_header;
    web_CALLBACK                  _on_body;
    web_CALLBACK                  _on_close;
    web_CALLBACK                  _on_err;
    web_CALLBACK                  _on_wsupgrade;
    std::function<bool(ws::REQ*,void*)> _on_wsbody;
};

Server::Server() {
    _p = co::make<ServerImpl>();

    ((ServerImpl*)_p)->on_connect(std::bind(&Server::on_connect, this, std::placeholders::_1));
    ((ServerImpl*)_p)->on_header(std::bind(&Server::on_header, this, std::placeholders::_1));
    ((ServerImpl*)_p)->on_body(std::bind(&Server::on_body, this, std::placeholders::_1));
    ((ServerImpl*)_p)->on_close(std::bind(&Server::on_close, this, std::placeholders::_1));
    ((ServerImpl*)_p)->on_err(std::bind(&Server::on_err, this, std::placeholders::_1));
    
    ((ServerImpl*)_p)->on_wsupgrade(std::bind(&Server::on_wsupgrade, this, std::placeholders::_1));
    ((ServerImpl*)_p)->on_wsbody(std::bind(&Server::on_wsbody, this, std::placeholders::_1, std::placeholders::_2));

}

Server::~Server() {
    if (_p) {
        auto p = (ServerImpl*)_p;
        if (!p->started()) co::del(p);
        _p = 0;
    }
}

bool Server::on_connect(REQ* req) {

    WEBLOG << "on_connect：" << req->ip;
    if (connected) {
        if (connected(req) == false) return false;
    }

    return true;
}
bool Server::on_header(REQ* req) {

    fastring path    = req->req.get("path").as_string();
    auto     method  = req->req.get("method").as_c_str();
    auto     fn      = route.get(path, method);
    auto     fn_size = fn.size();
    if (fn_size<=0)
    {
        fn = route.get_pcre(path, method);  //正则路由
    }
    if (fn_size <= 0) {
        return false;
    } else {
        req->p_callback_vector = new co::vector<rut_callback>(fn);
    }

    WEBLOG << "on_header：" << req->req.pretty();
    return true;
}

bool Server::on_body(REQ* req) {

    if (req->p_callback_vector==NULL)
    {
        return false;
    }
    co::vector<rut_callback> fn = *(co::vector<rut_callback>*)(req->p_callback_vector);
    delete (co::vector<rut_callback>*)(req->p_callback_vector);
    req->p_callback_vector      = NULL;
    auto                     fn_size = fn.size() ;
    if (fn_size <= 0) return false;


    Json query = parse_form_query(req->req, isUTF8);
    req->req.set("query", query);

    fastring type = req->req.get("content-type").as_string().tolower();
    if (type.find("application/json") != type.npos) {
        Json json;
        if (isUTF8) {
            req->body_bin = Encode::GBKToUTF8(req->body_bin.c_str());
        }
        json.parse_from(req->body_bin);
        req->req.set("body", json);
    } else if (type.find("multipart/form-data") != type.npos) {
        auto json = parse_form_data(req->body_bin, isUTF8);
        req->req.set("body", json);
    } else if (type.find("form-urlencoded") != type.npos) {
        auto json = parse_form_urlencoded(req->body_bin, isUTF8);
        req->req.set("body", json);
    }
    
    WEBLOG << "on_bod：" << req->req.pretty();
    //请求前处理hook
    if (before_req) {
        if (before_req(req)==false)
        {
            return true;
        }
    }
    for (size_t i = 0; i < fn_size; i++) {
       if( fn[i](req)==false) {
       
           return true;
       }
    }
    return true;
}
bool Server::on_close(REQ* req) {
    WEBLOG << "on_close：" << req->ip;
    if (req->p_callback_vector != NULL) {
        delete (co::vector<rut_callback>*)(req->p_callback_vector);
        req->p_callback_vector = NULL;
    }

     if (req->p_wscalback_vector != NULL) {
        delete (co::vector<ws_rut_callback>*)(req->p_wscalback_vector);
        req->p_wscalback_vector = NULL;
    }

    if (closed) closed(req);
    return true;
}
bool Server::on_err(REQ* req) {
    WEBLOG << "on_err：" << req->err;
    if (weberr) weberr(req);
    return true;
}
bool Server::on_wsupgrade(REQ* req) {
    WEBLOG << "on_wsupgrade：" << req->ip;
    fastring path    = req->req.get("path").as_string();
    auto     method  = req->req.get("method").as_c_str();
    auto     fn      = route.get_ws(path, method);
    auto     fn_size = fn.size();
    if (fn_size <= 0) {
        return false;
    }
    req->p_wscalback_vector = new co::vector<ws_rut_callback>(fn);
    return true;
}

bool Server::on_wsbody(ws::REQ* req, void* callback_vector) {
    WEBLOG << "on_wsbody:" << req->body;
    if (!callback_vector)
    {
        return false;
    }
    co::vector<ws_rut_callback> fn = *(co::vector<ws_rut_callback>*)(callback_vector);
    auto                        fn_size = fn.size();
    for (size_t i = 0; i < fn_size; i++) {
        if (fn[i](req) == false) {
            return true;
        }
    }

   // req->send_msg("coweb-ws", ws::opcode::text);
    return true;
}

void Server::start(const char* ip, int port) { ((ServerImpl*)_p)->start(ip, port, NULL, NULL); }

void Server::start(const char* ip, int port, const char* key, const char* ca) { ((ServerImpl*)_p)->start(ip, port, key, ca); }

void Server::exit() { ((ServerImpl*)_p)->exit(); }

void ServerImpl::start(const char* ip, int port, const char* key, const char* ca) {
    CHECK(_on_header != NULL) << "req callback not set..";
    atomic_store(&_started, true, mo_relaxed);
    _serv.on_connection(&ServerImpl::on_connection, this);
    _serv.on_exit([this]() { co::del(this); });
    _serv.start(ip, port, key, ca);
}

inline int hex2int(char c) {
    if ('0' <= c && c <= '9') return c - '0';
    if ('a' <= c && c <= 'f') return c - 'a' + 10;
    if ('A' <= c && c <= 'F') return c - 'A' + 10;
    return -1;
}

void send_error_message(int err, Json* res, void* conn) {}

void ServerImpl::on_connection(tcp::Connection conn) {
    char     c;
    int      r   = 0;
    size_t   pos = 0;
    fastring buf;
    Json     res;
    REQ      req;
    bool     keep_alive = false, isupgade = false;
    req.ip = co::peer(conn.socket());  // get ip
    req.set(conn);
    if (_on_connect(&req) == false) goto closed;
    god::bless_no_bugs();
    while (true) {
        { /* recv http header and body */
        recv_beg:
            if (buf.capacity() == 0) {
                // try to recieve a single byte
                r = conn.recv(&c, 1, FLG_web_conn_idle_sec * 1000);
                if (r == 0) goto recv_zero_err;
                if (r < 0) {
                    if (!co::timeout()) goto recv_err;
                    if (_stopped) {
                        conn.reset();
                        goto end;
                    }  // server stopped
                    if (_serv.conn_num() > FLG_web_max_idle_conn) goto idle_err;
                    goto recv_beg;
                }
                buf.reserve(4096);
                buf.append(c);
            }
            while (true) {
                if (buf.size() > FLG_web_max_header_size) goto header_too_long_err;
                if (buf.capacity() - buf.size() < 4096)  //剩余空间不足4k
                    buf.reserve(buf.size() + 4096);
                r = conn.recv((void*)(buf.data() + buf.size()), (int)(buf.capacity() - buf.size()), keep_alive ? -1 : FLG_web_recv_timeout);
                if (r == 0) goto recv_zero_err;
                if (r < 0) {
                    if (!co::timeout()) goto recv_err;
                    if (_serv.conn_num() > FLG_web_max_idle_conn) goto idle_err;
                    if (buf.empty()) {
                        buf.reset();
                        goto recv_beg;
                    }
                    goto recv_err;
                }
                buf.resize(buf.size() + r);
                int reqlen = parse_web_headers(&buf, &req.req);
                if (reqlen > 0) {
                   bool route_ok= _on_header(&req);  //请求头解析完成
                    fastring Connection = req.req.get("connection").as_string();
                    Connection.tolower();
                    if (Connection.size() == 10 && Connection.starts_with('k'))  // keep-alive
                    {
                        keep_alive = true;
                        WEBLOG << "当前为长连接";
                    } 
                    isupgade = (Connection.size() == 7 && Connection.starts_with('u'));  // upgrade
                    //路由失败
                    if (!route_ok && !isupgade && !keep_alive) goto closed;
                    if (!route_ok && keep_alive) {
                        req.send_msg("route err", 404, false);
                    }
                    int body_len = req.req.get("content-length").as_int();
                    while (body_len > 0) {
                        if ((buf.size() - reqlen) >= body_len) {
                            fastring body;
                            body.append(buf.data() + reqlen, body_len);
                            req.body_bin = (fastring &&) body;
                            break;
                            // WEBLOG << "body接收完成";
                        }
                        auto p   = buf.data() + buf.size();
                        auto sub = reqlen + body_len - buf.size();
                        if (buf.capacity() - buf.size() < sub)  //空间不足
                            buf.reserve(buf.size() + sub);
                        r = conn.recv((void*)(p), (int)(sub), keep_alive ? -1 : FLG_web_recv_timeout);
                        if (r == 0) goto recv_zero_err;
                        if (r < 0) {
                            if (!co::timeout()) goto recv_err;
                            if (_serv.conn_num() > FLG_web_max_idle_conn) goto idle_err;
                            if (buf.empty()) {
                                buf.reset();
                                goto recv_beg;
                            }
                            goto recv_err;
                        }
                        buf.resize(buf.size() + r);
                    }
                    //分块
                    {
                        bool                       ischunk = req.req.has_member("transfer-encoding");  //分块
                        struct phr_chunked_decoder decoder = {};                                       /* zero-clear */
                        size_t                     size    = reqlen, rsize;
                        decoder.consume_trailer            = 1;
                        while (ischunk) {
                            rsize    = buf.size();
                            int pret = phr_decode_chunked(&decoder, (char*)buf.data() + size, &rsize);
                            if (pret == -1) goto chunk_err;
                            size += rsize;
                            buf.resize(size);
                            if (pret >= 0) {
                                fastring body;
                                body.append(buf.data() + reqlen, buf.size() - reqlen);
                                req.body_bin = (fastring &&) body;
                                WEBLOG << "chunk接收完成";
                                break;
                            }
                            //继续接收
                            buf.reserve(buf.size() + 4096);
                            r = conn.recv((void*)(buf.data() + buf.size()), (int)(buf.capacity() - buf.size()), FLG_web_recv_timeout);
                            if (r == 0) goto recv_zero_err;
                            if (r < 0) {
                                if (!co::timeout()) goto recv_err;
                                if (_serv.conn_num() > FLG_web_max_idle_conn) goto idle_err;
                                if (buf.empty()) {
                                    buf.reset();
                                    goto recv_beg;
                                }
                                goto recv_err;
                            }
                            buf.resize(buf.size() + r);
                        }
                    }
                    // websocket或长连接
                    {
                        if (isupgade) {
                            fastring Upgrade = req.req.get("upgrade").as_string();
                            if (Upgrade.starts_with('w') || Upgrade.starts_with('W')) {
                                WEBLOG << "ws升级事件";
                                if (_on_wsupgrade(&req) == false) goto closed;
                                fastring sendbuf;
                                ws::upgrade_to_websocket(req.req, sendbuf);
                                conn.send(sendbuf.data(), sendbuf.size());  //响应升级
                                buf.reserve(0);
                                switch (ws::handle(conn,req.p_wscalback_vector, _on_wsbody)) {
                                    default: break;
                                    case -1: goto closed;
                                };
                            }
                        }
                    }
                    if (route_ok) {
                        _on_body(&req);
                    }
                    
                    buf.resize(0);
                    req.body_bin.clear();
                    req.req.reset();
                    req.err.clear();
                    if (!keep_alive) {
                        co::sleep(60000);  //等待60秒响应数据
                        goto closed;
                    }
                } else if (reqlen == -1) {
                    goto recv_zero_err;
                }
            }
        };
    }

recv_zero_err:
    req.err << "http client close the connection: " << co::peer(conn.socket());
    goto end;
idle_err:
    req.err << "http close idle ";
    _on_err(&req);
    conn.reset();
    goto end;
header_too_long_err:
    req.err = "http recv error: header too long";
    _on_err(&req);
    goto reset_conn;
recv_err:
    req.err = "http recv error ";
    _on_err(&req);
    goto reset_conn;
chunk_err:
    req.err = "http invalid chunked data..";
    _on_err(&req);
    goto reset_conn;
reset_conn:
    conn.reset(3000);
    goto end;
closed:
    conn.reset(3000);
    goto end;
end:
    _on_close(&req);
    god::bless_no_bugs();
}
}  // namespace web

// so

#undef WEBLOG