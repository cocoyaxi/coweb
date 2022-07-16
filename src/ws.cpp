#include "coweb/ws.h"
#include "coweb/sha1.hpp"
#include "coweb/web.h"

namespace ws {
void SHA1(fastring& key_src, uint8_t* sha1buf) {
    web::sha1_context ctx;
    web::init(ctx);
    web::update(ctx, key_src.data(), key_src.size());
    web::finish(ctx, sha1buf);
}




void REQ::send_msg(fastring msg, opcode code) {
    fastring buf = encode_msg(msg, code);
    _p->send(buf);
}
void upgrade_to_websocket(Json& req, fastring& buf ) {
    fastring header;
    uint8_t  sha1buf[20];

    header << "HTTP/1.1 101 Switching Protocols\r\n";
    header << "Upgrade"
           << ": "
           << "WebSocket"
           << "\r\n";
    header << "Connection"
           << ": "
           << "Upgrade"
           << "\r\n";

    auto key_src = req.get("sec-websocket-key").as_string();
    key_src.append("258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
    SHA1(key_src, sha1buf);
    auto accept_key = base64_encode(sha1buf, sizeof(sha1buf));

    header << "Sec-WebSocket-Accept"
           << ": " << accept_key << "\r\n";

    if (req.has_member("sec-websocket-protocol")) {
        auto protocal_str = req.get("sec-websocket-protocol").as_string();
        header << "Sec-WebSocket-Protocol"
               << ": "
               << "protocal_str"
               << "\r\n";
    }
    header << "\r\n";
    buf = header;
}

int parse_header(const char* buf, size_t size, ws_header& h) {
    const unsigned char* inp = (const unsigned char*)(buf);
    h.msg_opcode_            = inp[0] & 0x0F;
    h.msg_fin_               = (inp[0] >> 7) & 0x01;
    unsigned char msg_masked = (inp[1] >> 7) & 0x01;

    int pos          = 2;
    int length_field = inp[1] & (~0x80);

    h.left_header_len_ = 0;
    if (length_field <= 125) {
        h.payload_length_ = length_field;
    } else if (length_field == 126)  // msglen is 16bit!
    {
        h.payload_length_ = ntohs(*(uint16_t*)&inp[2]);  // (inp[2] << 8) + inp[3];
        pos += 2;
        h.left_header_len_ = MEDIUM_HEADER - size;
    } else if (length_field == 127)  // msglen is 64bit!
    {
        h.payload_length_ = (size_t)be64toh(*(uint64_t*)&inp[2]);
        pos += 8;
        h.left_header_len_ = LONG_HEADER - size;
    } else {
        return -1;
    }

    if (msg_masked) { h.mask_ = *((unsigned int*)(inp + pos)); }
    h.payload_prt_ = pos + 4;
    return h.left_header_len_ == 0 ? 0 : -2;
}
ws_frame_type parse_payload(const char* buf, size_t size, ws_header& h, fastring& outbuf) {
    const unsigned char* inp = (const unsigned char*)(buf + h.payload_prt_);
    if (h.payload_length_ > size) return ws_frame_type::WS_INCOMPLETE_FRAME;

    if (h.payload_length_ > outbuf.size()) { outbuf.resize((size_t)h.payload_length_); }

    if (h.mask_ == 0) {
        memcpy(&outbuf[0], (void*)(inp), h.payload_length_);
    } else {
        // unmask data:
        for (size_t i = 0; i < h.payload_length_; i++) { outbuf[i] = inp[i] ^ ((unsigned char*)(&h.mask_))[i % 4]; }
    }

    if (h.msg_opcode_ == 0x0)
        return (h.msg_fin_) ? ws_frame_type::WS_TEXT_FRAME : ws_frame_type::WS_INCOMPLETE_TEXT_FRAME;  // continuation
                                                                                                       // frame ?
    if (h.msg_opcode_ == 0x1) return (h.msg_fin_) ? ws_frame_type::WS_TEXT_FRAME : ws_frame_type::WS_INCOMPLETE_TEXT_FRAME;
    if (h.msg_opcode_ == 0x2) return (h.msg_fin_) ? ws_frame_type::WS_BINARY_FRAME : ws_frame_type::WS_INCOMPLETE_BINARY_FRAME;
    if (h.msg_opcode_ == 0x8) return ws_frame_type::WS_CLOSE_FRAME;
    if (h.msg_opcode_ == 0x9) return ws_frame_type::WS_PING_FRAME;
    if (h.msg_opcode_ == 0xA) return ws_frame_type::WS_PONG_FRAME;
    return ws_frame_type::WS_BINARY_FRAME;
}

int handle(web::REQ* p, tcp::Connection& conn, void* callback_vector, std::function<bool(ws::REQ*, void*)> _on_wsbody) {
    fastring buf(1024 * 100);
    int      r;
    ws::REQ  req(p);
   
    while (true) {
        buf.resize(0);
        r = conn.recv((void*)(buf.data()), (int)(buf.capacity()), -1);
        if (r > 0) {
            buf.resize(r);
            ws_header wsh;
            if (parse_header(buf.data(), buf.size(), wsh) == 0) {
                fastring outbuf;
                auto     code = parse_payload(buf.data(), buf.size(), wsh, outbuf);
                req.body      = (fastring &&) outbuf;
                req.code_     = code;
                if (code == ws_frame_type::WS_CLOSE_FRAME) {
                   
                    return -1;
                }
                _on_wsbody(&req, callback_vector);
            }
        } else {
            DLOG << "ws 异常关闭";
            return -1;
        }
    }
    return -1;
}

fastring encode_msg(fastring msg, opcode code) {
    size_t   length = msg.size();
    size_t   header_length;
    fastring buf(4096);
    char     msg_header_[10];
    if (length < 126) {
        header_length  = 2;
        msg_header_[1] = static_cast<char>(length);
    } else if (length <= UINT16_MAX) {
        header_length                 = 4;
        msg_header_[1]                = 126;
        *((uint16_t*)&msg_header_[2]) = htons(static_cast<uint16_t>(length));
    } else {
        header_length                 = 10;
        msg_header_[1]                = 127;
        *((uint64_t*)&msg_header_[2]) = htobe64(length);
    }

    int flags      = 0;
    msg_header_[0] = (flags & SND_NO_FIN ? 0 : 128);
    if (!(flags & SND_CONTINUATION)) { msg_header_[0] |= code; }
    buf.append(msg_header_, header_length).append(msg);
    return buf;
}
}  // namespace ws