#include <oatuh.h>

/*
 * util
 *  utility function
 */
static char *memnstr(char *src, int n, char *str, int s)
{
	for (int i = 0; i < n - s; i++)
	{
		if (!memcmp(src + i, str, s))
			return src + i;
	}
	return NULL;
}

static char *memndup(char *src, size_t n)
{
	char *dest;
	
	dest = (char *) calloc(n + 1, sizeof(char));
	memcpy(dest, src, n);
	return dest;
}

static int htoi(char *hex)
{
	int decimal;
	int number;
	int power;

	decimal = 0;
	power = 1;
	for (int i = strlen(hex) - 1; i >= 0; i--)
	{
		if ('0' <= hex[i] && hex[i] <= '9')	
			number = hex[i] - '0';
		else if ('a' <= hex[i] && hex[i] <= 'f')
			number = hex[i] - 'a' + 10;
		else if ('A' <= hex[i] && hex[i] <= 'F')
			number = hex[i] - 'A' + 10;
		decimal += number * power; 
		power *= 16;
	}
	return decimal;
}

/*
 * parser
 *  parse uri, header, body
 */

/* header */
static MAP_CHILD *get_header(MAP_CHILD *header, char *key)
{
	int i;

	for (i = 0; header[i].key; i++)
	{
		if (!strcasecmp(header[i].key, key))
			return header + i;
	}
	return header + i;
}

/* parser */
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

static int oatuh_parser(REQUEST *req)
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

/*
 * socket
 *  the part that uses socket directly
 */
static int connect_socket(REQUEST *req)
{
    struct hostent *host;
    struct sockaddr_in addr;

    req->connection.socket = socket(PF_INET, SOCK_STREAM, 0);
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(req->port));
    addr.sin_addr.s_addr = inet_addr(req->ip);
    if (connect(req->connection.socket, (struct sockaddr*)&addr, sizeof(addr)) != 0)
    {
        close(req->connection.socket);
		return CONNECTION_FAIL;
    }
    return NO_ERROR;
}

static int connect_ssl(REQUEST *req)
{
    const SSL_METHOD *method;
    X509 *cert;

	// ssl handshake 
    SSL_library_init();
    SSL_load_error_strings();
    method = TLS_client_method();
	req->connection.tls.ctx = SSL_CTX_new(method);
    if (!req->connection.tls.ctx)
	{
		SSL_CTX_free(req->connection.tls.ctx);
		return SSL_CTX_CREATE_FAIL;
	}
    req->connection.tls.ssl = SSL_new(req->connection.tls.ctx);
    SSL_set_tlsext_host_name(req->connection.tls.ssl, req->host);
    SSL_set_fd(req->connection.tls.ssl, req->connection.socket);
    if (SSL_connect(req->connection.tls.ssl) == -1)
	{
		SSL_free(req->connection.tls.ssl);
		return HANDSHAKE_FAIL;
	}
	// subject issuer
    cert = SSL_get_peer_certificate(req->connection.tls.ssl);
    if (cert)
    {
        req->connection.tls.subject = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
        req->connection.tls.issuer = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
        X509_free(cert);
    }
	return NO_ERROR;
}

static void write_header(REQUEST *req)
{
	char *header;
	int length;
	int written;

	length = 0;
	length += strlen(req->type->name) + strlen(req->path) + 11;
	for (int i = 0; req->header[i].key; i++)
		length += strlen(req->header[i].key) + strlen(req->header[i].value) + 3;
	length++;
	header = calloc(length + 1, sizeof(char));
	strcat(header, req->type->name);
	strcat(header, " ");
	strcat(header, req->path);
	strcat(header, " HTTP/1.1\n");
	for (int i = 0; req->header[i].key; i++)
	{
		strcat(header, req->header[i].key);
		strcat(header, ": ");
		strcat(header, req->header[i].value);
		strcat(header, "\n");
	}
	strcat(header, "\n");

	written = 0;
	while (written < length)
	{
		if (req->scheme == HTTPS)
			written += SSL_write(req->connection.tls.ssl, header, length);
		else
			written += write(req->connection.socket, header, length);
	}
	free(header);
}

static void write_body_raw(REQUEST *req)
{
	BODY_RAW *body;
	int written;
	
	body = (BODY_RAW *) req->body;
	written = 0;
	while (written < body->length)
	{
		if (req->scheme == HTTPS)
			written += SSL_write(req->connection.tls.ssl, body->body, body->length - written);
		else
			written += write(req->connection.socket, body->body, body->length - written);
	}
}

static int write_request(REQUEST *req)
{
	BODY_TYPE type;

	write_header(req);
	if (req->type->request_body && req->body)
	{
		type = *(int *) req->body;
		if (type == RAW)
			write_body_raw(req);
	}
}

static void read_header(REQUEST *req)
{
	char *header, *temp;
	char *pos, *cursor;
	char buffer[2048];
	int bytes, length;
	int count;

	// read header
	length = 0;
	header = strdup("");
	do
	{
		bytes = req->scheme == HTTPS ? SSL_read(req->connection.tls.ssl, buffer, 2047) : read(req->connection.socket, buffer, 2047);
		temp = (char *) calloc(length + bytes + 1, sizeof(char));
		memcpy(temp, header, length);
		memcpy(temp + length, buffer, bytes);
		free(header);
		header = temp;
		length += bytes;
		if (strstr(buffer, "\r\n\r\n"))
			break;
	}
	while (1);
	pos = strstr(header, "\r\n\r\n");
	req->response->buffer = memndup(pos + 4, length - (int)(pos - header + 4));
	req->response->buffer_length = length - (int)(pos - header + 4);
	*(pos + 2) = 0;

	// status header
	pos = strstr(header, "\r\n");
	req->response->status_code = strndup(header + 9, 3);
	req->response->status = strndup(header + 13, pos - header - 13);
	// count header
	pos = header;
	count = -1;
	while ((pos = strstr(pos + 2, "\r\n")))
		count++;
    req->response->header = (MAP_CHILD *) calloc (count + 1, sizeof(MAP_CHILD));
	pos = header;
	for (int i = 0; i < count && (pos = strstr(pos + 2, "\r\n")); i++)
	{
		for (cursor = pos + 2; *cursor != ':'; cursor++);
		req->response->header[i].key = strndup(pos + 2, cursor - pos - 2);
		req->response->header[i].value = strndup(cursor + 2, strstr(cursor + 2, "\r\n") - cursor - 2);
	}
	free(header);
}

static int read_body_chunked(REQUEST *req)
{
	char **block;
	int *length_block;
	char buffer[2048];
	char *size, *temp, *pos;
	int bytes, length, block_pos, block_count;

	block_pos = 0;
	block_count = 1;
	block = (char **) calloc(block_count, sizeof(char *));
	length_block = (int *) calloc(block_count, sizeof(int));

start:
	while (!(pos = memnstr(req->response->buffer, req->response->buffer_length, "\r\n", 2)))
	{
		bytes = req->scheme == HTTPS ? SSL_read(req->connection.tls.ssl, buffer, 1) : read(req->connection.socket, buffer, 1);
		temp = (char *) calloc(req->response->buffer_length + 2, sizeof(char));
		memcpy(temp, req->response->buffer, req->response->buffer_length);
		memcpy(temp + req->response->buffer_length, buffer, 1);
		free(req->response->buffer);
		req->response->buffer = temp;
		req->response->buffer_length += bytes;
	}
	size = strndup(req->response->buffer, pos - req->response->buffer);
	length = htoi(size);
	req->response->buffer_length -= strlen(size) + 2;
	memmove(req->response->buffer, req->response->buffer + strlen(size) + 2, req->response->buffer_length);
	free(size);

	if (!length)
		goto end;

	if (block_pos == block_count)
	{
		temp = (char *)block;
		block = (char **) calloc(block_count * 2, sizeof(char *));
		for (int i = 0; i < block_count; i++)
			block[i] = ((char **)temp)[i];
		free((char **)temp);

		temp = (char *)length_block;
		length_block = (int *) calloc(block_count * 2, sizeof(int));
		for (int i = 0; i < block_count; i++)
			length_block[i] = ((int *)temp)[i];
		free((int *)temp);

		block_count *= 2;
	}

	if (length > req->response->buffer_length)
	{
		length -= req->response->buffer_length - 2;
		while (length > 0)
		{
			bytes = req->scheme == HTTPS ? SSL_read(req->connection.tls.ssl, buffer, length > 2047 ? 2047 : length) : read(req->connection.socket, buffer, length > 2047 ? 2047 : length);
			temp = (char *) calloc(req->response->buffer_length + bytes + 1, sizeof(char));
			memcpy(temp, req->response->buffer, req->response->buffer_length);
			memcpy(temp + req->response->buffer_length, buffer, bytes);
			free(req->response->buffer);
			req->response->buffer = temp;
			req->response->buffer_length += bytes;
			length -= bytes;
		}
		block[block_pos] = req->response->buffer;
		length_block[block_pos++] = req->response->buffer_length;
		req->response->buffer = strdup("");
		req->response->buffer_length = 0;

		goto start;
	}
	temp = req->response->buffer;
	block[block_pos] = memndup(req->response->buffer, length + 2);
	length_block[block_pos++] = length + 2;
	req->response->buffer = memndup(req->response->buffer + length + 2, req->response->buffer_length - length - 2);
	req->response->buffer_length -= length + 2;
	free(temp);

	goto start;

end:
	length = 0;
	for (int i = 0; i < block_pos; i++)
		length += length_block[i];
	req->response->body = calloc(length, sizeof(char));
	req->response->length = length;
	length = 0;
	for (int i = 0; i < block_pos; i++)
	{
		memcpy(req->response->body + length, block[i], length_block[i] - 2);
		free(block[i]);
	}
	free(block);
	free(length_block);

	return NO_ERROR;
}

static int read_body_length(REQUEST *req)
{
	char buffer[2048];
	int bytes, length;

	req->response->length = atoi(get_header(req->response->header, "Content-Length")->value);
	req->response->body = (char *) calloc(req->response->length, sizeof(char));	
	if (!req->response->body)
		return MEMORY_ALLOCATION_FAIL;
	length = req->response->buffer_length;
	memcpy(req->response->body, req->response->buffer, length);
	while (length < req->response->length)
	{
		bytes = req->scheme == HTTPS ? SSL_read(req->connection.tls.ssl, buffer, 2047) : read(req->connection.socket, buffer, 2047);
		buffer[bytes] = 0;
		memcpy(req->response->body + length, buffer, bytes);
		length += bytes;
	}
	return NO_ERROR;
}

static int read_response(REQUEST *req)
{
	int error_code;

	req->response = (RESPONSE *) calloc(1, sizeof(RESPONSE));
	read_header(req);
	if (req->type->response_body)
	{
		if (get_header(req->response->header, "Transfer-Encoding")->value &&
		!strcasecmp(get_header(req->response->header, "Transfer-Encoding")->value, "chunked"))
			error_code = read_body_chunked(req);
		else
			error_code = read_body_length(req);
		if (error_code != NO_ERROR)
			return error_code;
	}
	return NO_ERROR;
}

/*
 * oatuh function
 */

/*
 * oatuh_create ()
 * 
 *   create oatuh REQUEST block
 *
 *    return (REQUEST *): REQUEST block pointer
 */
REQUEST	*oatuh_create()
{
	REQUEST *req;

	req = (REQUEST *) calloc(1, sizeof(REQUEST));
	return req;
}

/*
 * oatuh_destroy ()
 * 
 *   destroy oatuh REQUEST block
 * 
 *    req (REQUEST *): REQUEST block to be freed
 */
void	oatuh_destroy(REQUEST *req)
{
	if (!req)
		return ;
	/* connection */
	if (req->connection.tls.subject)
		free(req->connection.tls.subject);
	if (req->connection.tls.issuer)
		free(req->connection.tls.issuer);
	/* request */
	if (req->authority)
		free(req->authority);
	if (req->host)
		free(req->host);
	if (req->ip)
		free(req->ip);
	if (req->port)
		free(req->port);
	if (req->path)
		free(req->path);
	if (req->header)
	{
		for (int i = 0; i < oatuh_get_header_size(req->header); i++)
		{
			if (req->header[i].key)
				free(req->header[i].key);
			if (req->header[i].value)
				free(req->header[i].value);
		}
		free(req->header);
	}
	if (req->body)
		oatuh_destroy_body(req->body);
	/* response */
	if (req->response)
	{
		if (req->response->status_code)
			free(req->response->status_code);
		if (req->response->status)
			free(req->response->status);
		if (req->response->header)
		{
			for (int i = 0; i < oatuh_get_header_size(req->response->header); i++)
			{
				if (req->response->header[i].key)
					free(req->response->header[i].key);
				if (req->response->header[i].value)
					free(req->response->header[i].value);
			}
			free(req->response->header);
		}
		if (req->response->buffer)
			free(req->response->buffer);
		if (req->response->body)
			free(req->response->body);
		free(req->response);
	}
	/* block */
	free(req);
}

/*
 * oatuh_get_header_size ()
 * 
 *   get size of headers in *header 
 * 
 *    header (MAP_CHILD *): headers to get size
 *    return (int): header size
 */
int		oatuh_get_header_size(MAP_CHILD *header)
{
	int size;

	size = 0;
	while (header[size++].key);
	return size - 1;
}

/*
 * oatuh_get_header ()
 * 
 *   get header value by key
 * 
 *    header (MAP_CHILD *): header list to search
 *    key (char *): header key like Host, Content-Type
 *    return (char *): header value
 */
char	*oatuh_get_header(MAP_CHILD *header, char *key)
{
	return get_header(header, key)->value;
}

/*
 * oatuh_set_header ()
 * 
 *   set header key-value. if the key already exists, update the value of that key.
 * 
 *    header (MAP_CHILD **): pointer of headers
 *    key (char *): key (like Host, Content-Type or custom)
 *    value (char *): value
 */
void	oatuh_set_header(MAP_CHILD **header, char *key, char *value)
{
	MAP_CHILD *temp;
	int size;

	if (*header)
	{
		if ((temp = get_header(*header, key)) && temp->key)
		{
			if (temp->value)
				free(temp->value);
			temp->value = strdup(value);
			return ;
		}
		size = oatuh_get_header_size(*header);
		temp = *header;
		*header = (MAP_CHILD *) calloc (size + 2, sizeof(MAP_CHILD));
		for (int i = 0; i < size; i++)
		{
			(*header)[i].key = temp[i].key;
			(*header)[i].value = temp[i].value;
		}
		(*header)[size].key = strdup(key);
		(*header)[size].value = strdup(value);
		free(temp);
		return ;
	}
	*header = (MAP_CHILD *) calloc (2, sizeof(MAP_CHILD));
	(*header)[0].key = strdup(key);
	(*header)[0].value = strdup(value);
}

/*
 * oatuh_create_raw_body ()
 * 
 *   create raw body
 *
 *    raw (char *): raw data
 *    return (void *): processed body pointer
 */
void *oatuh_create_raw_body(char *raw)
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

/*
 * oatuh_destroy_raw_body ()
 * 
 *   destroy raw body
 *
 *    body (BODY_RAW *): raw body
 */
void oatuh_destroy_raw_body(BODY_RAW *body)
{
	if (!body)
		return ;
	if (body->body)
		free(body->body);
	free(body);
}

/*
 * oatuh_destroy_body ()
 * 
 *   destroy any body
 *
 *    body (void *): body of any type
 */
void oatuh_destroy_body(void *body)
{
	BODY_TYPE type;

	type = *(int *) body;
	if (type == RAW)
		oatuh_destroy_raw_body((BODY_RAW *)body);
}

/*
 * oatuh ()
 *
 *   request REST API
 *
 *    req (REQUEST *): request Block pointer
 *    return (int): error code on fail, NO_ERROR on success
 */
int		oatuh(REQUEST *req)
{
    int error_code;

    error_code = oatuh_parser(req);
    if (error_code != NO_ERROR)
        return error_code;

    error_code = connect_socket(req);
    if (error_code != NO_ERROR)
        return error_code;

	if (req->scheme == HTTPS)
	{
		error_code = connect_ssl(req);
		if (error_code != NO_ERROR)
			return error_code;
	}

	write_request(req);

	error_code = read_response(req);
	if (error_code != NO_ERROR)
		return error_code;

    close(req->connection.socket);
	if (req->scheme == HTTPS)
	{
		if (req->connection.tls.ssl)
			SSL_free(req->connection.tls.ssl);
		if (req->connection.tls.ctx)
			SSL_CTX_free(req->connection.tls.ctx);
	}
	return NO_ERROR;
}