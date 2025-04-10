#include "log.h"



FILE *log_fd;
void set_log_fd(FILE *fd)
{
    log_fd = fd;
}

int chan_log(int level, const char *format, ...)
{
#ifndef QUIET
    if (log_fd == NULL)
    {
        log_fd = stderr; // Default to stderr if log_fd is not set
    }
    if (level >= LOGLEVEL) {
        va_list args;
        int result;
        
        va_start(args, format);
        result = vfprintf(log_fd, format, args);
        va_end(args);
        
        return result;
    }
#endif
    (void)format; // Suppress unused parameter warning
    return 0;
}

int debug(const char *format, ...)
{
#ifndef QUIET
    if (log_fd == NULL)
    {
        log_fd = stderr; // Default to stderr if log_fd is not set
    }
    if (LOG_DEBUG >= LOGLEVEL) {
        va_list args;
        int result;
        
        va_start(args, format);
        // fprintf(log_fd, "[DEBUG] ");
        result = vfprintf(log_fd, format, args);
        va_end(args);
        
        return result;
    }
#endif
    (void)format; // Suppress unused parameter warning
    return 0;
}

int info(const char *format, ...)
{
#ifndef QUIET
    if (log_fd == NULL)
    {
        log_fd = stderr; // Default to stderr if log_fd is not set
    }
    if (LOG_INFO >= LOGLEVEL) {
        va_list args;
        int result;
        
        va_start(args, format);
        // fprintf(log_fd, "[INFO] ");
        result = vfprintf(log_fd, format, args);
        va_end(args);
        
        return result;
    }
#endif
    (void)format; // Suppress unused parameter warning
    return 0;
}

int error(const char *format, ...)
{
#ifndef QUIET
    if (log_fd == NULL)
    {
        log_fd = stderr; // Default to stderr if log_fd is not set
    }
    if (LOG_ERROR >= LOGLEVEL) {
        va_list args;
        int result;
        
        va_start(args, format);
        // fprintf(log_fd, "[ERROR] ");
        result = vfprintf(log_fd, format, args);
        va_end(args);
        
        return result;
    }
#endif
    (void)format; // Suppress unused parameter warning
    return 0;
}

int debug_np(const char *format, ...)
{
#ifndef QUIET
    if (log_fd == NULL)
    {
        log_fd = stderr; // Default to stderr if log_fd is not set
    }
    if (LOG_DEBUG >= LOGLEVEL) {
        va_list args;
        int result;
        
        va_start(args, format);
        result = vfprintf(log_fd, format, args);
        va_end(args);
        
        return result;
    }
#endif
    (void)format; // Suppress unused parameter warning
    return 0;
}

// void print_bools(bool *bits, int size)
// {
    
//     for (int i = 0; i < size; i++)
//     {
//         chan_log(LOG_DEBUG, "%d", bits[i]);
//         if (i == 3)
//         {
//             chan_log(LOG_DEBUG, " ");
//         }

//         // if (i % 32 == 31)
//         // {
//         //     printf("\n");
//         // }
//     }
// }

