#include <stdio.h>
#include <time.h>
#include "list.h"

#ifndef RENDEZVOUS_ENDPOINT_H
#define RENDEZVOUS_ENDPOINT_H

typedef enum {
	ENDPOINT_INITIAL,
	ENDPOINT_REGISTERING,
	ENDPOINT_REGISTER_FAIL,
	ENDPOINT_REGISTER_OK,
}endpoint_status;

typedef enum {
	CONNECTION_INIT,
	CONNECTION_REQUEST,
	CONNECTION_PING,
	CONNECTION_FAILED,
	CONNECTION_OK,
}rendezvous_connection_status;

typedef struct {
	struct dl_list list;
	int udp_fd;
	time_t last_register;
	time_t last_request;
	char server_ip[25];
	int server_port;
	endpoint_status status;
	int status_code;
	char cid[41];
	char oid[41];
	char ed[25];
	char ped[25];
	char service[65];
	char ic[65];
	char key[33];
}rendezvous_endpoint_t;

typedef struct {
	struct dl_list list;
	rendezvous_endpoint_t *my_endpoint;
	rendezvous_connection_status status;
	int status_code;
	int retry_count;
	int conn_info;
	time_t last_request;
	char connid[9];
	char tid[41];
	char toid[41];
	char r_ed[25];
	char r_ped[25];
	char service[65];
	char ic[65];
}rendezvous_connection_t;

extern char *g_server_ip;
extern int g_server_port;
extern int p2p_lib_init;
extern int p2p_lib_log_level;
extern int p2p_lib_crypt_message;
extern int p2p_lib_debug_message;
typedef rendezvous_endpoint_t * REND_EPT_HANDLE;
typedef rendezvous_connection_t * REND_CONN_HANDLE;
#define ED_BIT 1
#define PED_BIT 2

REND_EPT_HANDLE new_rendezvous_endpoint(char *cid, char *service, char *oid, char *ic, char *key, int udp_fd);
int rendezvous_endpoint_reg(REND_EPT_HANDLE endpoint);
void free_rendezvous_endpoint(REND_EPT_HANDLE endpoint);
int get_rendezvous_endpoint(REND_EPT_HANDLE endpoint, int *status, char *cid, char *ed, char *ped);
int get_rendezvous_endpoint_error(REND_EPT_HANDLE endpoint, int *status, int *error_code);

REND_CONN_HANDLE new_rendezvous_connection(REND_EPT_HANDLE my_endpoint, char *tid, char *service, char *toid, char *tic);
void free_rendezvous_connection(REND_CONN_HANDLE conn);
int get_rendezvous_connection(REND_CONN_HANDLE conn, int *status, char *r_ed, char *r_ped);
int get_rendezvous_connection_error(REND_CONN_HANDLE conn, int *status, int *error_code);

//==== for debug==
rendezvous_endpoint_t *find_endpoint_by_cid(char *cid);
void show_connections(void);
void show_endpoints(void);

#endif
