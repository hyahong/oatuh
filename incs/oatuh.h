/*
 * Copyright 2022 Yeoun 
 *
 *   Author: Yeoun 
 */

#ifndef OATUH_H
# define OATUH_H

# include <stdio.h>
# include <stdlib.h>
# include <unistd.h>
# include <string.h>
# include <sys/socket.h>
# include <arpa/inet.h>
# include <netdb.h>
# include <fcntl.h>

# include "request.h"
# include "response.h"

# define MEMORY_ALLOCATION_FAIL	1

# define INVALID_METHOD			100
# define INVALID_HOST			101

# define CONNECTION_FAIL		200
# define HANDSHAKE_FAIL			201

# define SSL_CTX_CREATE_FAIL	300

# define NO_ERROR				0

REQUEST	*oatuh_create();
void	oatuh_destroy(REQUEST *req);

int		oatuh_get_header_size(MAP_CHILD *header);
char	*oatuh_get_header(MAP_CHILD *header, char *key);
void	oatuh_set_header(MAP_CHILD **header, char *key, char *value);

void	*oatuh_create_raw_body(char *raw, int length);
void    oatuh_destroy_raw_body(BODY_RAW *body);
void    oatuh_destroy_body(void *body);

int     oatuh(REQUEST *req);

#endif