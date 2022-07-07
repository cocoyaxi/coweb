/*
 * Copyright (c) 2009-2014 Kazuho Oku, Tokuhiro Matsuno, Daisuke Murase,
 *                         Shigeo Mitsunari
 *
 * The software is licensed under either the MIT License (below) or the Perl
 * license.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <assert.h>
#include <stdio.h>
#include "picohttpparser.h"

#define REQ                                                                                                                        \
    "GET /wp-content/uploads/2010/03/hello-kitty-darth-vader-pink.jpg HTTP/1.1\r\n"                                                \
    "Host: www.kittyhell.com\r\n"                                                                                                  \
    "User-Agent: Mozilla/5.0 (Macintosh; U; Intel Mac OS X 10.6; ja-JP-mac; rv:1.9.2.3) Gecko/20100401 Firefox/3.6.3 "             \
    "Pathtraq/0.9\r\n"                                                                                                             \
    "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"                                                  \
    "Accept-Language: ja,en-us;q=0.7,en;q=0.3\r\n"                                                                                 \
    "Accept-Encoding: gzip,deflate\r\n"                                                                                            \
    "Accept-Charset: Shift_JIS,utf-8;q=0.7,*;q=0.7\r\n"                                                                            \
    "Keep-Alive: 115\r\n"                                                                                                          \
    "Connection: keep-alive\r\n"                                                                                                   \
    "Cookie: wp_ozh_wsa_visits=2; wp_ozh_wsa_visit_lasttime=xxxxxxxxxx; "                                                          \
    "__utma=xxxxxxxxx.xxxxxxxxxx.xxxxxxxxxx.xxxxxxxxxx.xxxxxxxxxx.x; "                                                             \
    "__utmz=xxxxxxxxx.xxxxxxxxxx.x.x.utmccn=(referral)|utmcsr=reader.livedoor.com|utmcct=/reader/|utmcmd=referral\r\n"             \
    "\r\n"
char bf[648] = {80,  79,  83,  84,  32,  47,  97,  112, 105, 47,  108, 111, 103, 32,  72,  84,  84,  80,  47,  49,  46,  49,  13,  10,
            111, 114, 105, 103, 105, 110, 58,  32,  104, 116, 116, 112, 58,  47,  47,  49,  50,  55,  46,  48,  46,  48,  46,  49,
            58,  56,  50,  13,  10,  97,  99,  99,  101, 112, 116, 58,  32,  42,  47,  42,  13,  10,  97,  99,  99,  101, 112, 116,
            45,  101, 110, 99,  111, 100, 105, 110, 103, 58,  32,  103, 122, 105, 112, 44,  32,  100, 101, 102, 108, 97,  116, 101,
            44,  32,  98,  114, 13,  10,  97,  99,  99,  101, 112, 116, 45,  108, 97,  110, 103, 117, 97,  103, 101, 58,  32,  122,
            104, 45,  67,  78,  13,  10,  99,  111, 110, 110, 101, 99,  116, 105, 111, 110, 58,  32,  107, 101, 101, 112, 45,  97,
            108, 105, 118, 101, 13,  10,  117, 115, 101, 114, 45,  97,  103, 101, 110, 116, 58,  32,  77,  111, 122, 105, 108, 108,
            97,  47,  53,  46,  48,  32,  40,  87,  105, 110, 100, 111, 119, 115, 32,  78,  84,  32,  49,  48,  46,  48,  59,  32,
            87,  105, 110, 54,  52,  59,  32,  120, 54,  52,  41,  32,  65,  112, 112, 108, 101, 87,  101, 98,  75,  105, 116, 47,
            53,  51,  55,  46,  51,  54,  32,  40,  75,  72,  84,  77,  76,  44,  32,  108, 105, 107, 101, 32,  71,  101, 99,  107,
            111, 41,  32,  67,  104, 114, 111, 109, 101, 47,  49,  48,  48,  46,  48,  46,  52,  56,  57,  54,  46,  54,  48,  32,
            83,  97,  102, 97,  114, 105, 47,  53,  51,  55,  46,  51,  54,  32,  69,  100, 103, 47,  49,  48,  48,  46,  48,  46,
            49,  49,  56,  53,  46,  50,  57,  13,  10,  97,  99,  99,  101, 115, 115, 45,  99,  111, 110, 116, 114, 111, 108, 45,
            114, 101, 113, 117, 101, 115, 116, 45,  104, 101, 97,  100, 101, 114, 115, 58,  32,  99,  111, 110, 116, 101, 110, 116,
            45,  116, 121, 112, 101, 13,  10,  115, 101, 99,  45,  102, 101, 116, 99,  104, 45,  109, 111, 100, 101, 58,  32,  99,
            111, 114, 115, 13,  10,  115, 101, 99,  45,  102, 101, 116, 99,  104, 45,  115, 105, 116, 101, 58,  32,  99,  114, 111,
            115, 115, 45,  115, 105, 116, 101, 13,  10,  114, 101, 102, 101, 114, 101, 114, 58,  32,  104, 116, 116, 112, 115, 58,
            47,  47,  98,  108, 111, 103, 46,  99,  115, 100, 110, 46,  110, 101, 116, 47,  119, 101, 105, 120, 105, 110, 95,  51,
            48,  54,  54,  54,  55,  53,  51,  47,  97,  114, 116, 105, 99,  108, 101, 47,  100, 101, 116, 97,  105, 108, 115, 47,
            57,  54,  50,  55,  55,  57,  55,  57,  13,  10,  99,  111, 110, 116, 101, 110, 116, 45,  116, 121, 112, 101, 58,  32,
            97,  112, 112, 108, 105, 99,  97,  116, 105, 111, 110, 47,  106, 115, 111, 110, 13,  10,  104, 111, 115, 116, 58,  32,
            49,  50,  55,  46,  48,  46,  48,  46,  49,  58,  56,  50,  13,  10,  99,  111, 110, 116, 101, 110, 116, 45,  108, 101,
            110, 103, 116, 104, 58,  32,  49,  48,  55,  13,  10,  13,  10,  91,  10,  9,   123, 10,  9,   9,   34,  108, 101, 100,
            49,  34,  58,  32,  49,  48,  48,  10,  9,   125, 44,  10,  9,   123, 10,  9,   9,   34,  108, 101, 100, 50,  34,  58,
            32,  50,  48,  48,  10,  9,   125, 44,  10,  9,   123, 10,  9,   9,   34,  108, 101, 100, 51,  34,  58,  32,  51,  48,
            48,  10,  9,   125, 44,  10,  9,   123, 10,  9,   9,   34,  108, 101, 100, 52,  34,  58,  32,  52,  48,  48,  10,  9,
            125, 44,  10,  9,   123, 10,  9,   9,   34,  108, 101, 100, 53,  34,  58,  32,  53,  48,  48,  10,  9,   125, 10,  93};
int main(void)
{
    {
        const char *method;
        size_t method_len;
        const char *path;
        size_t path_len;
        int minor_version;
        struct phr_header headers[32];
        size_t num_headers;
        int i, ret;
        int last_len = 0;
        for (i = 0; i < 1; i++) {
            num_headers = sizeof(headers) / sizeof(headers[0]);
            ret =
                phr_parse_request(bf, 648, &method, &method_len, &path, &path_len, &minor_version, headers, &num_headers, last_len);
            //  assert(ret == sizeof(REQ) - 1);
        }
    }
    int minor_version;
    int status;
    const char *msg = "123456";
    size_t msg_len=6;
    struct phr_header headers[4];
    size_t num_headers;

    char inputbuf[648];
                                                                                                                      
       // size_t slen = sizeof(s) - 1;                                                                                               
         headers[0].name = "Keep-Alive";
        headers[0].name_len = sizeof("Keep-Alive");  

        headers[0].value = "115";
        headers[0].value_len = sizeof("115"); 
        num_headers = 1; // sizeof(headers) / sizeof(headers[0]);                                                                        
      //  memcpy(inputbuf - slen, s, slen);                                                                                          
     int ret= phr_parse_response(inputbuf , 648, &minor_version, &status, &msg, &msg_len, headers, &num_headers, 0);
        assert(ret == sizeof(REQ) - 1);
    return 0;
}
