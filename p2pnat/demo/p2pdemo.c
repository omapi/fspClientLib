#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <netinet/in.h> //for sockaddr_in
#include <sys/types.h>  //for socket
#include <sys/socket.h> //for socket
#include "p2p_api.h"

#define VERSION "1.0.0"

int debug_output = 0;
int log_level = 0;
char demo_log_name[128] = {0};
int random_cid = 1;
int server_port = 0;
char server_address[24] = "";
char client_id[41] = {0};
char client_key[33] = {0};
char client_service[20] = "SIP";
int client_port = 0;
char g_tid[41] = {0};

static char Usage[] =
  "p2p rendezvous\n"
  "Usage: p2pnat [options] \n"
  "Options:\n"
  "        -s server   P2PNAT server address to use\n"
  "        -p port     P2PNAT server port\n"
  "        -c tid      cid to connect\n"
  "        -i cid      client id\n"
  "        -k key      client key\n"
  "        -S service  client service\n"
  "        -L port     client bind port\n"
  "        -d          output log info to screen\n"
  "        -t          test mode, donn't crypt message\n"
  "        -l level    log level\n"
  "        -N name     log app name\n";

int ttest(void)
{
	int nr_of_bytes;
	char fingprint[41] = {0};
	unsigned char *key="12345678901234561234567890123456";
	unsigned char *data="abcdefg1234567890abchello,are you ok.haha";
	unsigned char *out, *out2;
	out = (unsigned char *)malloc(strlen(data)/16*16+16);
	out2 = (unsigned char *)malloc(strlen(data)/16*16+16);
	nr_of_bytes = message_encrypt(key, 32, data, strlen(data), out);

	message_decrypt(key, 32, out, nr_of_bytes, out2);
	printf("out2:%s\n", out2);
	generate_fingprint("somedata somedata", "somekeysomekey", fingprint, 41);
	printf("fingprint:%s\n", fingprint);
	return 0;
}

int phrase_argv(int argc, char *argv[])
{
	int rc = 0;

	while ((rc = getopt(argc, argv, "k:L:S:N:l:c:p:s:i:dt")) != -1) {
		switch(rc) {
		case 'd':
			debug_output = 1;
			break;
		case 'l':
			log_level = atoi(optarg);
			break;
		case 'S':
			strcpy(client_service, optarg);
			break;
		case 'L':
			client_port = atoi(optarg);
			break;
		case 'N':
			strcpy(demo_log_name, optarg);
			break;
		case 's':
			strcpy(server_address, optarg);
			break;
		case 'p':
			server_port = atoi(optarg);
			break;
		case 't':
			debug_output = 2;
			break;
		case 'i':
			random_cid = 0;
			strcpy(client_id, optarg);
			break;
		case 'k':
			strcpy(client_key, optarg);
			break;
		case 'c':
			strcpy(g_tid, optarg);
			break;
		default:
			fprintf(stderr,"%s\n", Usage);
			exit(1);
		}
	}
	return 0;
}

REND_CONN_HANDLE check_endpoint_and_connect(REND_EPT_HANDLE endpoint, char *tid)
{
	int status;
	char cid[41], ped[25];
	REND_CONN_HANDLE conn = NULL;
	if(get_rendezvous_endpoint(endpoint, &status, cid, NULL, ped) == 0){
		if(status == ENDPOINT_REGISTER_OK)
			conn = new_rendezvous_connection(endpoint, tid, client_service, "default", NULL);
		if(log_level>=5)
			printf("cid:%s, status:%d, ped:%s\n", cid, status, ped);
	}
	return conn;
}

void check_connection_and_send(int peer_fd, REND_CONN_HANDLE conn)
{
	static int num_send;
	int rc, status = 0;
	char udp_message[1024] = {0};
	char r_ed[25];
	char r_ped[25];
	if(num_send > 0){
		printf("send finish\n");
		exit(0);
	}
	if(get_rendezvous_connection(conn, &status, r_ed, r_ped) == 0){
		// printf("========conn:%d, %s, %s\n", status, r_ed, r_ped);
		if(status == CONNECTION_OK) {
			//如果连接建立成功，则可以发送应用数据
			struct sockaddr_in remote_addr;
			strcpy(udp_message, "application data application data application data");
			if(strlen(r_ped) > 0){
				printf("send data to %s\n", r_ped);
				rc = endpoint_to_address(r_ped, &remote_addr);
				if(rc == 0)
					sendto(peer_fd, udp_message, strlen(udp_message), 0, (struct sockaddr *)&remote_addr, sizeof(struct sockaddr_in));
			}
			if(strlen(r_ed) > 0){
				printf("send data to %s\n", r_ed);
				rc = endpoint_to_address(r_ed, &remote_addr);
				if(rc == 0)
					sendto(peer_fd, udp_message, strlen(udp_message), 0, (struct sockaddr *)&remote_addr, sizeof(struct sockaddr_in));
			}
			num_send++;
		}else if(status == CONNECTION_FAILED){
			exit(-1);
		}
	}
}

int main(int argc, char *argv[])
{
	int rc = 0;
	char ed[25];
	char udp_message[1024] = {0};
	rendezvous_packet_t resp_packet;
	struct sockaddr_in peer_addr;
	int peer_fd;
	REND_EPT_HANDLE p2p_endpoint;
	REND_CONN_HANDLE p2p_conn = NULL;
	ttest();
	phrase_argv(argc, argv);
	if(strlen(demo_log_name) > 0)
		rc = p2p_init(NULL, demo_log_name, log_level, server_address, server_port);
	else
		rc = p2p_init(NULL, "p2pdemo", log_level, server_address, server_port);
	if(rc != 0){
		fprintf(stderr, "init p2p failed\n");
		return -1;
	}
	if(debug_output >= 2)
		set_p2p_option(0, 1);
	if(debug_output >= 1){
		set_event_log_type(eLOG_TYPE_SCREEN);
		if(log_level > 0)
		show_event_log_config();
	}
	if(log_level > 0)
	debug_log(eLOG_COMP_API, 0, __FILE__, __LINE__, "demo server version %s", VERSION);

	peer_fd = new_udp_socket(client_port, NULL);

	//初始化约会客户端
	if(random_cid == 1){
		p2p_endpoint = new_rendezvous_endpoint(NULL, client_service, NULL, NULL, client_key, peer_fd);
	}else{
		p2p_endpoint = new_rendezvous_endpoint(client_id, client_service, NULL, NULL, client_key, peer_fd);
	}
	if(p2p_endpoint == NULL) {
		close(peer_fd);
		printf("init rendezvous endpoint fail\n");
		return 0;
	}
	rendezvous_endpoint_reg(p2p_endpoint);
	while(1){
		rendezvous_status_handle();

		//如果指定了目标tid，则注册成功后去连接它，用于测试
		if(strlen(g_tid) > 0 && p2p_conn == NULL){
			p2p_conn = check_endpoint_and_connect(p2p_endpoint, g_tid);
		}

		memset(udp_message, 0, sizeof(udp_message));
		rc = recv_udp_packet(peer_fd, udp_message, sizeof(udp_message), &peer_addr);
		if(rc <= 0){
			check_connection_and_send(peer_fd, p2p_conn); //检查连接，如果成功则发送测试数据，用于测试
			continue;
		}

		//解析数据包，判断是不是p2p协议数据
		rc = decode_rendezvous_packet(udp_message, &peer_addr, &resp_packet);
		if(rc == 0){
			rendezvous_message_handle(peer_fd, &resp_packet);
			continue;
		}

		//被连接端，响应连接端发送的应用数据，用于测试
		if(p2p_conn == NULL){
			address_to_endpoint(&peer_addr, ed);
			memcpy(udp_message, "reply ", 6);
			memcpy(udp_message+6, ed, strlen(ed));
			sendto(peer_fd, udp_message, strlen(udp_message), 0, (struct sockaddr *)&peer_addr, sizeof(struct sockaddr_in));
		}else{
			printf("recv from %s == %s\n", address_to_endpoint(&peer_addr, ed), udp_message);
		}
	}
	free_rendezvous_endpoint(p2p_endpoint);
	close(peer_fd);
	return 0;
}
