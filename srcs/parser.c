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
    MAP_CHILD *temp;
    MAP_CHILD *pretemp;
    int idx;

    temp = req->header;
    req->header = (MAP_CHILD *) calloc (req->header ?
        oatuh_get_header_size(req->header) + oatuh_get_header_size(preheader) + 1 :
        oatuh_get_header_size(preheader) + 1, sizeof(MAP_CHILD)); 
    for (idx = 0; preheader[idx].key; idx++)
    {
        req->header[idx].key = strdup(preheader[idx].key);
        req->header[idx].value = strdup(preheader[idx].value);
    }

    if (!temp)
        return ;
    for (int i = 0; temp[i].key; i++)
    {
        if (get_header(req->header, temp[i].key)->key)
            continue;
        req->header[idx].key = temp[i].key;
        req->header[idx++].value = temp[i].value;
    }
    free(temp);
}

static void parser_body_header(REQUEST *req)
{
    BODY_TYPE type;
    char buffer[100];

    type = *(int *) req->body;
    if (type == RAW)
    {
        sprintf(buffer, "%d", ((BODY_RAW *) req->body)->length);
        oatuh_set_header(&req->header, "Content-Length", buffer);
    }
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
	if (req->type->request_body && req->body)
        parser_body_header(req);
    return NO_ERROR;
}

/* header */

MAP_CHILD *get_header(MAP_CHILD *header, char *key)
{
	int i;

	for (i = 0; header[i].key; i++)
	{
		if (!strcasecmp(header[i].key, key))
			return header + i;
	}
	return header + i;
}

/* body parser */

void *oatuh_raw_body(char *raw)
{
    BODY_RAW *body;
    
    body = (BODY_RAW *) calloc(1, sizeof(BODY_RAW));
    if (!body)
        return NULL;
    body->type = RAW;
    body->body = strdup(raw);
    if (!body->body)
        return NULL;
    body->length = strlen(raw);
    return (void *)body;
}