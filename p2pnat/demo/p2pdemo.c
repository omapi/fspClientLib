#include "global.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <signal.h>
#include <errno.h>
#include <netinet/in.h> //for sockaddr_in
#include <sys/types.h> //for socket
#include <sys/socket.h> //for socket
#include <pthread.h>
#include "p2p_api.h"

#define VERSION "1.1.1"

// #define NUM_ENDPOINTS 10
int NUM_ENDPOINTS = 1;
int app_type = 0;
int report_test = 0;
char report_message[1024] = { 0 };
int debug_output = 0;
int log_level = 0;
int free_endpoint = 0;
/* libp2p全局配置 */
char demo_log_name[128] = "p2pdemo";
char cid_key_file[128] = "";
char server_address[24] = "";
int server_port = 0;
int keep_reg_interval = 0;
/* P2P客户端及连接参数控制 */
int client_port = 0;
char client_id[41] = { 0 };
char client_key[33] = { 0 };
char client_service[20] = "DMO";
char client_ic[65] = { 0 };
char client_oid[32] = "default";
char g_tid[41] = { 0 };
char client_toid[32] = "default";
/* demo维护的测试客户端列表 */
EPT_NODE* ept_list;
int cok, cfail;

static char Usage[] = "p2p rendezvous\n"
                      "Usage: p2pnat [options] \n"
                      "Options:\n"
                      "        -s server   P2PNAT server address to use\n"
                      "        -p port     P2PNAT server port\n"
                      "        -c tid      cid to connect\n"
                      "        -O toid     toid to connect\n"
                      "        -I ic       invite code\n"
                      "        -i cid      client id\n"
                      "        -o oid      client oid\n"
                      "        -k key      client key\n"
                      "        -S service  client service\n"
                      "        -L port     client bind port\n"
                      "        -d          output log info to screen\n"
                      "        -F          free endpoint while p2p connect ok\n"
                      "        -R          report p2p test result\n"
                      "        -t          test mode, donn't crypt message\n"
                      "        -l level    log level\n"
                      "        -T          test get device_id\n"
                      "        -K 20       keep nat interval\n"
                      "        -C 10       p2p endpoint nums\n"
                      "        -A 0        test app type\n"
                      "        -B file     load cid and key from file\n"
                      "        -N name     log app name\n";

void sig_handler(int signo)
{
    print_trace(signo);
    exit(1);
}

int phrase_argv(int argc, char* argv[])
{
    int rc = 0;

    while ((rc = getopt(argc, argv, "A:B:C:K:k:L:S:N:l:c:p:s:i:I:o:O:dtTFR")) != -1) {
        switch (rc) {
        case 'F':
            free_endpoint = 1;
            break;
        case 'd':
            if (debug_output < 1)
                debug_output = 1;
            break;
        case 'l':
            log_level = atoi(optarg);
            break;
        case 'R':
            report_test = 1;
            break;
        case 'T': //测试函数后退出
            if (app_type == 255) {
                testSend();
            } else if (app_type == 1) {
                // ./build.sh c sdfsf -s 192.168.130.80:9911 -S 000EA93D0525 -A 1 -K 2 -T
                sendRequestToMccs(0, server_address, client_service, keep_reg_interval);
            } else
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
        case 'C':
            NUM_ENDPOINTS = atoi(optarg);
            break;
        case 'A':
            app_type = atoi(optarg);
            break;
        case 'B':
            strncpy(cid_key_file, optarg, sizeof(cid_key_file) - 1);
            break;
        case 'p':
            server_port = atoi(optarg);
            break;
        case 't':
            debug_output = 2;
            break;
        case 'i':
            strncpy(client_id, optarg, sizeof(client_id) - 1);
            break;
        case 'o':
            strncpy(client_oid, optarg, sizeof(client_oid) - 1);
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
        case 'O':
            strncpy(client_toid, optarg, sizeof(client_toid) - 1);
            break;
        default:
            fprintf(stderr, "%s\n", Usage);
            exit(1);
        }
    }
    return 0;
}

/* ---------------------- 下面是新的多客户端测试 -------------------------- */
static int check_endpoint_status(EPT_NODE* list, int num);

// 向指定服务器发送请求(它连接指定cid)或测试数据
int send_to_server_request_conn(int udp_fd, char* tid, char* server_ped)
{
    int rc = -1;
    char udp_message[1024] = { 0 };
    struct sockaddr_in remote_addr;
    if (server_ped == NULL || strlen(server_ped) == 0)
        return rc;
    if (tid != NULL && strlen(tid) > 0)
        snprintf(udp_message, 1024, "conn %s", tid);
    else
        snprintf(udp_message, 1024, "this is application test request, this is application test request, this is application test request");
    rc = endpoint_to_address(server_ped, &remote_addr);
    if (rc == 0)
        rc = sendto(udp_fd, udp_message, strlen(udp_message), 0, (struct sockaddr*)&remote_addr, sizeof(struct sockaddr_in));
    else
        rc = -1;
    return rc;
}

// 线程　周期处理客户端状态, 100ms period
static void* status_period(void* param)
{
    char udp_message[64] = { 0 };
    struct sockaddr_in peer_addr;
    int peer_fd = new_udp_socket(0, NULL);
    while (1) {
        recv_udp_packet(peer_fd, udp_message, sizeof(udp_message), &peer_addr, 0);
        rendezvous_status_handle();
    }
    return 0;
}

// 线程　周期检查客户端状态，根据状态推动测试流程
static void* check_endpoint_period(void* param)
{
    char udp_message[64] = { 0 };
    struct sockaddr_in peer_addr;
    int peer_fd = new_udp_socket(0, NULL);
    while (1) {
        recv_udp_packet(peer_fd, udp_message, sizeof(udp_message), &peer_addr, 0);
        check_endpoint_status(ept_list, NUM_ENDPOINTS);
    }
    return 0;
}

// 检查所有节点，看是否有数据接收
static int check_read(EPT_NODE* list, int num, fd_set* rset, int timeout)
{
    int rc, i, max_fd;
    struct timeval tval;
    tval.tv_sec = timeout;
    tval.tv_usec = 100 * 1000;
    max_fd = 0;
    FD_ZERO(rset);
    for (i = 0; i < num; i++) {
        FD_SET(list[i].fd, rset);
        if (max_fd < list[i].fd)
            max_fd = list[i].fd;
    }
    rc = select(max_fd + 1, rset, NULL, NULL, &tval);
    return rc;
}

// 处理客户端连接事件，更新节点的连接状态
static int demo_handle_endpoint_event(REND_EPT_HANDLE endpoint, int event)
{
    int i, status;
    if (((event & ENDPOINT_EVENT_CONN_FAIL) == 0) && ((event & ENDPOINT_EVENT_CONN_OK) == 0)) {
        // 不是连接成功或连接失败事件
        return 0;
    }
    for (i = 0; i < NUM_ENDPOINTS; i++) {
        if (endpoint != ept_list[i].ept)
            continue;
        if ((event & ENDPOINT_EVENT_CONN_FAIL) != 0) {
            cfail++;
            ept_list[i].connStatus = CONNECTION_FAILED;
        } else if ((event & ENDPOINT_EVENT_CONN_OK) != 0) {
            cok++;
            ept_list[i].connStatus = CONNECTION_OK;
            ept_list[i].sendCnt = 0;
            // 连接成功后，获取对方的地址
            if (ept_list[i].conn != NULL) {
                get_rendezvous_connection(ept_list[i].conn, &status, ept_list[i].r_ed, ept_list[i].r_ped);
                if (status == CONNECTION_POK) {
                    char newPort[15] = { 0 };
                    get_rendezvous_connection_attr(ept_list[i].conn, "newPort", newPort);
                    printf("connect ok with newPort %s\n", newPort);
                }
            }
        }
    }
    return 0;
}

static void show_ept_status(EPT_NODE* node, time_t cur_check, int* do_show)
{
    static int cnt = 0;
    static time_t last_check;

    // 多客户端时每5秒显示一次客户端状态，日志等级为0时只显示2次
    if (cur_check != 0 && cur_check - last_check >= 5 && NUM_ENDPOINTS > 1 && (cnt < 2 || log_level > 0)) {
        last_check = cur_check;
        *do_show = 1;
        cnt++;
    } else if (*do_show == 0) {
        return;
    }
    if (node == NULL) {
        dump_p2p_lib_global(1);
        fprintf(stderr, "========================conn ok %d, conn fail %d========================================== \n", cok, cfail);
    } else {
        fprintf(stderr, "%p fd[%03d] cid %s ped %s status %d conn_status (%p)%d, r_ed %s, r_ped %s\n", node->ept, node->fd, node->cid, node->ped, node->regStatus, node->conn, node->connStatus, node->r_ed, node->r_ped);
    }
    return;
}

static int check_endpoint_status(EPT_NODE* list, int num)
{
    time_t cur_check = time(NULL);
    int i, status, do_show = 0;
    REND_EPT_HANDLE endpoint;
    EPT_NODE* cur_node = NULL;
    int cliMode = 0;

    show_ept_status(cur_node, cur_check, &do_show);
    if (strlen(g_tid) > 0)
        cliMode = 1;
    // 检查客户端注册状态
    for (i = 0; i < num; i++) {
        cur_node = &list[i];
        endpoint = cur_node->ept;
        show_ept_status(cur_node, 0, &do_show);
        if (cur_node->regStatus != ENDPOINT_REGISTER_OK) {
            if (get_rendezvous_endpoint(endpoint, &status, cur_node->cid, NULL, cur_node->ped) == 0) {
                cur_node->regStatus = status;
            } else {
                cur_node->ept = new_rendezvous_endpoint(NULL, client_service, client_oid, client_ic, NULL, cur_node->fd);
                if (cur_node->ept != NULL) {
                    rendezvous_endpoint_reg(cur_node->ept);
                    rendezvous_endpoint_eventCallbacks(cur_node->ept, ENDPOINT_EVENT_NAT_CHANGE | ENDPOINT_EVENT_CONN_FAIL | ENDPOINT_EVENT_CONN_OK, demo_handle_endpoint_event);
                }
            }
            continue;
        }
        if (cur_node->regStatus == ENDPOINT_REGISTER_OK) {
            if (cliMode == 1 && i == 0 && (cur_node->conn == NULL || cur_node->connStatus == CONNECTION_FAILED)) {
                // demo主动使用第一个客户端去和服务器建立控制通道
                if (cur_node->conn != NULL) {
                    free_rendezvous_connection(cur_node->conn);
                    cur_node->conn = NULL;
                }
                cur_node->r_ed[0] = '\0';
                cur_node->r_ped[0] = '\0';
                cur_node->connStatus = CONNECTION_REQUEST;
                cur_node->conn = new_rendezvous_connection(endpoint, g_tid, client_service, client_toid, client_ic);
                continue;
            } else if (cliMode == 1 && i > 0 && list[0].connStatus == CONNECTION_OK) {
                // 控制通道成功连接，可以发送指令让对方连接我
                if (cur_node->connStatus != CONNECTION_REQUEST || free_endpoint == 1)
                    select_endpoint_to_be_conn(list, num, i);
            }
            if (cur_node->conn != NULL && cur_node->connStatus == CONNECTION_OK) {
                // 连接成功，发送测试数据
                if (cur_node->sendCnt < 1) {
                    send_to_server_request_conn(cur_node->fd, NULL, cur_node->r_ed);
                    send_to_server_request_conn(cur_node->fd, NULL, cur_node->r_ped);
                    cur_node->sendCnt++;
                } else if (cliMode == 0) {
                    free_rendezvous_connection(cur_node->conn);
                    cur_node->conn = NULL;
                    cur_node->connStatus = CONNECTION_INIT;
                    if (free_endpoint == 1) {
                        free_rendezvous_endpoint(endpoint);
                        cur_node->regStatus = ENDPOINT_INITIAL;
                    }
                }
            }
        }
    }
    show_ept_status(NULL, 0, &do_show);
    return 0;
}

// 向另一demo发送指令，让它连接我
int select_endpoint_to_be_conn(EPT_NODE* list, int num, int i)
{
    int rc;
    char* server_ped = NULL;
    if (strlen(list[0].r_ed) > 0)
        server_ped = list[0].r_ed;
    else if (strlen(list[0].r_ped) > 0)
        server_ped = list[0].r_ped;
    else
        return -1;
    rc = send_to_server_request_conn(list[0].fd, list[i].cid, server_ped);
    if (rc >= 0) {
        list[i].r_ed[0] = '\0';
        list[i].r_ped[0] = '\0';
        list[i].connStatus = CONNECTION_REQUEST;
    }
    return 0;
}

// 选择一个空闲的客户端去连接tid
int select_endpoint_conn_to_tid(EPT_NODE* list, int num, char* tid)
{
    int i;
    for (i = 0; i < num; i++) {
        if (list[i].regStatus != ENDPOINT_REGISTER_OK)
            continue;
        if (list[i].conn != NULL && (list[i].connStatus != CONNECTION_INIT && list[i].connStatus != CONNECTION_FAILED))
            continue;
        break;
    }
    if (i >= num) {
        // printf("havn't available endpoint to connect %s\n", tid);
    } else {
        if (list[i].conn != NULL)
            free_rendezvous_connection(list[i].conn);
        list[i].r_ed[0] = '\0';
        list[i].r_ped[0] = '\0';
        list[i].conn = new_rendezvous_connection(list[i].ept, tid, client_service, client_toid, client_ic);
        list[i].connStatus = CONNECTION_REQUEST;
    }
    return 0;
}

int select_endpoint_query(EPT_NODE* list, int num, char* query, char* resp)
{
    int i, rc;
    for (i = 0; i < num; i++) {
        if (list[i].regStatus != ENDPOINT_REGISTER_OK)
            continue;
        break;
    }
    if (i >= num) {
        sprintf(resp, "havn't available endpoint to query %s\n", query);
    } else {
        rc = rendezvous_endpoint_query(list[i].ept, query, resp);
        if (rc != 0) {
            sprintf(resp, "fail to get query, rc = %d\n", rc);
        }
    }
    return 0;
}

int handle_app_message(int fd, char* message, struct sockaddr_in* peer_addr)
{
    char ed[25];
    address_to_endpoint(peer_addr, ed);
    if (strncmp(message, "conn ", 5) == 0) {
        select_endpoint_conn_to_tid(ept_list, NUM_ENDPOINTS, message + 5);
        return 0;
    } else if (strncmp(message, "query ", 6) == 0) {
        char resp[128] = { 0 };
        select_endpoint_query(ept_list, NUM_ENDPOINTS, message + 6, resp);
        resp[strlen(resp)] = '\n';
        sendto(fd, resp, strlen(resp), 0, (struct sockaddr*)peer_addr, sizeof(struct sockaddr_in));
        return 0;
    } else if (app_type == 0) {
        if (strncmp(message, "reply to", 8) == 0) {
            printf("client recv from %s == %s\n", ed, message);
        } else {
            printf("recv from %s == %s\n", ed, message);
            memcpy(message, "reply to ", 9);
            strcpy(message + strlen(message), ed);
            sendto(fd, message, strlen(message), 0, (struct sockaddr*)peer_addr, sizeof(struct sockaddr_in));
        }
    } else if (app_type == 1) {
        // Test MCCS service, 首位类型为1表示从代理服务器收到客户端转发请求
        if (message[0] == '{') {
            printf("recv===%s\n", message);
        } else {
            char* p = message + 13;
            char* last = strrchr(p, '}');
            printf("recv from %s:%s\n", ed, p);
            if (last != NULL) {
                *last = ',';
                strcat(last, "\"reply\":\"ok\"}");
            }
            sendto(fd, message, strlen(p) + 13, 0, (struct sockaddr*)peer_addr, sizeof(struct sockaddr_in));
        }
    }
    return 0;
}

int demo_main(void)
{
    int i, rc = 0, log_type = P2P_LOG_TYPE_NONE;
    char udp_message[1024] = { 0 };
    struct sockaddr_in peer_addr;
    pthread_t extra_thread_id = 0;
    pthread_t extra_thread_id2 = 0;
    KEY_VAL* kv = NULL;
    // 初始化P2P库
    if (debug_output >= 1 && log_level > 0)
        log_type = P2P_LOG_TYPE_SCREEN;
    rc = p2p_init(NULL, demo_log_name, log_type, log_level, server_address, server_port);
    if (rc != 0)
        return rc;
    if (keep_reg_interval > 0)
        set_p2p_reg_keep_interval(keep_reg_interval);
    if (debug_output >= 2)
        set_p2p_option(0, 1);

    // 开启服务端口
    if (strlen(cid_key_file) > 0) {
        kv = (KEY_VAL*)malloc(sizeof(KEY_VAL) * NUM_ENDPOINTS);
        if (kv != NULL) {
            memset(kv, 0, NUM_ENDPOINTS * sizeof(KEY_VAL));
            scan_file(cid_key_file, ':', kv, NUM_ENDPOINTS);
        }
    }
    ept_list = (EPT_NODE*)malloc(sizeof(EPT_NODE) * NUM_ENDPOINTS);
    if (ept_list == NULL) {
        return rc;
    }
    memset(ept_list, 0, NUM_ENDPOINTS * sizeof(EPT_NODE));
    for (i = 0; i < NUM_ENDPOINTS; i++) {
        char *cid, *key;
        cid = key = NULL;
        if (i > 0)
            client_port = 0;
        if (kv != NULL) {
            cid = kv[i].key;
            key = kv[i].value;
        } else if (i == 0) {
            cid = client_id;
            key = client_key;
        }
        ept_list[i].fd = new_udp_socket(client_port, NULL);
        ept_list[i].ept = new_rendezvous_endpoint(cid, client_service, client_oid, client_ic, key, ept_list[i].fd);
        if (ept_list[i].ept != NULL) {
            rendezvous_endpoint_reg(ept_list[i].ept);
            rendezvous_endpoint_eventCallbacks(ept_list[i].ept, ENDPOINT_EVENT_NAT_CHANGE | ENDPOINT_EVENT_CONN_FAIL | ENDPOINT_EVENT_CONN_OK, demo_handle_endpoint_event);
        }
    }
    if (kv != NULL) {
        freeKV(kv, NUM_ENDPOINTS, 0);
        free(kv);
    }

    // 开启周期状态维护线程
    pthread_create(&extra_thread_id, NULL, (void*)status_period, NULL);
    pthread_create(&extra_thread_id2, NULL, (void*)check_endpoint_period, NULL);

    // 接收处理服务器消息
    while (1) {
        fd_set rset;
        socklen_t addr_len = sizeof(struct sockaddr_in);
        rc = check_read(ept_list, NUM_ENDPOINTS, &rset, 0);
        if (rc <= 0) {
            continue;
        }
        for (i = 0; i < NUM_ENDPOINTS; i++) {
            int udp_fd = ept_list[i].fd;
            if (FD_ISSET(udp_fd, &rset)) {
                memset(udp_message, 0, sizeof(udp_message));
                rc = recvfrom(udp_fd, udp_message, sizeof(udp_message), 0, (struct sockaddr*)&peer_addr, &addr_len);
                if (rc <= 0)
                    continue;
                //解析并处理P2P数据包
                rc = handle_rendezvous_packet(udp_fd, udp_message, &peer_addr);
                if (rc != 0) {
                    //处理应用数据
                    handle_app_message(udp_fd, udp_message, &peer_addr);
                }
            }
        }
    }
    return 0;
}

int main(int argc, char* argv[])
{
    int rc;
    phrase_argv(argc, argv);
    if (report_test == 1) {
        p2p_behavior_test();
        return 0;
    }
    // signal(SIGSEGV, sig_handler);
    if (log_level > 0)
        printf("demo server version %s\n", VERSION);
    rc = demo_main();
    return rc;
}

/* ------------------ 下面是老的demo测试 --------------------- */
#if 0
int check_endpoint_event(REND_EPT_HANDLE endpoint, int event)
{
	int status;
	char cid[41], ped[25];
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
			conn = new_rendezvous_connection(endpoint, tid, client_service, client_toid, client_ic);
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
		if(status == CONNECTION_OK || status == CONNECTION_POK) {
			//如果连接建立成功，则可以发送应用数据
			struct sockaddr_in remote_addr;
			strcpy(udp_message, "application data application data application data");
			if(status == CONNECTION_POK){
				char newPort[15] = {0};
				get_rendezvous_connection_attr(conn, "newPort", newPort);
				strcat(udp_message, " with newPort ");
				strcat(udp_message, newPort);
			}
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

int main2(int argc, char *argv[])
{
	int rc = 0;
	int log_type = P2P_LOG_TYPE_NONE;
	char ed[25];
	char udp_message[1024] = {0};
	struct sockaddr_in peer_addr;
	int peer_fd;
	pthread_t extra_thread_id = 0;
	REND_EPT_HANDLE p2p_endpoint;
	REND_CONN_HANDLE p2p_conn = NULL;
	signal(SIGSEGV, sig_handler);
	phrase_argv(argc, argv);
	if(report_test == 1){
		p2p_behavior_test();
		return 0;
	}
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
		p2p_endpoint = new_rendezvous_endpoint(NULL, client_service, client_oid, client_ic, client_key, peer_fd);
	}else{
		p2p_endpoint = new_rendezvous_endpoint(client_id, client_service, client_oid, client_ic, client_key, peer_fd);
	}
	if(p2p_endpoint == NULL) {
		close(peer_fd);
		printf("init rendezvous endpoint fail\n");
		return 0;
	}
	rendezvous_endpoint_reg(p2p_endpoint);
	rendezvous_endpoint_eventCallbacks(p2p_endpoint, ENDPOINT_EVENT_NAT_CHANGE|ENDPOINT_EVENT_CONN_FAIL|ENDPOINT_EVENT_CONN_OK, check_endpoint_event);
	pthread_create(&extra_thread_id, NULL, (void*)status_period, NULL);
	while(1){
		// rendezvous_status_handle();

		//如果指定了目标tid，则注册成功后去连接它，用于测试
		if(strlen(g_tid) > 0 && p2p_conn == NULL){
			p2p_conn = check_endpoint_and_connect(p2p_endpoint, g_tid);
		}

		memset(udp_message, 0, sizeof(udp_message));
		rc = recv_udp_packet(peer_fd, udp_message, sizeof(udp_message), &peer_addr, 0);
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
#endif // old main
