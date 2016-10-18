#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <signal.h>
#include <netinet/in.h> //for sockaddr_in
#include <sys/types.h>  //for socket
#include <sys/socket.h> //for socket
#include "p2p_api.h"

#define VERSION "1.0.0"

int debug_output = 0;
int free_endpoint = 0;
int log_level = 0;
char demo_log_name[128] = {0};
int random_cid = 1;
int server_port = 0;
char server_address[24] = "";
char client_id[41] = {0};
char client_key[33] = {0};
char client_ic[65] = {0};
char client_service[20] = "DMO";
int client_port = 0;
int keep_reg_interval = 0;
char g_tid[41] = {0};

static char Usage[] =
  "p2p rendezvous\n"
  "Usage: p2pnat [options] \n"
  "Options:\n"
  "        -s server   P2PNAT server address to use\n"
  "        -p port     P2PNAT server port\n"
  "        -c tid      cid to connect\n"
  "        -I ic       invite code\n"
  "        -i cid      client id\n"
  "        -k key      client key\n"
  "        -S service  client service\n"
  "        -L port     client bind port\n"
  "        -d          output log info to screen\n"
  "        -F          free endpoint while p2p connect ok\n"
  "        -t          test mode, donn't crypt message\n"
  "        -l level    log level\n"
  "        -T          test get device_id\n"
  "        -K          keep nat interval\n"
  "        -N name     log app name\n";

void print_trace( int signo )
{
	void *array[30];
	size_t size;
	char **strings;
	size_t i;
	int debug_flag = 1;

	size = backtrace (array, 30);
	strings = backtrace_symbols (array, size);

	if(debug_flag == 1)
	{
		printf ("Sig-%d  Obtained %zd stack frames.\n", signo, size);
		for (i = 0; i < size; i++)
			printf ("%s\n", strings[i]);
	}
	free(strings);
}

void sig_handler(int signo)
{
	print_trace( signo );
	exit(1);
}

int test_get_deviceId(void)
{
	char device_id[64], device_key[64];
	memset(device_id, 0, sizeof(device_id));
	memset(device_key, 0, sizeof(device_key));
	get_deviceId_key(device_id, device_key);
	printf("get device_id:%s, device_key:%s\n", device_id, device_key);
	memset(device_id, 0, sizeof(device_id));
	get_deviceId_byMac("000EA9362200", device_id);
	printf("get device_id %s for mac 000EA9362200\n", device_id);
	return 0;
}

void test_fingprint(void)
{
	char fingprint[41] = {0};
	p2p_generate_fingprint("somedata somedata", "somekeysomekey", fingprint, 41);
	printf("fingprint:%s\n", fingprint);
	return;
}

int test_encrypt(char *key, char *data, char *enc_data, int len)
{
	int nr_of_bytes;
	unsigned char *out;
	out = (unsigned char *)malloc(strlen(data)/16*16+16);
	nr_of_bytes = p2p_message_encrypt(key, 32, data, strlen(data), out);
	p2p_base64_encode(out, nr_of_bytes, enc_data, len);
	printf("===key:%s\n===src:%s\n===enc:%s\n", key, data, enc_data);
	return 0;
}

int test_decrypt(char *key, char *enc_data)
{
	int total;
	char message[1024] = {0};
	char out_message[1024] = {0};
	total = p2p_base64decode(message, enc_data, 1024);
	p2p_message_decrypt((unsigned char *)key, 32, (unsigned char *)message, total, (unsigned char *)out_message);
	printf("===key:%s\n===de_src:%s\n", key, out_message);
	return 0;
}

int ttest(void)
{
	char message[1024] = {0};
	unsigned char *key="ZGQyNTFiYzMtZjU0NC00NTY3LWIwYTUt";
	unsigned char *data="service: SIP\r\ned: 192.168.130.157:5060\r\noid: default\r\nversion: 1.1.3";
	test_encrypt(key, data, message, sizeof(message));
	test_decrypt(key, message);
	test_fingprint();
	test_get_deviceId();
	return 0;
}

int phrase_argv(int argc, char *argv[])
{
	int rc = 0;

	while ((rc = getopt(argc, argv, "K:k:L:S:N:l:c:p:s:i:I:dtTF")) != -1) {
		switch(rc) {
		case 'F':
			free_endpoint = 1;
			break;
		case 'd':
			if(debug_output < 1)
				debug_output = 1;
			break;
		case 'l':
			log_level = atoi(optarg);
			break;
		case 'T':
			ttest();
			exit(0);
			break;
		case 'S':
			strncpy(client_service, optarg, sizeof(client_service) - 1);
			break;
		case 'L':
			client_port = atoi(optarg);
			break;
		case 'N':
			strncpy(demo_log_name, optarg, sizeof(demo_log_name) - 1);
			break;
		case 's':
			strncpy(server_address, optarg, sizeof(server_address) - 1);
			break;
		case 'K':
			keep_reg_interval = atoi(optarg);
			break;
		case 'p':
			server_port = atoi(optarg);
			break;
		case 't':
			debug_output = 2;
			break;
		case 'i':
			random_cid = 0;
			strncpy(client_id, optarg, sizeof(client_id) - 1);
			break;
		case 'I':
			strncpy(client_ic, optarg, sizeof(client_ic) - 1);
			break;
		case 'k':
			strncpy(client_key, optarg, sizeof(client_key) - 1);
			break;
		case 'c':
			strncpy(g_tid, optarg, sizeof(g_tid) - 1);
			break;
		default:
			fprintf(stderr,"%s\n", Usage);
			exit(1);
		}
	}
	return 0;
}

int check_endpoint_event(REND_EPT_HANDLE endpoint, int event)
{
	int status;
	char cid[41], ped[25];
	REND_CONN_HANDLE conn = NULL;
	if(get_rendezvous_endpoint(endpoint, &status, cid, NULL, ped) == 0){
		printf("%s(%s) got event %x\n", cid, ped, event);
		if(free_endpoint == 1){
			printf("free endpoint %s(%s)\n", cid, ped);
			free_rendezvous_endpoint(endpoint);
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
			conn = new_rendezvous_connection(endpoint, tid, client_service, "default", client_ic);
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
	int log_type = P2P_LOG_TYPE_FILE;
	char ed[25];
	char udp_message[1024] = {0};
	struct sockaddr_in peer_addr;
	int peer_fd;
	REND_EPT_HANDLE p2p_endpoint;
	REND_CONN_HANDLE p2p_conn = NULL;
	signal(SIGSEGV, sig_handler);
	phrase_argv(argc, argv);
	if(log_level > 0)
		printf("demo server version %s\n", VERSION);
	if(debug_output >= 1)
		log_type = P2P_LOG_TYPE_SCREEN;
	if(strlen(demo_log_name) > 0)
		rc = p2p_init(NULL, demo_log_name, log_type, log_level, server_address, server_port);
	else
		rc = p2p_init(NULL, "p2pdemo", log_type, log_level, server_address, server_port);
	if(rc != 0){
		fprintf(stderr, "init p2p failed\n");
		return -1;
	}
	if(keep_reg_interval > 0)
		set_p2p_reg_keep_interval(keep_reg_interval);
	if(debug_output >= 2)
		set_p2p_option(0, 1);

	peer_fd = new_udp_socket(client_port, NULL);

	//初始化约会客户端
	if(random_cid == 1){
		p2p_endpoint = new_rendezvous_endpoint(NULL, client_service, NULL, client_ic, client_key, peer_fd);
	}else{
		p2p_endpoint = new_rendezvous_endpoint(client_id, client_service, NULL, client_ic, client_key, peer_fd);
	}
	if(p2p_endpoint == NULL) {
		close(peer_fd);
		printf("init rendezvous endpoint fail\n");
		return 0;
	}
	rendezvous_endpoint_reg(p2p_endpoint);
	rendezvous_endpoint_eventCallbacks(p2p_endpoint, ENDPOINT_EVENT_NAT_CHANGE|ENDPOINT_EVENT_CONN_FAIL|ENDPOINT_EVENT_CONN_OK, check_endpoint_event);
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

		//解析并处理数据包
		rc = handle_rendezvous_packet(peer_fd, udp_message, &peer_addr);
		if(rc == 0){
			// 处理成功，继续接收处理下个包
			continue;
		}

		//被连接端，响应连接端发送的应用数据，用于测试
		if(p2p_conn == NULL){
			address_to_endpoint(&peer_addr, ed);
			memcpy(udp_message, "reply ", 6);
			memcpy(udp_message+6, ed, strlen(ed));
			sendto(peer_fd, udp_message, strlen(udp_message), 0, (struct sockaddr *)&peer_addr, sizeof(struct sockaddr_in));
		}else{
			address_to_endpoint(&peer_addr, ed);
			printf("recv from %s == %s\n", ed, udp_message);
		}
	}
	free_rendezvous_endpoint(p2p_endpoint);
	close(peer_fd);
	return 0;
}
