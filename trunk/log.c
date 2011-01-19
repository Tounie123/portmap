/**
 *
 * portmap log part
 *
 * author liyangguang (liyangguang@software.ict.ac.cn)
 *
 * date 2011-01-14 15:39:00
 *
 * changes
 */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <stdarg.h>
#include <errno.h>
#include <time.h>



FILE *g_log_file;
int
log_init(const char *log_file)
{
    assert(log_file != NULL); 
    g_log_file = fopen(log_file, "w+"); 

    if (g_log_file == NULL) {
    
        fprintf(stderr, "Failed to open %s .error:%s \n", log_file, strerror(errno));
        return -1;
    }
    return 0;
}

void
log_write(const char *level, const char *func, const char *file, int line, const char *fmt, ...)
{
    assert(g_log_file != NULL);
    va_list vl;
    va_start(vl, fmt);
    char tmp[1000];
    time_t t;
    struct tm *tt;
    char time_buf[100];

    t = time(NULL);
    tt = localtime(&t);
    strftime(time_buf, 100, "%Y-%m-%d %H:%M:%S", tt);


    vsnprintf((char *)tmp, 1000, fmt, vl);
    fprintf(g_log_file, "%s %s [func:%s()] [%s:%d] %s\n", level, time_buf, func, file, line, tmp);
    fflush(g_log_file);
    va_end(vl);
}

void
log_close()
{

    if (g_log_file != NULL)
        fclose(g_log_file);
}
