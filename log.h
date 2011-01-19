/**
 *
 * portmap log part
 *
 * author: liyangguang (liyangguang@software.ict.ac.cn)
 *
 * date: 2011-01-14 15:27:01
 *
 * changes
 */
#ifndef _PORTMAP_LOG_H
#define _PORTMAP_LOG_H

void log_write(const char *level, const char *func, const char *file, int line, const char *fmt,
        ...);

int log_init(const char *log_file);
int log_close();
#define LOG_INFO(fmt, args...) \
    do {                       \
        log_write("LOG_INFO", __func__, __FILE__, __LINE__, fmt, ##args); \
    }while(0)
#ifdef DEBUG
#define LOG_DEBUG(fmt, args...) \
    do { \
        log_write("LOG_DEBUG", __func__, __FILE__, __LINE__, fmt, ##args); \
    }while(0)
#else
#define LOG_DEBUG(fmt, args...) do{}while(0)
#endif /* ifdef DEBUG */

#define LOG_ERR(fmt, args...)                                  \
    do{   \
        log_write("LOG_ERR", __func__, __FILE__, __LINE__, fmt, ##args); \
    }while(0)

#endif /* _PORTMAP_LOG_H */


