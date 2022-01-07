# Request C library

## Oatuh

Oauth is very ease request library for http/https.

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
* Support to upload raw(text, json, html etc...), multipart(not yet supported) type
* Cookie (not yet supported)
* Proxy (not yet supported)

