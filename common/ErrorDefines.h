#ifndef ERRORS_DEFINES_H
#define ERRORS_DEFINES_H

#define LOGI(LOGTAG, ...)                          \
       do {                                        \
            printf(LOGTAG);                        \
            printf(" %s:%d ", __FILE__, __LINE__); \
            printf(__VA_ARGS__);                   \
            printf("\n");                          \
       } while(0);

#define LOGD(LOGTAG, ...)                         
//#define LOGD(LOGTAG, ...)                          \
       do {                                        \
            printf(LOGTAG);                        \
            printf(" %s:%d ", __FILE__, __LINE__); \
            printf(__VA_ARGS__);                   \
            printf("\n");                          \
       } while(0);

#define LOGE(LOGTAG, ...)                                             \
       do {                                                           \
            printf(LOGTAG);                                           \
            printf(" Critical Error in: %s:%d ", __FILE__, __LINE__); \
            printf(__VA_ARGS__);                                      \
            printf("\n");                                             \
       } while(0);

#endif
