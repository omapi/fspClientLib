#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <dirent.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
//#include <netinet/in.h>
//#include <sys/socket.h>
//#include <sys/types.h>
#include <sys/fcntl.h>
#include "fsplib.h"
#include "p2p_api.h"

char device_id[256]={0};
char fsp_password[60]={0};
char new_fsp_password[60]={0};
char file_name[60]={0};
char dir_name[256]="";
char log_dir[256]=".";
int fsp_method;

static char Usage[] =
"fsp client demo,getting a file from fsp server to local\n"
"Usage: test [options]\n"
"Option:\n"
"      -i     the fsp server device id,default value is 'OM20'\n"
"      -p     the fsp server password\n"
"      -f    get a file from the fsp server\n"
"      -l     the dir for saving the log file\n"
"      -c     change fsp password\n"
"      -d     get a dirent \n"; 

int phrase_argv(int argc, char *argv[])
{
	int rc = 0;

	while ((rc = getopt(argc, argv, "i:p::f:c::d::")) != -1) {
		switch(rc) {
			case 'i':
				strcpy(device_id,optarg);
				printf("device_id-%s\n",device_id);
				break;
			case 'p':
				if(optarg!=NULL)
				strcpy(fsp_password,optarg);
				printf("fsp_password-%s\n",fsp_password);
				break;
			case 'c':
				fsp_method=FSP_CC_CH_PASSWD;
				if(optarg!=NULL)
				strcpy(new_fsp_password,optarg);
				printf("new_fsp_password-%s\n",new_fsp_password);
				break;
			case 'f':
				fsp_method=FSP_CC_GET_FILE;
				strcpy(file_name,optarg);
				printf("get file name-%s\n",file_name);
				break;
			case 'l':
				strcpy(log_dir,optarg);
				printf("log_dir-%s\n",log_dir);
				break;
			case 'd':
				fsp_method=FSP_CC_GET_DIR;
				if(optarg!=NULL)
					strcpy(dir_name,optarg);
				break;
			default:
				fprintf(stderr,"%s\n",Usage);
				exit(1);
		}
	}
	return 0;
}



int main (int argc, char *argv[])
{
	FSP_SESSION* s;
	FSP_PKT p;
	FSP_FILE *f;
	int rc;
	int i;
	char* save_file_name=NULL;
	int write_fd;
	int write_flag=0;

	FSP_DIR *dir;
	struct dirent *d;

	int p2p_log_type=P2P_LOG_TYPE_FILE;
	phrase_argv(argc,argv);

	if(*device_id=='\0')
	{
		fprintf(stderr,"%s\n",Usage);
		return 0;
	}
	rc = p2p_init(log_dir,"fspClient",p2p_log_type,5,NULL,0);
	if(rc!=0)
	{
		printf("p2p init fail\n");
		return 0;
	}

	s = fsp_open_session(device_id,NULL,fsp_password);
	assert(s);
	s->timeout=9000;

	/* read dir*/
	if(fsp_method==FSP_CC_GET_DIR)
	{
		dir= fsp_opendir(s,dir_name);
		assert(dir);
		while((d=fsp_readdir(dir))!=NULL)
			printf("d_name-%s\n",d->d_name);

		printf("after read dir\n");
		fsp_closedir(dir);
	}
	/* get a file */
	else if(fsp_method==FSP_CC_GET_FILE)
	{
		f=fsp_fopen(s,file_name,"rb");
		assert(f);

		save_file_name=strrchr(file_name,'/');
		if(save_file_name==NULL)
			save_file_name=file_name;
		else save_file_name++;
		printf("save_file_name-%s\n",save_file_name);
		while( ( i=fsp_fread(p.buf,1,1000,f) ) )
		{
			if(write_flag == 0)
			{
				write_fd=open(save_file_name,O_WRONLY|O_CREAT|O_TRUNC,S_IRWXU);

				if(write_fd<=0)
				{
					printf("open file error\n");
					break;
				}
				write_flag=1;
			}
			write(write_fd,p.buf,i);
		}
		fsp_fclose(f);
		close(write_fd);
		if(write_flag==0)
		{
			printf("get file fail\n");
		}
		else printf("get file success\n");
	}
	/*change password*/
	else if(fsp_method==FSP_CC_CH_PASSWD)
	{
		printf("new_fsp_password-%s\n",new_fsp_password);
		fsp_ch_passwd(s,new_fsp_password);
	}
	printf("resends %d, dupes %d, cum. rtt %ld, last rtt %d\n",s->resends,s->dupes,s->rtts/s->trips,s->last_rtt);
	/* bye! */ 
	fsp_close_session(s);
	return 0;
}
