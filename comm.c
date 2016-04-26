#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h> //for sockaddr_in
#include <sys/types.h>  //for socket
#include <sys/socket.h> //for socket
#include <string.h>
#include <sys/fcntl.h>
#include "errno.h"

int new_udp_socket(int local_port, char *local_address)
{
	int udp_fd, rc;
	struct sockaddr_in addr;
	int flag = 1, len = sizeof(int);
	udp_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(udp_fd <= 0){
		printf("%s:%d open socket error", __FUNCTION__, __LINE__);
		return -1;
	}
	bzero(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(local_port);
	if(local_address != NULL && strlen(local_address) > 0){
		addr.sin_addr.s_addr = inet_addr(local_address);
	}else{
		addr.sin_addr.s_addr = htonl(INADDR_ANY);
	}
	if(setsockopt(udp_fd, SOL_SOCKET, SO_REUSEADDR, &flag, len) == -1){
		printf("%s:%d setsockopt error", __FUNCTION__, __LINE__);
		perror("setsockopt");
		close( udp_fd );
		return -1;
	}

	if((rc=bind(udp_fd, (struct sockaddr*)&addr, sizeof(addr))) < 0){
		printf("%s:%d socket binding error rc = %d", __FUNCTION__, __LINE__, rc);
		close( udp_fd );
		return -1;
	}
	rc = fcntl(udp_fd, F_SETFD, FD_CLOEXEC);
	return udp_fd;
}

int recv_udp_packet(int udp_fd, char *udp_message, int message_len, struct sockaddr_in *peer_addr)
{
	int rc;
	fd_set rset;
	struct timeval tval;
	socklen_t addr_len = sizeof(struct sockaddr_in);
	tval.tv_sec = 1;
	tval.tv_usec = 100*1000;
	FD_ZERO(&rset);
	FD_SET(udp_fd, &rset);
	rc = select(udp_fd+1, &rset, NULL, NULL, &tval);
	if(rc <= 0) {
		return -1;
	}
	memset(udp_message, 0, message_len);
	if(FD_ISSET(udp_fd, &rset)) {
		rc = recvfrom(udp_fd, udp_message, message_len, 0, (struct sockaddr *)peer_addr ,&addr_len); 
	}
	return rc;
}
