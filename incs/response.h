#ifndef RESPONSE_H
# define RESPONSE_H

typedef struct map_child MAP_CHILD;
struct map_child
{
    char *key;
    char *value;
};

typedef struct response RESPONSE;
struct response
{
    char *status_code;
    char *status;
    MAP_CHILD *header;

    char *buffer;
    int buffer_length;

    char *body;
    int length;
};

#endif