#ifndef LIBMINIUV_UTIL_H
#define LIBMINIUV_UTIL_H
#define DEBUG
#ifdef DEBUG
#define miniuv_debug(fmt, arg...) \
    printf("[%s.%u]: " fmt "\n", __FUNCTION__, __LINE__, ##arg)
#else
    #define miniuv_debug(fmt,...)
#endif
#define FAILED -1
#define SUCCESS 0
#define MINIUV_STOP_RUNNING 0x00000001


#endif