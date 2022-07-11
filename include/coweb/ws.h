#pragma once
#include "co/hash/base64.h"
#include "co/json.h"
#include "co/tcp.h"
#include "ws_define.h"
#include <mutex>

namespace ws {
typedef struct WSHEADER {
    size_t payload_prt_         = 0;
    size_t payload_length_      = 0;
    size_t left_payload_length_ = 0;

    size_t        left_header_len_ = 0;
    unsigned int  mask_            = 0;
    unsigned char msg_opcode_      = 0;
    unsigned char msg_fin_         = 0;

} ws_header;

class REQ {
   public:
    fastring      body;
    ws_frame_type code_ = ws_frame_type::WS_ERROR_FRAME;
    REQ();
    ~REQ();
    void set(tcp::Connection& conn) { conn_ = &conn; }

    void send_msg(fastring msg, opcode code = ws::opcode::text);

   private:
    tcp::Connection* conn_ = 0;
    fastring         sendbuf;
    std::mutex       mtx;
    char             exit = 1;
};

void          upgrade_to_websocket(Json& req, fastring& buf);
ws_frame_type parse_payload(const char* buf, size_t size, ws_header& h, fastring& outbuf);
int           parse_header(const char* buf, size_t size, ws_header& h);
int           handle(tcp::Connection& conn,void * callback_vector, std::function<bool(ws::REQ*, void*)> _on_wsbody);
fastring      encode_msg(fastring msg, opcode code = text);
}  // namespace ws
