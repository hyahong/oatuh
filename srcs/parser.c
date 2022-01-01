#include <oatuh.h>

static int parser_method(REQUEST *req)
{
    char method[10] = { 0, };

    req->type = NULL;
    strcpy(method, req->method);
    for (int i = 0; i <= METHOD_NUMBER; i++)
    {
        if (!strcasecmp(method, method_type[i].name))
        {
            req->type = &method_type[i];
            return NO_ERROR;
        }
    }
    return INVALID_METHOD;
}

static void parser_uri(REQUEST *req)
{
    int idx;
    char *cursor;

    cursor = req->uri;
    // scheme
    req->scheme = HTTP;
    if (strlen(cursor) > 7 && !strncmp(cursor, "http://", 7))
        cursor += 7;
    else if (strlen(cursor) > 8 && !strncmp(cursor, "https://", 8))
    {
        req->scheme = HTTPS;
        cursor += 8;
    }
    // authority
    idx = 0;
    while (cursor[idx] && cursor[idx] != '/')
        idx++;
    req->authority = strndup(cursor, idx);
    cursor += idx;
    // path
    req->path = strdup(strlen(cursor) > 0 ? cursor : "/");

    // port
    idx = 0;
    cursor = req->authority;
    while (cursor[idx] && cursor[idx] != ':')
        idx++;
    req->host = strndup(cursor, idx);
    if (!cursor[idx])
        req->port = strdup(req->scheme == HTTPS ? "443" : "80");
    else
        req->port = strdup(cursor + idx + 1);
}

static int parser_dns(REQUEST *req)
{
    struct hostent *host = NULL;
    struct in_addr addr;
 
    memset(&addr, 0x00, sizeof(addr));
    host = gethostbyname(req->host);
    if (!host)
        return INVALID_HOST;
    addr.s_addr = *(u_long *)host->h_addr_list[0];
    req->ip = strdup(inet_ntoa(addr));
    return NO_ERROR;
}

static void parser_header(REQUEST *req)
{
    MAP_CHILD preheader[7] = {
        {"Host", req->host},
        {"User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/96.0.4664.110 Safari/537.36"},
        {"Accept", "*/*"},
        {"Accept-Encoding", "identity"},
        {"Connection", "keep-alive"},
        {NULL, NULL},
    };

    for (int i = 0; preheader[i].key; i++)
        oatuh_set_header(&(req->header), preheader[i].key, preheader[i].value);
}

static void parser_body(REQUEST *req)
{
    printf("body parser\n");
}

int oatuh_parser(REQUEST *req)
{
    int error_code;

    error_code = parser_method(req);
    if (error_code != NO_ERROR)
        return error_code;
    parser_uri(req);
    error_code = parser_dns(req);
    if (error_code != NO_ERROR)
        return error_code;
    parser_header(req);
    if (req->type->request_body)
        parser_body(req);
    return NO_ERROR;
}