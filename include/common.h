#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>

//#define ENABLE_LOGGING 

#ifdef ENABLE_LOGGING
    #define LOG_PRINT(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
    #define LOG_PRINT(fmt, ...) ((void)0)
#endif 

#endif 