#include "tide/status.h"

const char *tide_status_string(TideStatus status)
{
    switch (status) {
    case TIDE_OK:
        return "ok";
    case TIDE_ERR_ALLOC:
        return "allocation failed";
    case TIDE_ERR_IO:
        return "i/o failed";
    case TIDE_ERR_INVALID:
        return "invalid input";
    case TIDE_ERR_UNSUPPORTED:
        return "unsupported operation";
    }

    return "unknown error";
}
