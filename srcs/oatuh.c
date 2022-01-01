#include <oatuh.h>

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

static int write_request(REQUEST *req)
{
	char *header;
	int length;

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
	if (req->scheme == HTTPS)
		SSL_write(req->connection.tls.ssl, header, length);
	else
		write(req->connection.socket, header, length);
}

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
	printf("(%s)\n", header);
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
}

static int read_body_chunked(REQUEST *req)
{
	char **block;
	char buffer[2048];
	char *size, *temp, *pos;
	int bytes, length, block_pos, block_count;

	block_pos = 0;
	block_count = 1;
	block = (char **) calloc(block_count, sizeof(char *));

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
	memmove(req->response->buffer, req->response->buffer + strlen(size) + 2, req->response->buffer_length);
	req->response->buffer_length -= strlen(size) + 2;
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
		block[block_pos++] = req->response->buffer;
		req->response->buffer = strdup("");

		goto start;
	}
	temp = req->response->buffer;
	block[block_pos++] = memndup(req->response->buffer, length + 2);
	req->response->buffer = memndup(req->response->buffer + length + 2, req->response->buffer_length - length - 2);
	req->response->buffer_length -= length + 2;
	free(temp);

	goto start;

end:
	length = 0;
	for (int i = 0; i < block_pos; i++)
		length += strlen(block[i]) - 2;
	req->response->body = calloc(length, sizeof(char));
	for (int i = 0; i < block_pos; i++)
	{
		strncat(req->response->body, block[i], strlen(block[i]) - 2);
		free(block[i]);
	}
	req->response->length = length;
	free(block);
	free(req->response->buffer);

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

REQUEST	*oatuh_create()
{
	REQUEST *req;

	req = (REQUEST *) calloc(1, sizeof(REQUEST));
	return req;
}

void	oatuh_destroy(REQUEST *req)
{
	free(req);
}

int		oatuh_get_header_size(MAP_CHILD *header)
{
	int size;

	size = 0;
	while (header[size++].key);
	return size - 1;
}

char	*oatuh_get_header(MAP_CHILD *header, char *key)
{
	return get_header(header, key)->value;
}

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
		return ;
	}
	*header = (MAP_CHILD *) calloc (2, sizeof(MAP_CHILD));
	(*header)[0].key = strdup(key);
	(*header)[0].value = strdup(value);
}

/*
 * oatuh () - request REST API
 *
 * REQUEST *req: request target data 
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

	return NO_ERROR;
}