/*
 *
 * Author: Sylvain Afchain <safchain@gmail.com>, (C) 2008 - 2013
 *
 * Copyright: See COPYING file that comes with this distribution
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <types.h>

#include <mm.h>

#include "udp.h"

int net_ubind(const char *ip, uint16_t port) {
        struct sockaddr_in sin;
        int sock, size, rtv;

        /* udp socket */
        if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
                rtv = -1;
                goto clean;
        }

        /* set size of an udp packet */
        size = 512;
        if (setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &size, sizeof (int)) == -1) {
                rtv = -1;
                goto clean;
        }
        if (setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &size, sizeof (int)) == -1) {
                rtv = -1;
                goto clean;
        }

        /* which port to bind */
        memset(&sin, 0, sizeof (sin));
        sin.sin_family = AF_INET;
        sin.sin_port = htons(port);

        /* which ip to bind */
        if ((sin.sin_addr.s_addr = inet_addr(ip)) == INADDR_NONE) {
                rtv = -1;
                goto clean;
        }

        /* bind on it */
        if (bind(sock, (struct sockaddr *) & sin, sizeof (struct sockaddr_in)) == -1) {
                rtv = -1;
                goto clean;
        }

        rtv = sock;

clean:
        return rtv;
}

int net_tbind(const char *ip, uint16_t port) {
        struct sockaddr_in sin;
        int sock, on, rtv;

        /* udp socket */
        if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
                rtv = -1;
                goto clean;
        }

        /* against "address already in use" */
        on = 1;
        if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof (int)) == -1) {
                rtv = -1;
                goto clean;
        }

        /* which port to bind */
        memset(&sin, 0, sizeof (sin));
        sin.sin_family = AF_INET;
        sin.sin_port = htons(port);

        /* which ip to bind */
        if ((sin.sin_addr.s_addr = inet_addr(ip)) == INADDR_NONE) {
                rtv = -1;
                goto clean;
        }

        /* bind on it */
        if (bind(sock, (struct sockaddr *) & sin, sizeof (struct sockaddr_in)) == -1) {
                rtv = -1;
                goto clean;
        }

        rtv = sock;

clean:
        return rtv;
}

SOCKUTP *net_bind(const char *ip, uint16_t port) {
        SOCKUTP *sockutp;

        /* allocation of a next sock */
        if ((sockutp = calloc(1, sizeof (SOCKUTP))) == NULL)
                return NULL;

        /* la socket en udp */
        if ((sockutp->udp_sock = net_ubind(ip, port)) == -1) {
                free(sockutp);
                return NULL;
        }

        /* la socket en tcp */
        if ((sockutp->tcp_sock = net_tbind(ip, port)) == -1) {
                free(sockutp);
                return NULL;
        }

        return sockutp;
}

void net_unbind(SOCKUTP *sockutp) {
        free(sockutp);
}
