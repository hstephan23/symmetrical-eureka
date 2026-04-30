#ifndef TIDE_STATUS_H
#define TIDE_STATUS_H

typedef enum TideStatus {
    TIDE_OK = 0,
    TIDE_ERR_ALLOC,
    TIDE_ERR_IO,
    TIDE_ERR_INVALID,
    TIDE_ERR_UNSUPPORTED
} TideStatus;

const char *tide_status_string(TideStatus status);

#endif
