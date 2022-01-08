#include "incs/oatuh.h"

int main(void)
{
    REQUEST *req = oatuh_create();
    char *body;

    body = "{\"uid\": 12, \"name\": \"oatuh\"}";

    req->method = "post";
    req->uri = "https://jsonplaceholder.typicode.com/posts";

    oatuh_set_header(&req->header, "Content-Type", "application/json; charset=UTF-8");

    req->body = oatuh_create_raw_body(body, strlen(body));

    oatuh(req);

    printf("-- request header -- \n");
    for (int i = 0; i < oatuh_get_header_size(req->header); i++)
        printf("%s: %s\n", req->header[i].key, req->header[i].value);

    printf("\n-- response header -- \n");
    printf("%s %s\n", req->response->status_code, req->response->status);
    for (int i = 0; i < oatuh_get_header_size(req->response->header); i++)
        printf("%s: %s\n", req->response->header[i].key, req->response->header[i].value);

    printf("\n-- response body -- \n");
    if (req->response->body)
        printf("%s\n", req->response->body);

    oatuh_destroy(req);
}