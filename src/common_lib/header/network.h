/*
 * network.h
 *
 *  Created on: 13-Feb-2016
 *      Author: abhijit
 */

#ifndef NETWORK_H_
#define NETWORK_H_
#include <common.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

#define app_tcp_listener_str(_x, _y) (app_tcp_listener_ip(_x, inet_addr(_y)))
#define app_tcp_listener(_x) (app_tcp_listener_ip(_x,htonl(INADDR_ANY) ))
int app_tcp_listener_ip(int port , in_addr_t ip);

#define app_udp_listener_str(_x, _y) (app_udp_listener_ip(_x, inet_addr(_y)))
#define app_udp_listener(_x) (app_udp_listener_ip(_x,htonl(INADDR_ANY) ))
int app_udp_listener_ip(int port , in_addr_t ip);

#define app_tcp_client_connect_str(_x, _y) (app_tcp_client_connect_ip(_x, inet_addr(_y)))
int app_tcp_client_connect_ip(int port, in_addr_t ip);

ssize_t app_send(int sockfd, const void *buf, size_t len, int flags);


#ifdef __cplusplus
}
#endif

#endif /* NETWORK_H_ */
