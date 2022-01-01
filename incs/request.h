#ifndef REQUEST_H
# define REQUEST_H

# include <openssl/ssl.h>
# include <openssl/err.h>

# include "response.h"

# define METHOD_NUMBER	7

typedef enum method METHOD;
enum method
{
	GET,
	POST,
	PUT,
	PATCH,
	DELETE,
	HEAD,
	OPTIONS
};

typedef struct type TYPE;
struct type
{
	char name[10];
	METHOD method;
	int request_body;
	int response_body;
};

typedef enum scheme SCHEME;
enum scheme
{
	HTTP,
	HTTPS
};

/*
 * Body type
 */
typedef struct body_raw BODY_RAW;
struct body_raw
{
	char *body;
	int length;
};

/*
 * connection
 */
typedef struct tls_socket TLS_SOCKET;
struct tls_socket
{
	SSL_CTX* ctx;
	char *subject;
	char *issuer;
	SSL *ssl;
};

typedef struct connection CONNECTION;
struct connection
{
	int socket;

	TLS_SOCKET tls;
};

/*
 * request block 
 */
typedef struct request REQUEST;
struct request
{
	CONNECTION connection;

	char *method;
	char *uri;

	TYPE *type;

	SCHEME scheme;
	char *authority;
	char *host;
	char *ip;
	char *port;
	char *path;

    MAP_CHILD *header;
	void *body;

    RESPONSE *response;
};

#endif