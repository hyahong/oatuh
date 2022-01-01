#include "incs/oatuh.h"

int main(void)
{
    {
        REQUEST *req = oatuh_create();
        req->method = "get";
        //req->uri = "https://jsonplaceholder.typicode.com/posts";
        //req->uri = "https://i.redd.it/z18w20p6j9381.png";
        req->uri = "https://ko.wikipedia.org/wiki/URL";

        oatuh_set_header(&(req->header), "Test", "new1");

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
}