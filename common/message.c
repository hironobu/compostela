/* $Id$ */
#include <sys/types.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include "message.h"

////////////////////

sc_log_message*
sc_log_message_new(ssize_t content_size)
{
    sc_log_message *msg = (sc_log_message*)malloc(offsetof(sc_log_message, content) + content_size);
    return msg;
}

void
sc_log_message_destroy(sc_log_message* msg)
{
    free(msg);
}

sc_log_message*
sc_log_message_resize(sc_log_message* msg, ssize_t newsize)
{
    sc_log_message *ret = (sc_log_message*)realloc(msg, offsetof(sc_log_message, content) + newsize);
    return ret;
}
