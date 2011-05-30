/* $Id$ */
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <assert.h>

#include "azlist.h"
#include "azbuffer.h"

#include "scmessage.h"
#include "supports.h"

#include "appconfig.h"
#include "config.h"

#include "sclog.h"

#include "connection.h"

////////////////////

struct _sc_aggregator_connection {
    int socket;
    //
    char *host;
    int port;
};

typedef struct _sc_aggregator_connection sc_aggregator_connection;

sc_aggregator_connection*
sc_aggregator_connection_new(const char* host, int port)
{
    sc_aggregator_connection* conn = (sc_aggregator_connection*)malloc(sizeof(sc_aggregator_connection));
    if (conn) {
        conn->socket = -1;
	conn->host   = strdup(host);
	conn->port   = port;
    }
    return conn;
}

int
sc_aggregator_connection_open(sc_aggregator_connection* conn)
{
    struct addrinfo hints, *ai, *ai0 = NULL;
    int err, s = -1;
    char sport[16];

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = PF_UNSPEC;
    hints.ai_flags = AI_NUMERICSERV;
    hints.ai_socktype = SOCK_STREAM;

    snprintf(sport, sizeof(sport), "%d", conn->port);

    err = getaddrinfo(conn->host, sport, &hints, &ai0);
    if (err) {
        sc_log(LOG_DEBUG, "getaddrinfo: %s", gai_strerror(err));
        return -1;
    }

    for (ai = ai0; ai; ai = ai->ai_next) {
        s = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
	if (s < 0) {
	    continue;
	}

	if (connect(s, ai->ai_addr, ai->ai_addrlen) < 0) {
	    close(s);
	    continue;
	}
	conn->socket = s;
	break;
    }
    freeaddrinfo(ai0);

    return conn->socket != -1 ? 0 : -1;
}

int
sc_aggregator_connection_is_opened(sc_aggregator_connection* conn)
{
    if (!conn) { return -1; }
    return conn->socket != -1 ? 1 : 0;
}

int
sc_aggregator_connection_close(sc_aggregator_connection* conn)
{
    if (!conn) { return -1; }
    close(conn->socket);
    conn->socket = -1;
    return 0;
}

int
sc_aggregator_connection_send_message(sc_aggregator_connection* conn, sc_message_0* msg)
{
    int ret = 0;
    int32_t len = ntohl(msg->length);

    sc_log(LOG_DEBUG, "send_message: code = %d, channel = %d, length = %ld", ntohs(msg->code), ntohs(msg->channel), len);
    if ((ret = sendall(conn->socket, msg, len + offsetof(sc_message_0, content), 0)) <= 0) {
        close(conn->socket);
	conn->socket = -1;
        perror("sendall");
        sc_log(LOG_DEBUG, "sending error");
        return -1;
    }
    sc_log(LOG_DEBUG, "send_message: done");

    return 0;
}

int
sc_aggregator_connection_receive_message(sc_aggregator_connection* conn, sc_message_0** pmsg)
{
    char buf[offsetof(sc_message_0, content)];
    sc_message_0* m = (sc_message_0*)buf;
    int32_t len = 0;
    int n;

    n = recvall(conn->socket, buf, sizeof(buf), 0);
    if (n < 0) {
        close(conn->socket);
	conn->socket = -1;
        perror("recvall");
        return -1;
    } else if (n == 0) {
        sc_log(LOG_DEBUG, "closed");
	return -4;
    }

    len = ntohl(m->length);

    m = sc_message_0_new(len);
    if (!m) {
        return -2;
    }

    memcpy(m, buf, sizeof(buf));
    if (len > 0) {
        if (recvall(conn->socket, m->content, len, 0) <= 0) {
            return -3;
        }
    }

    *pmsg = m;

    return 0;
}

void
sc_aggregator_connection_destroy(sc_aggregator_connection* conn)
{
    sc_aggregator_connection_close(conn);

    free(conn->host);
    free(conn);
}