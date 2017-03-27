#ifndef _FSPCLINETDEMO_H
#define  _FSPCLINETDEMO_H 1

typedef struct SERVER_INFO
{
	//basic
	char device_id[256];
	char invite_code[256];
	char password[60];
	char key[256];
	char tid[256];
	char ip[60];
	char mac[60];
	int  port;
}SERVER_INFO;

typedef struct FSP_METHOD_PARAMS
{
	int type_flag;
	//get file method
	char get_url[256];
	char save_url[256];

	//ls method
	char dir_name[256];
	
	//change password method
	char new_password[60];
	//log option
}FSP_METHOD_PARAMS;

#endif
