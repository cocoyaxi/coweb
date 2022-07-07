#include "coweb/http1.h"
#include "co/co.h"
#include "coweb/Encodec.h"
#include "picohttpparser/picohttpparser.h"
namespace web
{
	REQ::REQ()
	{
		auto pbuf = &sendbuf;
		auto pmtx = &mtx;
		auto pexit = &exit;
		go([=]() {
			fastring buf;
			while (*pexit == 1)
			{
				pmtx->lock();
				buf = *pbuf;
				pbuf->resize(0);
				pmtx->unlock();

				if (buf.size() > 0)
				{
					auto r = conn_->send(buf.data(), buf.size());
					if (r <= 0)
					{
						DLOG << "web_send err!";
						*pexit = -1;//exit co send
					}
				}
				co::sleep(1);
			}
			*pexit = -1;//exit co send
			});
	}
	REQ::~REQ()
	{
		if (exit == 1)
		{
			exit = 0;
			while (exit == 0)
			{
				co::sleep(10);
			}
		}
	}
	void REQ::send(fastring msg)
	{
		mtx.lock();
		sendbuf.append((fastring&)msg);
		mtx.unlock();
	}

	void REQ::send_msg(Json& body, int code,bool utf8) {
		RES res;
		res.status = code;
		res.add_header("content-type", utf8?"application/json; charset=UTF-8":"application/json");
		if (utf8) {
			res.body = Encode::GBKToUTF8(body.str().c_str());
		}
		else {
			body.str(res.body);
		}
		
		send(res.get());
	}
	void REQ::send_msg(fastring& body, int code, bool utf8)
	{
		RES res;
		res.status = code;
		res.add_header("content-type", utf8 ? "text/html; charset=UTF-8" : "text/html");
		if (utf8) {
			res.body = Encode::GBKToUTF8(body.c_str());
		}
		else{
			res.body = body;
		}
		send(res.get());
	}
	inline fastring& fastring_cache() {
		static __thread fastring* kS = 0;
		if (kS) return *kS;
		return *(kS = new fastring(128));
	}

	inline fastream& fastream_cache() {
		return *(fastream*)&fastring_cache();
	}

	const char** create_status_table() {
		static const char* s[512];
		for (int i = 0; i < 512; ++i) s[i] = "";
		s[100] = "Continue";
		s[101] = "Switching Protocols";
		s[200] = "OK";
		s[201] = "Created";
		s[202] = "Accepted";
		s[203] = "Non-authoritative Information";
		s[204] = "No Content";
		s[205] = "Reset Content";
		s[206] = "Partial Content";
		s[300] = "Multiple Choices";
		s[301] = "Moved Permanently";
		s[302] = "Found";
		s[303] = "See Other";
		s[304] = "Not Modified";
		s[305] = "Use Proxy";
		s[307] = "Temporary Redirect";
		s[400] = "Bad Request";
		s[401] = "Unauthorized";
		s[402] = "Payment Required";
		s[403] = "Forbidden";
		s[404] = "Not Found";
		s[405] = "Method Not Allowed";
		s[406] = "Not Acceptable";
		s[407] = "Proxy Authentication Required";
		s[408] = "Request Timeout";
		s[409] = "Conflict";
		s[410] = "Gone";
		s[411] = "Length Required";
		s[412] = "Precondition Failed";
		s[413] = "Payload Too Large";
		s[414] = "Request-URI Too Long";
		s[415] = "Unsupported Media Type";
		s[416] = "Requested Range Not Satisfiable";
		s[417] = "Expectation Failed";
		s[500] = "Internal Server Error";
		s[501] = "Not Implemented";
		s[502] = "Bad Gateway";
		s[503] = "Service Unavailable";
		s[504] = "Gateway Timeout";
		s[505] = "HTTP Version Not Supported";
		return s;
	}

	inline const char* status_str(int n) {
		static const char** s = create_status_table();
		return (100 <= n && n <= 511) ? s[n] : s[500];
	}


	void RES::add_header(fastring k, fastring v) {
		header.append(k).append(": ").append(v).append("\r\n");
	}
	fastring RES::get(void)
	{
		fastring buf;
		buf.append(version).append(" ");
		(fastream&)buf << (status);
		buf.append(" ").append(status_str(status)).append("\r\n");
		buf.append(header);
		if (body.size() > 0)
		{
			buf.append("Content-Length: ");
			buf << (body.size());
			buf.append("\r\n");
			buf.append("\r\n").append(body);
		}
		else
		{
			buf.append("\r\n");
		}
		return buf;
	};

	int parse_web_headers(fastring* buf, Json* req) {
		fastring s(1024), name(256);
		char* method, * path;
		int pret, minor_version;
		struct phr_header headers[max_headers];
		size_t buflen = 0, prevbuflen = 0, method_len, path_len, num_headers = max_headers;
		pret = phr_parse_request(buf->c_str(), buf->size(), (const char**)&method, &method_len, (const char**)&path, &path_len, &minor_version, headers, &num_headers, prevbuflen);

		if (pret == -1)
			return -1;
		/* request is incomplete, continue the loop */
		assert(pret == -2);
		if (buflen == sizeof(buf))
			return -2;
		if (pret > 0) {
			s.append(method, method_len);
			req->add_member("method", s);

			s.clear(); s.append(path, path_len);
			req->add_member("path", s);

			req->add_member("version", (minor_version != 0) ? "HTTP/1.1" : "HTTP/1.0");//http1.0/1.1
			for (size_t i = 0; i < num_headers; i++)
			{
				name.clear(); s.clear();

				name.append(headers[i].name, headers[i].name_len);
				name.tolower();
				s.append(headers[i].value, headers[i].value_len);
				req->add_member(name.c_str(), s);
			}
			return pret; /* successfully parsed the request */
		}

		return -2;
	}

	int parse_web_chunk(fastring* buf, Json* req) {
	}
	Json parse_form_data(fastring& body, bool utf8)
	{
		Json r;
		auto name_len = body.find("\r\n");
		if (name_len == body.npos)
		{
			return r;
		}
		fastring form_name;
		size_t ptr_name, ptr_name_len, ptr_value, ptr_value_len;
		std::string k, v;
		form_name.append(body.data(), name_len);

		for (size_t i = name_len; i < body.size();)
		{
			ptr_name = body.find(" name=\"", i);
			if (ptr_name == body.npos)
				break;
			ptr_name += 7;
			ptr_name_len = body.find('"', ptr_name);
			if (ptr_name_len == body.npos)
				break;
			ptr_name_len = ptr_name_len - ptr_name;
			k.resize(0); v.resize(0);
			k.append(body.data() + ptr_name, ptr_name_len);
			if (utf8)
			{
				k = Encode::UTF8ToGBK(k);
			}
			ptr_value = body.find("\r\n\r\n", ptr_name + ptr_name_len);
			if (ptr_value == body.npos) break;
			ptr_value = ptr_value + 5;
			ptr_value_len = body.find(form_name.c_str(), ptr_value);
			if (ptr_value_len == body.npos) break;
			ptr_value_len = ptr_value_len - 3 - ptr_value;
			v.append(body.data() + ptr_value, ptr_value_len);
			if (body.data()[ptr_name + ptr_name_len + 1] == ';' && body.data()[ptr_name + ptr_name_len + 3] == 'f' && body.data()[ptr_name + ptr_name_len + 4] == 'i')//; filename
			{
				auto vv = base64_encode(v);
				r.add_member(k.c_str(), vv);
			}
			else
			{
				if (utf8)
				{
					v = Encode::UTF8ToGBK(v);
				}
				r.add_member(k.c_str(), v.c_str());
			}

			i = ptr_value + ptr_value_len + 2;
		}
		return r;
	}
}
