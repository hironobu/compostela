/* $Id$ */
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <netdb.h>

#include "scmessage.h"


enum { PORT = 8187 };

typedef struct _sc_follow_context {
    char *filename;
    int channel_code;
    off_t current_position;
    off_t filesize;
    int _fd;
} sc_follow_context;


////////////////////

typedef struct _sc_aggregator_connection {
    int socket;
} sc_aggregator_connection;

sc_aggregator_connection*
sc_aggregator_connection_new()
{
    sc_aggregator_connection* conn = (sc_aggregator_connection*)malloc(sizeof(sc_aggregator_connection));

    return conn;
}

int
sc_aggregator_connection_open(sc_aggregator_connection* conn, const char* addr, int port)
{
    struct addrinfo hints, *ai, *ai0 = NULL;
    int err, s = -1;
    char sport[16];

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = PF_UNSPEC;
    hints.ai_flags = AI_NUMERICSERV;
    hints.ai_socktype = SOCK_STREAM;

    snprintf(sport, sizeof(sport), "%d", port);

    err = getaddrinfo(addr, sport, &hints, &ai0);
    if (err) {
        fprintf(stderr, "getaddrinfo: %s", gai_strerror(err));
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
sc_aggregator_connection_send_message(sc_aggregator_connection* conn, sc_message* msg)
{
    ssize_t cb = 0, n = len;
    const char* p = (const char*)msg;

    while (n) {
        cb = send(conn->socket, p, n, 0);
	if (cb == -1) {
	    return -1;
	}

	//
	p += cb;
	n -= cb;
    }

    return 0;
}

void
sc_aggregator_connection_destroy(sc_aggregator_connection* conn)
{
    close(conn->socket);
    free(conn);
}

////////////////////

sc_follow_context*
sc_follow_context_new(const char* fname)
{
    sc_follow_context* cxt = NULL;
    
    cxt = (sc_follow_context*)malloc(sizeof(sc_follow_context));
    if (cxt) {
        memset(cxt, 0, sizeof(sc_follow_context));

        cxt->filename = strdup(fname);
	if (!cxt->filename) {
	    free(cxt);
	    cxt = NULL;
	}

        // we should read control files for 'fname'
    }

    return cxt;
}

int
sc_follow_context_open(sc_follow_context* cxt)
{
    struct stat st;

    cxt->_fd = open(cxt->filename, O_RDONLY);
    if (cxt->_fd <= 0) {
        return -1;
    }

    fstat(cxt->_fd, &st);
    cxt->filesize = st.st_size;
    lseek(cxt->_fd, cxt->current_position, SEEK_SET);
    return 0;
}

int
sc_follow_context_run(sc_follow_context* cxt, sc_aggregator_connection* conn)
{
    ssize_t csize = 2048;
    sc_message* msg = sc_message_new(size);

    while (1) {
	int cb = read(cxt->_fd, &msg->content, csize);
	if (cb <= 0) {
            sleep(1);
	    continue;
	}
	msg->length = cb;

        if (sc_aggregator_connection_send_message(conn, msg) != 0) {
	    // connection broken
	    break;
	}
        //
    }

    sc_message_destroy(buf);
}

void
sc_follow_context_destroy(sc_follow_context* cxt)
{
    free(cxt->filename);
    free(cxt);
}

int
main(int argc, char** argv)
{
    sc_follow_context* cxt = NULL;
    sc_aggregator_connection* conn = NULL;

    const char* fname = (argc > 1 ? argv[1] : "default");
    const char* servhost = (argc > 2 ? argv[2] : "log");

    cxt = sc_follow_context_new(fname);
    sc_follow_context_open(cxt);

    printf("cxt = %p\n", cxt);

    conn = sc_aggregator_connection_new();
    sc_aggregator_connection_open(conn, servhost, PORT);

    printf("conn = %p\n", conn);

    return sc_follow_context_run(cxt, conn);
}