#ifndef LOG_H
#define LOG_H

#include <stdarg.h>

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>

// Log levels
#define LOG_DEBUG 0
#define LOG_INFO 1
#define LOG_ERROR 2

#define LOGLEVEL LOG_DEBUG
#define QUIET


extern FILE *log_fd;
void set_log_fd(FILE *fd);

// static inline FILE *get_log_fd()
// {
//     // If ite
//     return log_fd;
// }

int chan_log(int level, const char *format, ...);

int debug(const char *format, ...);

int info(const char *format, ...);

int error(const char *format, ...);

int debug_np(const char *format, ...);


// Define debug macro based on QUIET flag
#ifdef QUIET
#define debug_macro(...)  // Do nothing in quiet mode
#define info_macro(...)   // Do nothing in quiet mode
#else
#define debug_macro(...) debug(stderr, __VA_ARGS__)
#define info_macro(...) info(stderr, __VA_ARGS__)
#endif


#endif // LOG_H