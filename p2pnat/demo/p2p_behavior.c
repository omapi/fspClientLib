#include "global.h"

/*{
    "p2p": true,
    "message": "dns resolv fail",
    "report": [
        {
            "server": "192.168.130.80",
            "big_ed": "192.168.20.64:5566",
            "big_reg": "success",
            "big_kp": "success",
            "small_ed": "192.168.20.64:5566",
            "small_reg": "success",
            "small_kp": "success"
        },
        {
            "server": "192.168.130.80",
            "big_ed": "192.168.20.64:5566",
            "big_reg": "success",
            "big_kp": "success",
            "small_ed": "192.168.20.64:5566",
            "small_reg": "success",
            "small_kp": "success"
        }
    ]
}*/
int p2p_behavior_test()
{
	int peer_fd, rc, i;
	char server_ip_pool[3][25];
	memset(server_ip_pool, 0, sizeof(server_ip_pool));
	p2p_demo_set_dns_servers("205.251.193.226", "168.95.1.1", "8.8.8.8");
	if(strlen(server_address) == 0){
		strcpy(server_address, "p2p.newrocktech.com");
	}
	snprintf(report_message+strlen(report_message), 1024-strlen(report_message), "{\"p2p\":true");
	rc = p2p_demo_dns_resove(server_address, server_ip_pool);
	if(rc != 0){
		snprintf(report_message+strlen(report_message), 1024-strlen(report_message), ",\"message\":\"dns resolv fail\"");
	}else{
		int more = 0;
		server_port = 7042;
		snprintf(report_message+strlen(report_message), 1024-strlen(report_message), ",\"report\":[");
		for(i = 0; i < 3; i++){
			if(strlen(server_ip_pool[i]) == 0)
				continue;
			if(more > 0)
				strcat(report_message, ",");
			p2p_behavior_test_server(server_ip_pool[i]);
			more++;
		}
		snprintf(report_message+strlen(report_message), 1024-strlen(report_message), "]");
	}
	snprintf(report_message+strlen(report_message), 1024-strlen(report_message), "}");
	if(log_level > 1)
		printf("\n%s\n", report_message);
	peer_fd = new_udp_socket(client_port, NULL);
	if(peer_fd <= 0){
		if(log_level > 1)
			printf("open peer_fd socket fail for send report_message\n");
	}else{
		report_test_result(peer_fd, 6908, report_message);
	}
	return 0;
}

int p2p_behavior_test_server(char *p2pServer)
{
	int client_fd, rc;
	int log_type = P2P_LOG_TYPE_NONE;
	client_port = 0;
	if(debug_output >= 1)
		log_type = P2P_LOG_TYPE_SCREEN;
	rc = p2p_init(NULL, "p2ptest", log_type, log_level, p2pServer, server_port);
	if(rc != 0)
		return -1;
	snprintf(report_message+strlen(report_message), 1024-strlen(report_message), "{\"server\":\"%s\",", p2pServer);
	while(1){
		char ed[25] = {0};
		char ped[25] = {0};
		client_fd = new_udp_socket(client_port, NULL);
		if(client_fd <= 0){
			snprintf(report_message+strlen(report_message), 1024-strlen(report_message), "\"%s_reg\":\"fail\",\"%s_kp\":\"fail\"", client_port==0?"big":"small", client_port==0?"big":"small");
			if(client_port == 0){
				client_port = 2001+time(NULL)%1000;
				strcat(report_message, ",");
				continue;
			}else{
				break;
			}
		}
		rc = p2p_demo_test_reg(client_fd, 10, ed, ped);
		if(log_level > 1)
		printf("p2p client %s(%s) register %s\n", ed, ped, rc==0?"success":"fail");
		snprintf(report_message+strlen(report_message), 1024-strlen(report_message), "\"%s_ed\":\"%s\",\"%s_ped\":\"%s\",", client_port==0?"big":"small", ed, client_port==0?"big":"small", ped);
		snprintf(report_message+strlen(report_message), 1024-strlen(report_message), "\"%s_reg\":\"%s\",", client_port==0?"big":"small", rc==0?"success":"fail");
		rc = test_send_mini_packet(client_fd, p2pServer, server_port);
		if(log_level > 1)
		printf("p2p client send to %s keep %s\n", p2pServer, rc==0?"success":"fail");
		snprintf(report_message+strlen(report_message), 1024-strlen(report_message), "\"%s_kp\":\"%s\"", client_port==0?"big":"small", rc==0?"success":"fail");
		close(client_fd);
		if(client_port == 0){
			client_port = 2001+time(NULL)%1000;
			strcat(report_message, ",");
		}else{
			break;
		}
	}
	snprintf(report_message+strlen(report_message), 1024-strlen(report_message), "}");
	return 0;
}

int p2p_demo_test_reg(int client_fd, int timeout, char *ed, char *ped)
{
	int status, time_count, rc;
	char cid[41];
	char udp_message[1024] = {0};
	struct sockaddr_in peer_addr;
	REND_EPT_HANDLE p2p_endpoint;
	// 创建客户端
	p2p_endpoint = new_rendezvous_endpoint(NULL, "P2PDEBUG", "default", "debug_ic", NULL, client_fd);
	if(p2p_endpoint == NULL){
		if(log_level > 1)
			printf("init rendezvous endpoint fail\n");
		return -1;
	}
	// 开始注册客户端
	rendezvous_endpoint_reg(p2p_endpoint);
	// 等待注册成功
	time_count = 0;
	while(1){
		rendezvous_status_handle();
		memset(udp_message, 0, sizeof(udp_message));
		rc = recv_udp_packet(client_fd, udp_message, sizeof(udp_message), &peer_addr, 1);
		if(rc > 0){
			handle_rendezvous_packet(client_fd, udp_message, &peer_addr);
			continue;
		}
		if(get_rendezvous_endpoint(p2p_endpoint, &status, cid, ed, ped) == 0){
			if(status == ENDPOINT_REGISTER_OK)
				break;
		}
		if(time_count > timeout){
			break;
		}
		time_count++;
	}
	// 释放客户端
	free_rendezvous_endpoint(p2p_endpoint);
	if(status != ENDPOINT_REGISTER_OK){
		if(log_level > 1)
			printf("p2p client register timeout\n");
		return -2;
	}
	return 0;
}

int test_send_mini_packet(int client_fd, char *ip, int port)
{
	int rc;
	char udp_message[1024] = {0};
	struct sockaddr_in peer_addr;
	rc = p2p_demo_send_packet(client_fd, ip, port, "192.168.22.25:1122");
	rc = recv_udp_packet(client_fd, udp_message, sizeof(udp_message), &peer_addr, 1);
	if(rc > 0){
		return 0;
	}
	return -1;
}

int p2p_demo_send_packet(int udp_fd, char *server_ip, int server_port, char *ped)
{
	int rc;
	struct sockaddr_in peer_addr;
	memset(&peer_addr, 0, sizeof(struct sockaddr_in));
	peer_addr.sin_family = AF_INET;
	peer_addr.sin_port = htons(server_port);
	peer_addr.sin_addr.s_addr = inet_addr(server_ip);
	rc = sendto(udp_fd, ped, strlen(ped), 0, (struct sockaddr *)&peer_addr, sizeof(struct sockaddr_in));
	if(rc < 0){
		if(log_level > 1)
		printf("send packet to %s:%d %d errno = %d, err:%s\n", server_ip, server_port, rc, errno, strerror(errno));
	}
	return rc;
}

void report_test_result(int peer_fd, int rport, char *result)
{
	int len;
	char remote[25];
	struct sockaddr_in remote_addr;
	sprintf(remote, "127.0.0.1:%d", rport);
	endpoint_to_address(remote, &remote_addr);
	len = sendto(peer_fd, result, strlen(result), 0, (struct sockaddr*)&remote_addr, sizeof(struct sockaddr_in));
	if(len <= 0){
		if(log_level > 1)
		printf("send fail to %s %d errno = %d, err:%s\n", remote, len, errno, strerror(errno));
	}
	return;
}
