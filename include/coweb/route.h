#pragma once
#include "coweb/http1.h"
#include "co/fastring.h"
#include "coweb/ws.h"
//#define route_CALLBACK std::function<bool(REQ*)>

#include <stdarg.h>

namespace web {
typedef bool (*rut_callback)(web::REQ *);
typedef bool (*ws_rut_callback)(ws::REQ *);
class ROUTE {
   public:
    ROUTE();
    ~ROUTE();
    void reg(fastring  url, co::vector <fastring> method, co::vector<rut_callback> v) {
        for (size_t i = 0; i < method.size(); i++) {
            fastring m_url;//GET_/xxx/xxx
            m_url << method[i] << "_" << url;
            map[m_url] = v;
        }
    }
    //path:/xx/xx?xxx=xx 
    //method:GET/POST/XXX
    co::vector<rut_callback> get(fastring path, const char *method) {
        co::vector<rut_callback> v;
        if (map.size()<=0)
        {
            return v;
        }
        fastring m_url;  // GET_/xxx/xxx
        size_t   len = path.find("?");
        if (len == path.npos) {
            m_url.append(method).append("_").append(path);
        } else {
            m_url.append(method).append("_").append(path.data(), len );
        }
        v = map[m_url];
        return v;
    }
    void reg_pcre(fastring url, co::vector<fastring> method, co::vector<rut_callback> v) {
        for (size_t i = 0; i < method.size(); i++) {
            fastring m_url;  // GET_/xxx/xxx
            m_url << method[i] << "_" << url;
            map_pcre[m_url] = v;
        }

        REQ req;
        for (size_t i = 0; i < v.size(); i++) {
            v[i](&req);
        }
    }
    // path:/xx/xx?xxx=xx
    // method:GET/POST/XXX
    co::vector<rut_callback> get_pcre(fastring path, const char* method) {
        fastring m_url;  // GET_/xxx/xxx
        size_t   len = path.find("?");
        if (len == path.npos) {
            m_url.append(method).append("_").append(path);
        } else {
            m_url.append(method).append("_").append(path.data(), len - 1);
        }
        co::vector<rut_callback> v = map_pcre[m_url];
        return v;
    }
    
    //websocket
    void reg_ws(fastring url, co::vector<fastring> method, co::vector<ws_rut_callback> v) {
        for (size_t i = 0; i < method.size(); i++) {
            fastring m_url;  // GET_/xxx/xxx
            m_url << method[i] << "_" << url;
            map_ws[m_url] = v;
        }
    }
    // path:/xx/xx?xxx=xx
    // method:GET/POST/XXX
    co::vector<ws_rut_callback> get_ws(fastring path, const char *method) {
        co::vector<ws_rut_callback> v;
        if (map.size() <= 0) {
            return v;
        }
        fastring m_url;  // GET_/xxx/xxx
        size_t   len = path.find("?");
        if (len == path.npos) {
            m_url.append(method).append("_").append(path);
        } else {
            m_url.append(method).append("_").append(path.data(), len);
        }
        v = map_ws[m_url];
        return v;
    }
   private:
    co::hash_map<fastring, co::vector<rut_callback>> map;
    co::hash_map<fastring, co::vector<rut_callback>> map_pcre;//pcre route map
    //ws
    co::hash_map<fastring, co::vector<ws_rut_callback>> map_ws;
};



}
