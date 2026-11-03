#ifndef SHIM_LWIP_SOCKETS_H
#define SHIM_LWIP_SOCKETS_H
/* lwIP sockets API == BSD sockets, so just pull in the host's. */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#endif
