/*
 * This is an implemetation of Viscous protocol.
 * Copyright (C) 2017  Abhijit Mondal
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


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
