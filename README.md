# Request C library

## Oatuh

Oauth is very ease-to-use request library for http/https.

**used OpenSSL for https. so it will work in OpenSSL available environment.**

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

This is an example of GET, POST method.

### GET

```c
#include "incs/oatuh.h"

int main(void)
{
    REQUEST *req;
    
    req = oatuh_create();

    req->method = "GET";
    req->uri = "uri";
    
    oatuh_set_header(&req->header, "Custom-Header-Key", "Custom-Header-Value");

    oatuh(req);

    oatuh_destroy(req);
    
    return 0;
}
```


### POST

```c
#include "incs/oatuh.h"

int main(void)
{
    REQUEST *req;
    char *body = "{\"uid\": 12, \"name\": \"oatuh\"}";
    
    req = oatuh_create();

    req->method = "POST";
    req->uri = "uri";
    req->body = oatuh_create_raw_body(body, strlen(body));

    oatuh_set_header(&req->header, "Content-Type", "application/json; charset=UTF-8");

    oatuh(req);

    oatuh_destroy(req);
    
    return 0;
}
```

## Function

### oatuh_create ()

```c
REQUEST *oatuh_create ()
```

create an initialized oatuh REQEUST block.

return REQUEST block pointer.

### oatuh_destroy ()

```c
void oatuh_destroy (REQUEST *req)
```

destroy an oatuh REQEUST block.

frees all resources used by oauth. if the body has not been freed, the body will also be freed.

<br/>
<br/>

### oatuh_get_header_size ()

```c
int oatuh_get_header_size (MAP_CHILD *header)
```

`MAP_CHILD *header` is the target header to get the size.

return the size of header

<br/>

### oatuh_get_header ()

```c
char *oatuh_get_header(MAP_CHILD *header, char *key)
```

`MAP_CHILD *header` is the target header to search the key.

`char *key` is the key to search for in the header list.

returns a value based on the key. Returns NULL if the key does not exist.


### oatuh_set_header ()

```c
void oatuh_set_header(MAP_CHILD **header, char *key, char *value)
```

`MAP_CHILD **header` is a pointer to the *header to be set.

`char *key` is the key.

`char *value` is the value according to key.

<br/>
<br/>

### oatuh_create_raw_body ()

```c
void *oatuh_create_raw_body(char *raw, int length)
```

create body of raw type like text, json, html.

`char *raw` is body.

`int length` is length of body. 

return the body pointer.

### oatuh_destroy_raw_body ()

```c
void oatuh_destroy_raw_body(BODY_RAW *body)
```

destroy the raw body. frees the body resources.

even if you don't use function `oatuh_destroy_raw_body ()`, it will be deallocated in `oatuh_destroy ()`.

`BODY_RAW *body` is the body pointer.

<br/>

### oatuh_destroy_body ()

```c
void oatuh_destroy_body(void *body)
```

destroy any type of body.

`void *body` is the body pointer.

<br/>
<br/>

### oatuh ()

```c
int     oatuh(REQUEST *req)
```

Parsing all data to be used except body and request.

return NO_ERROR on success, error mecro like INVALID_METHOD on error.


## Macro

### Error handling

oatuh returns NO ERROR on success and the error code below on failure.

```
NO_ERROR: no error.

MEMORY_ALLOCATION_FAIL: memory allocation fail.

INVALID_METHOD: when a method other than `GET`, `POST`, `PUT`, `PATCH`, `DELETE`, `HEAD`, or `OPTIONS` is entered.
INVALID_HOST: can't get ip address from domain.

CONNECTION_FAIL: socket connection fail.
HANDSHAKE_FAIL: ssl handshake error.

SSL_CTX_CREATE_FAIL: ssl ctx allocation fail.
```
