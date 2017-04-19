#include "global.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h> //for sockaddr_in
#include <sys/types.h>  //for socket
#include <sys/socket.h> //for socket
#include <string.h>
#include <netdb.h>
#include <resolv.h>
#include <sys/fcntl.h>
#ifdef X86
#include <execinfo.h>
#endif
#include "errno.h"
// #include <sys/un.h>

void print_trace( int signo )
{
#ifdef X86
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
#endif
	return;
}

int decodeMAC(char *content, int content_len)
{
	int i;
	if(content == NULL || content_len <= 13)
		return -1;
	for(i = 1; i <= 12; i++){
		if((i+12) >= content_len)
			break;
		*((unsigned char *)content+i) -= (5 + (*((unsigned char *)content+i+12))%20);
	}
	return 0;
}

int encodeMAC(char *content, int content_len)
{
	int i;
	if(content == NULL || content_len < 14)
		return -1;
	for(i = 1; i <= 12; i++){
		if((i+12) >= content_len)
			break;
		*((unsigned char *)content+i) += (5 + (*((unsigned char *)content+i+12))%20);
	}
	return 0;
}

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

int recv_udp_packet(int udp_fd, char *udp_message, int message_len, struct sockaddr_in *peer_addr, int timeout)
{
	int rc;
	fd_set rset;
	struct timeval tval;
	socklen_t addr_len = sizeof(struct sockaddr_in);
	tval.tv_sec = timeout;
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

int p2p_demo_dns_resove(char *domain_name, char host_address[3][25])
{
	int ret;
	struct addrinfo hints;
	struct sockaddr_in *s_addr;
	struct addrinfo *res, *cur;

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET; /* Allow IPv4 */
	hints.ai_flags = AI_PASSIVE; /* For wildcard IP address */
	hints.ai_protocol = 0; /* Any protocol */
	hints.ai_socktype = SOCK_STREAM;
	ret = getaddrinfo(domain_name, NULL, &hints, &res);
	if(ret == 0)
	{
		int i = 0;
		for(cur = res; cur != NULL; cur = cur->ai_next)
		{
			if(i >= 3)
				break;
			s_addr = (struct sockaddr_in *)cur->ai_addr;
			inet_ntop(AF_INET, &s_addr->sin_addr, host_address[i], 16);
			if(log_level > 1)
				printf("IP address [%s] for hostname - <%s>\n", host_address[i], domain_name);
			i++;
		}
		freeaddrinfo(res);
	}
	else
	{
		res_init();
		ret = p2p_demo_dns_lookup(domain_name, host_address);
		if(ret > 0)
		{
			if(log_level > 1){
				int i;
				for(i = 0; i < ret; i++)
					printf("internal resolv IP address [%s] for hostname - <%s>\n", host_address[i], domain_name);
			}
			ret = 0;
		}
		else
		{
			if(log_level > 1)
				printf("check %s dns address!!! getaddrinfo(%d) - %s\n", domain_name, ret, gai_strerror(ret));
			return -1;
		}
	}
	return ret;
}

void readFile(char *buf)
{
	FILE *fp;
	fp = fopen("pdm.send", "r");
	if(fp != NULL){
		fread(buf, 1, 1000, fp);
		fclose(fp);
	}
	return;
}

void testSend()
{
	int peer_fd, rc;
	char udp_message[1024] = {0};
	char r_ped[25];
	struct sockaddr_in peer_addr;
	peer_fd = new_udp_socket(client_port, NULL);
	// strcpy(udp_message, "hello");
	readFile(udp_message);
	sprintf(r_ped, "%s:%d", server_address, server_port);
	endpoint_to_address(r_ped, &peer_addr);
	sendto(peer_fd, udp_message, strlen(udp_message), 0, (struct sockaddr *)&peer_addr, sizeof(struct sockaddr_in));
	memset(udp_message, 0, sizeof(udp_message));
	rc = recv_udp_packet(peer_fd, udp_message, sizeof(udp_message), &peer_addr, 1);
	if(rc > 0){
		printf("recv=============\n%s\n", udp_message);
	}
	close(peer_fd);
	return;
}

void sendRequestToMccs(int udp_fd, char *s, char *mac, int reqId)
{
	int rc, doRecv = 0;
	struct sockaddr_in peer_addr;
	char udp_message[1024] = {0};
	if(udp_fd == 0) {
		udp_fd = new_udp_socket(client_port, NULL);
		doRecv = 1;
	}
	snprintf(udp_message, 1024, "1%s{\"request\":%d,\"body\":\"p2pdemo test\"}", mac, reqId);
	printf("send %s to %s(%s)\n", udp_message, s, mac);
	udp_message[0] = 1;
	encodeMAC(udp_message, strlen(udp_message+1)+1);
	endpoint_to_address(s, &peer_addr);
	rc = sendto(udp_fd, udp_message, strlen(udp_message), 0, (struct sockaddr *)&peer_addr, sizeof(struct sockaddr_in));
	if(doRecv == 1) {
		rc = recv_udp_packet(udp_fd, udp_message, sizeof(udp_message), &peer_addr, 1);
		if(rc > 0) {
			printf("recv===%s\n", udp_message);
		}
		close(udp_fd);
	}
	return;
}

/* ----- scan function ---- */
static char *trim_space(char *str)
{
	int i;
	if(str == NULL || strlen(str) == 0)
		return str;
	// trim last space
	for(i = strlen(str) - 1; i >= 0 && isspace(str[i]); i--);
	str[i+1] = '\0';
	// skip first space
	if(i > 0)
		for(;isblank(*str);str++);
	return str;
}

int demo_get_key_value(char *buf, char sep, char **key, char **value)
{
	char *v;
	v = strchr(buf, sep);
	if(v == NULL)
	{
		*key = NULL;
		*value = NULL;
		return -1;
	}
	*v++ = '\0';
	*key = trim_space(buf);
	*value = trim_space(v);
	return 0;
}

int scan_check_file(char *file_name, char sep, KEY_VAL *kv, int num)
{
	int rc, i;
	int done = 0;
	FILE *fp = NULL;
	char buf[1024];
	fp = fopen(file_name, "r");
	if(fp == NULL) {
		return -1;
	}
	while(done == 0 && fgets(buf, sizeof(buf), fp) != NULL)
	{
		char *key, *value;
		rc = demo_get_key_value(buf, sep, &key, &value);
		if(rc == 0)
		{
			// printf("key:\"%s\", val:\"%s\"\n", key, value);
			done = 1; //assume all key value set done
			if(kv == NULL)
			continue;
			for(i = 0; i < num; i++)
			{
				if(kv[i].value != NULL || kv[i].key == NULL)
					continue;
				done = 0; //some key's value not set, should continue scan
				if(strcmp(key, kv[i].key) == 0 && strlen(key) == strlen(kv[i].key))
				{
					kv[i].value = strdup(value);
					break;
				}
			}
		}
	}
	fclose(fp);
	return 0;
}

int scan_file(char *file_name, char sep, KEY_VAL *kv, int num)
{
	int rc, i;
	FILE *fp = NULL;
	char buf[1024];
	fp = fopen(file_name, "r");
	if(fp == NULL)
		return -1;
	while(fgets(buf, sizeof(buf), fp) != NULL)
	{
		char *key, *value;
		rc = demo_get_key_value(trim_space(buf), sep, &key, &value);
		if(rc == 0)
		{
			for(i = 0; i < num; i++){
				if(kv[i].key != NULL)
					continue;
				kv[i].key = strdup(key);
				kv[i].value = strdup(value);
				break;
			}
		}
	}
	fclose(fp);
	return 0;
}

void freeKV(KEY_VAL *kv, int num, int dump)
{
	int i;
	for(i = 0; i < num; i++){
		if (dump == 1 && kv[i].key != NULL)
			printf("will free %s:%s\n", kv[i].key, kv[i].value);
		if(kv[i].key != NULL){
			free(kv[i].key);
			kv[i].key = NULL;
		}
		if(kv[i].value != NULL){
			free(kv[i].value);
			kv[i].value = NULL;
		}
	}
	return;
}

/* ---- Test function ------- */
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
	unsigned char fingprint[41] = {0};
	p2p_generate_fingprint("somedata somedata", "somekeysomekey", fingprint, 41);
	printf("fingprint:%s\n", fingprint);
	return;
}

int test_encrypt(unsigned char *key, unsigned char *data, char *enc_data, int len)
{
	int nr_of_bytes;
	size_t data_len = strlen((char *)data);
	unsigned char *out;
	out = (unsigned char *)malloc(data_len/16*16+16);
	nr_of_bytes = p2p_message_encrypt(key, 32, data, data_len, out);
	p2p_base64_encode((char *)out, nr_of_bytes, enc_data, len);
	printf("===key:%s\n===src:%s\n===enc:%s\n", key, data, enc_data);
	return 0;
}

int test_decrypt(unsigned char *key, char *enc_data)
{
	int total;
	char message[1024] = {0};
	char out_message[1024] = {0};
	total = p2p_base64decode(message, enc_data, 1024);
	p2p_message_decrypt(key, 32, (unsigned char *)message, total, (unsigned char *)out_message);
	printf("===key:%s\n===de_src:%s\n", key, out_message);
	return 0;
}

int ttest(void)
{
	char message[1024] = {0};
	unsigned char *key=(unsigned char *)"ZGQyNTFiYzMtZjU0NC00NTY3LWIwYTUt";
	unsigned char *data=(unsigned char *)"service: SIP\r\ned: 192.168.130.157:5060\r\noid: default\r\nversion: 1.1.3";
	test_encrypt(key, data, message, sizeof(message));
	test_decrypt(key, message);
	test_fingprint();
	test_get_deviceId();
	return 0;
}
