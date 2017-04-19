#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <netinet/in.h> //for sockaddr_in
#include <sys/types.h>  //for socket
#include <sys/socket.h> //for socket
#include <arpa/inet.h>
#include <time.h>
#include <errno.h>
#include "p2p_api.h"

typedef struct __node {
    int fd;
    int connStatus;
    int regStatus;
    int sendCnt;
    REND_EPT_HANDLE ept;
    REND_CONN_HANDLE conn;
    char cid[41];
    char ped[25];
    char r_ed[25];
    char r_ped[25];
}EPT_NODE;

typedef struct {
	char *key;
	char *value;
}KEY_VAL;

extern int debug_output;
extern int app_type;
extern int free_endpoint;
extern int report_test;
extern char report_message[1024];
extern int log_level;
extern char demo_log_name[128];
extern int server_port;
extern char server_address[24];
extern char client_id[41];
extern char client_key[33];
extern char client_ic[65];
extern char client_oid[32];
extern char client_toid[32];
extern char client_service[20];
extern int client_port;
extern int keep_reg_interval;
extern char g_tid[41];

// 常用函数
void print_trace( int signo );
int decodeMAC(char *content, int content_len);
int encodeMAC(char *content, int content_len);
void p2p_demo_set_dns_servers(char *dns1, char *dns2, char *dns3);
int p2p_demo_dns_lookup(char *name, char ip_str[3][25]);
int p2p_demo_dns_resove(char *domain_name, char host_address[3][25]);
int new_udp_socket(int local_port, char *local_address);
int recv_udp_packet(int udp_fd, char *udp_message, int message_len, struct sockaddr_in *peer_addr, int timeout);
int scan_file(char *file_name, char sep, KEY_VAL *kv, int num);
void freeKV(KEY_VAL *kv, int num, int dump);
void testSend();
int ttest(void);

// libp2pnat中的未公开函数
char *address_to_endpoint(struct sockaddr_in *addr, char *ed);
int endpoint_to_address(char *ed, struct sockaddr_in *addr);
void p2p_base64_encode(const char *in,size_t inlen,char *out,size_t outlen);
int p2p_base64decode(void *dst, char *src, int maxlen);
int p2p_message_decrypt(unsigned char *key, int key_len, unsigned char *enc_message, int message_len, unsigned char *out_message);
int p2p_generate_fingprint(char *data, char *key, unsigned char *fingprint, int fingprint_len);
int p2p_message_encrypt(unsigned char *key, int key_len, unsigned char *message, int message_len, unsigned char *enc_message);
void dump_p2p_lib_global(int debug);

// demo测试
int select_endpoint_to_be_conn(EPT_NODE *list, int num, int i);
int send_to_server_request_conn(int udp_fd, char *tid, char *server_ped);
void sendRequestToMccs(int udp_fd, char *s, char *mac, int reqId);

// P2P行为测试
void report_test_result(int peer_fd, int rport, char *result);
int p2p_behavior_test();
int p2p_behavior_test_server(char *p2pServer);
int p2p_demo_test_reg(int client_fd, int timeout, char *ed, char *ped);
int test_send_mini_packet(int client_fd, char *ip, int port);
int p2p_demo_send_packet(int udp_fd, char *server_ip, int server_port, char *ped);
