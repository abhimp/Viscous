/*
 * network.c
 *
 *  Created on: 13-Feb-2016
 *      Author: abhijit
 */

#include <network.h>

int app_tcp_listener_ip(int port , in_addr_t ip)
{
        int listener_d = socket(AF_INET, SOCK_STREAM, 0);
        if (listener_d == -1)
                LOGW("Can't open socket");

        int optval = 1;
        setsockopt(listener_d, SOL_SOCKET, SO_REUSEADDR,
        	     (const void *)&optval , sizeof(int));
        // BIND
        struct sockaddr_in name;
        name.sin_family = AF_INET;
        name.sin_port = htons(port);
        name.sin_addr.s_addr = ip;
        int c = bind(listener_d, (struct sockaddr *)&name, sizeof(name));
        if(c){
            LOGW("Can't bind the port");
			return -1;
		}

        //LISTEN
        if(listen(listener_d, 10) == -1)
                LOGW("Can't listen");
        return listener_d;
}

int app_udp_listener_ip(int port , in_addr_t ip)
{
        int listener_d = socket(AF_INET, SOCK_DGRAM, 0);
        if (listener_d == -1)
                LOGW("Can't open socket");

        int optval = 1;
        // BIND
        struct sockaddr_in name;
        name.sin_family = AF_INET;
        name.sin_port = htons(port);
        name.sin_addr.s_addr = ip;
        int c = bind(listener_d, (struct sockaddr *)&name, sizeof(name));
        if(c){
            LOGW("Can't bind the port");
			return -1;
		}

        setsockopt(listener_d, SOL_SOCKET, SO_REUSEADDR,
        	     (const void *)&optval , sizeof(int));
        //LISTEN
//        if(listen(listener_d, 10) == -1)
//                LOGW("Can't listen");
        return listener_d;
}

int app_tcp_client_connect_ip(int port, in_addr_t ip)
{
	struct sockaddr_in server_addr;
	int client_fd = socket(AF_INET, SOCK_STREAM, 0);
	
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	server_addr.sin_addr.s_addr = ip;
	if(connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)))
	{
		LOGW("Could not connect\n");
		perror("Cloud not connect");
		return -1;
	}
	return client_fd;
}
