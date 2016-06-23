#include <stdio.h>
#include <stdlib.h>

#ifndef P2P_API
#define P2P_API

#define P2P_LIB_VERSION "1.1.4"

typedef void * REND_EPT_HANDLE;
typedef void * REND_CONN_HANDLE;

typedef enum {
    P2P_LOG_TYPE_NONE,
    P2P_LOG_TYPE_FILE,
    P2P_LOG_TYPE_SCREEN,
    P2P_LOG_TYPE_MAX,
}P2P_LOG_TYPE;

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

int p2p_init(char *log_path, char *app_name, P2P_LOG_TYPE log_type, int log_level, char *server, int server_port);

REND_EPT_HANDLE new_rendezvous_endpoint(char *cid, char *service, char *oid, char *ic, char *key, int udp_fd);
int rendezvous_endpoint_reg(REND_EPT_HANDLE endpoint);
void free_rendezvous_endpoint(REND_EPT_HANDLE endpoint);
int get_rendezvous_endpoint(REND_EPT_HANDLE endpoint, int *status, char *cid, char *ed, char *ped);
int get_rendezvous_endpoint_error(REND_EPT_HANDLE endpoint, int *status, int *error_code);

REND_CONN_HANDLE new_rendezvous_connection(REND_EPT_HANDLE my_endpoint, char *tid, char *service, char *toid, char *tic);
void free_rendezvous_connection(REND_CONN_HANDLE conn);
int get_rendezvous_connection(REND_CONN_HANDLE conn, int *status, char *r_ed, char *r_ped);
int get_rendezvous_connection_error(REND_CONN_HANDLE conn, int *status, int *error_code);

int handle_rendezvous_packet(int udp_fd, char *udp_message, struct sockaddr_in *peer_addr);
int rendezvous_status_handle(void); //周期调用
int set_p2p_reg_keep_interval(int interval);
void set_p2p_option(int crypt, int debug);

int get_deviceId_key(char *id, char *key);

#endif
