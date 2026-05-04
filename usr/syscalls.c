// usr/syscalls.c
#include <sys/stat.h>
#include <errno.h>

void *_sbrk(int incr)
{
    extern char _end; // 由链接脚本定义，指向堆的起始地址
    static char *heap_end = &_end;
    char *prev_heap_end = heap_end;
    heap_end += incr;
    return (void *)prev_heap_end;
}