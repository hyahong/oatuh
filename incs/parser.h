#ifndef PARSER_H
# define PARSER_H

# include "request.h"

MAP_CHILD   *get_header(MAP_CHILD *header, char *key);

int		    oatuh_parser(REQUEST *req);

#endif