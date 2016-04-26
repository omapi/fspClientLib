#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef COMMON_UTIL_H
#define COMMON_UTIL_H

#define DEFAULT_P2P_SERVER "p2p.newrocktech.com"
extern char *g_server_ip;
extern int g_server_port;
extern int p2p_lib_log_level;
extern int keep_register_interval;
extern int keep_connection_interval;
int get_key_value(char *buf, char sep, char **key, char **value);
int get_line(char **line, int buf_size, int *n, char *message);
char *address_to_endpoint(struct sockaddr_in *addr, char *ed);
int endpoint_to_address(char *ed, struct sockaddr_in *addr);
int addr_get_from_sock(int udp_fd, struct sockaddr_in *addr);
int detect_local_address(char *server_ip, int server_port, struct sockaddr_in *addr);
int dns_resove(char *domain_name, char *host_address);
int get_deviceId_key(char *id, char *key);
void init_p2p_log(char *log_path_dir, char *application);
void set_p2p_log_level(int new_level);
void set_p2p_option(int crypt, int debug);
int p2p_init(char *log_path, char *app_name, int log_level, char *server, int server_port);
int set_p2p_reg_keep_interval(int interval);

#endif
