#include <stdio.h>
#include <netinet/in.h> //for sockaddr_in
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "rendezvous_endpoint.h"
#include "list.h"

#ifndef RENDEZVOUS_PACKET_H
#define RENDEZVOUS_PACKET_H
typedef enum {
	P2P_NONE_CLASS,
	P2P_REQUEST,
	P2P_RESPONSE,
}rendezvous_class;

typedef enum {
	P2P_NONE_TYPE,
	P2P_REGISTER,
	P2P_CONNECT,
	P2P_FORWARD,
	P2P_PING,
	P2P_PONG,
}rendezvous_type;

typedef enum {
	T_ENDPOINT,
	T_CONNECTION,
}context_type;

typedef struct
{
	rendezvous_class p_class; /* P2PNAT00, P2PNAT01 */
	rendezvous_type p_type; /* REG, CON, FWD, PING, PONG */
	char mid[9]; /* length 8 */
	char connid[9]; /* length 8 */
	char cid[41];
	char tid[41];
	char ed[25];
	char ped[25];
	char oid[25];
	char toid[25];
	char service[65];
	char ic[65];
	char fp[41];

	char key[33];
	char code[4];
	char version[10];
	char emsg[256];
	unsigned char secure;
	struct sockaddr_in peer_addr;
}rendezvous_packet_t;

typedef struct
{
	struct dl_list list;
	char mid[9];
	context_type type;
	time_t last_request;
	void *context;
}message_request_t;

void generate_rendezvous_packet_mid(rendezvous_packet_t *packet, context_type type, void *context);
void *check_response_packet(rendezvous_packet_t *packet);
void clean_timeout_message(int timeout);
int base64decode(void *dst, char *src, int maxlen);
void base64encode(unsigned char *from, char *to, int len);
void base64_encode(const char *in,size_t inlen,char *out,size_t outlen);
int message_encrypt(unsigned char *key, int key_len, unsigned char *message, int message_len, unsigned char *enc_message);
int message_decrypt(unsigned char *key, int key_len, unsigned char *enc_message, int message_len, unsigned char *out_message);
int generate_fingprint(char *data, char *key, unsigned char *fingprint, int fingprint_len);
int decode_rendezvous_packet(char *udp_message, struct sockaddr_in *peer_addr, rendezvous_packet_t *packet);
int encode_rendezvous_packet(rendezvous_endpoint_t *endpoint, rendezvous_packet_t *packet, char *udp_message);
int init_rendezvous_packet(rendezvous_packet_t *packet, rendezvous_class p_class, rendezvous_type p_type);
int send_rendezvous_packet_to(rendezvous_endpoint_t *endpoint, rendezvous_packet_t *packet, char *server_ip, int server_port);
int send_keep_packet(int udp_fd, char *server_ip, int server_port);
int send_rendezvous_packet(int udp_fd, rendezvous_packet_t *packet);

int rendezvous_message_handle(int udp_fd, rendezvous_packet_t *packet);
int rendezvous_status_handle(void); //周期调用

#endif
