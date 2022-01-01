#include <stdio.h>
#include <unistd.h>

#define UNICODE_SIZE_ONE_MASK 0x80
#define UNICODE_SIZE_TWO_MASK 0x20
#define UNICODE_SIZE_THREE_MASK 0x10
#define UNICODE_SIZE_FOUR_MASK 0x8
#define UNICODE_FOLLOW_MASK 0xC0

int usize(char c)
{
    if (!(c & UNICODE_SIZE_ONE_MASK) || (c & UNICODE_FOLLOW_MASK) == 0x80)
        return 1;
    if (!(c & UNICODE_SIZE_TWO_MASK))
        return 2;
    if (!(c & UNICODE_SIZE_THREE_MASK))
        return 3;
    if (!(c & UNICODE_SIZE_FOUR_MASK))
        return 4;
    return 1;
}

int uwrite(int fd, char *str, int length)
{
    int size;
    int off;

    off = str + length;
    while (str < off)
    {
        size = usize(*str);
        // TODO
        write(fd, str, size);
        str += size;
    }
    return 0;
}

int main(void)
{
    char *test = "가나다라";
    uwrite(1, test, 12);
    return 0;
}