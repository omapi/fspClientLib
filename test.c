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

char device_id[60]="OM20";
char device_key[60]="";
char fsp_password[32]="";
char file_name[60]="xxfan";
static char Usage[] =
"fsp client demo,getting a file from fsp server to local\n"
"Usage: test [options]\n"
"Option:\n"
"      -i     the fsp server device id,default value is 'OM20'\n"
"      -k     the fsp server device key\n"
"      -p     the fsp server password\n"
"      -g     get a file from the fsp server\n";

int phrase_argv(int argc, char *argv[])
{
	int rc = 0;

	while ((rc = getopt(argc, argv, "i:k:p:g:")) != -1) {
		switch(rc) {
			case 'i':
				strcpy(device_id,optarg);
				break;
			case 'k':
				strcpy(device_key,optarg);
				break;
			case 'p':
				strcpy(fsp_password,optarg);
				break;
			case 'g':
				strcpy(file_name,optarg);
				break;
			default:
				fprintf(stderr,"%s\n",Usage);
				exit(1);
		}
	}
	return 0;
}
int	create_dir_if_absent(char* url)
{

	int i;
	for(i=1;url[i]!='\0';i++)
	{
		if(url[i] == '/')
		{
			url[i]='\0';	

			if(access(url,NULL)!=0)
			{
				if(mkdir(url,0755) == -1)
				{
					return -1;
				}
				else 
				{
					printf("mkdir-%s\n",url);
				}
			}
			url[i]='/';
		}
	}
	return 0;
}


int main (int argc, char *argv[])
{
	int i;
	int rc;
	int write_fd;
	FSP_SESSION* s;
	FSP_PKT p;
	FSP_FILE *f;
	//	struct dirent *d;
	//	FSP_DIR *dir;

	int p2p_log_type= P2P_LOG_TYPE_FILE;
	phrase_argv(argc,argv);

	printf("device_id-%s,password-%s,file_name-%s\n",device_id,fsp_password,file_name);
	p2p_init("./","fspClient",p2p_log_type,5,NULL,0);
	s=fsp_open_session(device_id,NULL,NULL);
	assert(s);
	s->timeout=9000;

	/* dir list test */

	/*	dir=fsp_opendir(s,"/");
		assert(dir);

		while ( (d=fsp_readdir(dir)) != NULL )
		printf("d_name-%s\n",d->d_name);

		printf("after read dir\n");
		fsp_closedir(dir);	
	 */
	/* get a file */
	f=fsp_fopen(s,file_name,"rb");
	assert(f);
	

	rc=create_dir_if_absent(file_name);
	if(rc==-1)
	{
		printf("mkdir error\n");
	}
	else
	{
		write_fd=open(file_name,O_RDWR|O_CREAT);
		if(write_fd>0)
		{
			while( ( i=fsp_fread(p.buf,1,1000,f) ) )
			{
				write(write_fd,p.buf,i);
			}
			fsp_fclose(f);
			close(write_fd);
		}
		else
		{
			printf("open file error\n");
			return 0;
		}

		printf("resends %d, dupes %d, cum. rtt %ld, last rtt %d\n",s->resends,s->dupes,s->rtts/s->trips,s->last_rtt);
	}
	/* bye! */ 
	fsp_close_session(s);
	return 0;
}
