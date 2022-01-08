# Request C library

## Oatuh

Oauth is very ease-to-use request library for http/https.
** used OpenSSL for https. so it will work in OpenSSL available environment. **

```c
#include "incs/oatuh.h"

int main(void)
{
    REQUEST *req;
    
    req = oatuh_create();
        
    req->method = "GET";
    req->uri = "https://github.com/Yechan0815/oatuh";

    oatuh(req);

    req->response->status_code; // 200
    req->response->status; // OK
    req->response->body; // html body
    req->response->length; // body length
    
    oatuh_get_header(req->response->header, "Content-Type"); // text/html; charset=utf-8
    
    oatuh_destroy(req);
    
    return 0;
}
```

## Documentation

* Support method `GET`, `POST`, `PUT`, `PATCH`, `DELETE`, `HEAD`, `OPTIONS`
* Custom headers
* Support to upload body type
    - raw (plain like text, json, html etc...)
    - multipart (support soon)

* Cookie (not yet supported)
* Proxy (not yet supported)

## Method

briefly describe each method.

below is the presence or absence of body according to method.

|Method|Request Body|Response Body|
|------|---|---|
|  GET  |X|O|
|  POST |O|O|
|  PUT  |O|O|
| PATCH |O|O|
| DELETE|X|O|
|  HEAD |X|X|
|OPTIONS|X|X|

If X, ignore even if the body can be written/read when sending/receiving. For example, if the method is GET, even if the body is allocated, it is not sent.

### GET

```c
#include "incs/oatuh.h"

int main(void)
{
    REQUEST *req;
    
    req = oatuh_create();

    req->method = "GET";
    req->uri = "uri";

    oatuh(req);

    oatuh_destroy(req);
    
    return 0;
}
```
