#include <assert.h>
#include <dirent.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <time.h>

#include "fsplib.h"
#include "./p2pnat/include/p2p_api.h"
#include "error_code.h"

FSP_TSF_CONTR	g_tsf_controller;

char g_device_id[256]={0};
char g_invite_code[256]={0};
char g_fsp_password[60]={0};
char g_new_fsp_password[60]={0};
char g_get_url[512]={0};
char g_save_url[512]={0};
char g_dir_name[256]={0};
char g_log_dir[256]=".";
int g_p2p_log_type=P2P_LOG_TYPE_NONE;
int g_fsp_method;
unsigned int g_max_wait_time=10;
unsigned int g_min_packet_size=768;
unsigned int g_first_resend_time=1340;
unsigned int g_limit_tsf_times=200;
unsigned int g_pkt_change_range=256;
char g_version[]="0.12_v10";
int g_tmp_num=0;
static char g_usage[] =
"fsp client demo\n"
"g_usage: fspClientDemo -id xxx -ic yyy -p zzz [options]... \n"
"\n"
"Option:\n"
"      -id,--device_id              client uses this id to find server\n"
"      -ic,--invite_code            the invite code of FSP server for  p2pnat\n"
"      -p,--password                the password of server\n"
"      -g,--get                     download a file or a dirent from the server of whose device_id is device_id\n"
"      -s,--save                    the local url for saving the download file or dirent \n"
"      -np,--new_password           change the  password of server\n"
"      -ls,--list                   display a list of files in the dirent\n"
"      -v,--version                 display the version of fspClinet and exit.\n"
"      -ps,--prefered_size          preferred size of reply's data block.default is 7348 bytes.If you need higher transfer speed,please using  bigger block size.The max value is 14708 bytes.\n"
"      -frt,--first_resend_time     \n"
"      -mwt,--max_wait_timeout      the maximum waiting time that doesn't recv response from server after serveral failed attempts\n"
"      -d,--debug		    yes,no\n"
"      -h,--help                    print this help.\n"
"\n"
"for example:  ./fspClientDemo -id 8b008c8c-2209-97ab-5143-f0a4aa470023 -ic newrocktech -p newrocktech -ls Recorder/\n";

int phrase_argv(int argc, char *argv[])
{
	int i = 0;
	if( argc == 1)
	{
		printf("Missing parameter:\n%s",g_usage);
		return -2;
	}
	for (i=1; i<argc; i++)
	{
		if(strcasecmp(argv[i],"--help")==0 || strcasecmp(argv[i],"-h")==0)
		{
			printf("%s",g_usage);
			return -3;
		}
		else if(strcasecmp(argv[i],"--version")==0 || strcasecmp(argv[i],"-v")==0)
		{
			printf("%s\n",g_version);
			return -4;
		}
		if(strcasecmp(argv[i],"--device_id")==0||strcasecmp(argv[i],"-id")==0)
		{
			if(i<argc-1 && *argv[i+1]!='-')
			{
				strcpy(g_device_id,argv[i+1]);
				i++;
			}
			else strcpy(g_device_id,"");
		}
		else if(strcasecmp(argv[i],"--password")==0||strcasecmp(argv[i],"-p")==0)
		{
			if(i<argc-1 && *argv[i+1]!='-')
			{
				strcpy(g_fsp_password,argv[i+1]);
				i++;
			}
			else strcpy(g_fsp_password,"");
		}
		else if(strcasecmp(argv[i],"--get")==0||strcasecmp(argv[i],"-g")==0)
		{
			if(i<argc-1 && *argv[i+1]!='-')
			{
				strcpy(g_get_url,argv[i+1]);
				i++;
			}
			else strcpy(g_get_url,"");
			g_fsp_method=FSP_CC_GET_FILE;
		}
		else if(strcasecmp(argv[i],"--save")==0||strcasecmp(argv[i],"-s")==0)
		{
			if(i<argc-1 && *argv[i+1]!='-')
			{
				strcpy(g_save_url,argv[i+1]);
				i++;
			}
			else strcpy(g_save_url,"");
		}
		else if(strcasecmp(argv[i],"--new_password")==0||strcasecmp(argv[i],"-np")==0)
		{
			if(i<argc-1 && *argv[i+1]!='-')
			{
				strcpy(g_new_fsp_password,argv[i+1]);
				i++;
			}
			else strcpy(g_new_fsp_password,"");
			g_fsp_method=FSP_CC_CH_PASSWD;
		}
		else if(strcasecmp(argv[i],"--list")==0||strcasecmp(argv[i],"-ls")==0)
		{
			if(i<argc-1 && *argv[i+1]!='-')
			{
				strcpy(g_dir_name,argv[i+1]);
				i++;
			}
			else strcpy(g_dir_name,"");
			g_fsp_method=FSP_CC_GET_DIR;
		}
		else if(strcasecmp(argv[i],"--invite_code")==0 || strcasecmp(argv[i],"-ic")==0)
		{
			if(i<argc-1 && *argv[i+1]!='-')
			{
				strcpy(g_invite_code,argv[i+1]);
				i++;
			}
			else strcpy(g_invite_code,"");

		}
		else if(strcasecmp(argv[i],"--min_allow_size")==0 || strcasecmp(argv[i],"-min")==0)
		{
			if(i<argc-1 && *argv[i+1]!='-')
			{
				g_min_packet_size=(unsigned int)atoi(argv[i+1]);
				printf("g_min_packet_size-%d\n",g_min_packet_size);
				if(g_min_packet_size>FSP_SPACE||g_min_packet_size<=0)
				{
					printf("the value-%d of min is illegal.It must between 0-%d\n",g_min_packet_size,FSP_SPACE);
					return -1;
				}
				i++;
			}
		}

		else if(strcasecmp(argv[i],"--first_resend_time")==0 || strcasecmp(argv[i],"-frt")==0)
		{
			if(i<argc-1 && *argv[i+1]!='-')
			{
				g_first_resend_time=(unsigned int)atoi(argv[i+1]);
				if(g_first_resend_time<=1000) g_first_resend_time=1000;
				i++;
			}
		}
		else if(strcasecmp(argv[i],"--max_wait_timeout")==0 || strcasecmp(argv[i],"-mwt")==0)
		{
			if(i<argc-1 && *argv[i+1]!='-')
			{
				g_max_wait_time=(unsigned int)atoi(argv[i+1]);
				if(g_max_wait_time<0) g_max_wait_time=5;
				i++;
			}
		}

		else if(strcasecmp(argv[i],"--debug")==0 || strcasecmp(argv[i],"-d")==0)
		{
			if(i<argc-1 && *argv[i+1]!='-')
			{
				if(strcmp(argv[i+1],"yes")==0)
					g_p2p_log_type=P2P_LOG_TYPE_FILE;
				i++;
			}
		}

		else
		{
			printf("Unknown command: %s\n",argv[i]);
			return -1;
		}
	}
	return 0;
}
//display the files list of the dir
int read_dir_method(FSP_SESSION* s,char* dir_name)
{
	FSP_DIR *dir;
	FSP_RDENTRY entry;
	FSP_RDENTRY *result;
	int rc;
	struct dirent *d;
	struct tm* p;
	int num_file=0;
	int num_link=0;
	int num_unkown=0;
	int num_dir=0;
	int num_hide=0;

	dir= fsp_opendir(s,dir_name);

	if(dir==NULL)
		return -1;

	printf("[start]\n");
	while(1)
	{
		if(dir ==NULL )
		{
			printf("[end]\n");
			printf("file-%d ,link-%d ,dir-%d\n",num_file,num_link,num_dir);
			return -1;
		}
		if(dir->dirpos<0 || dir->dirpos % 4) {
			printf("[end]\n");
			printf("file-%d ,link-%d ,dir-%d \n",num_file,num_link,num_dir);
			return -2;
		}

		rc=fsp_readdir_native(dir,&entry,&result);

		if(rc !=0)
		{
			printf("[end]\n");
			printf("file-%d ,link-%d ,dir-%d \n",num_file,num_link,num_dir);
			return rc;
		}
		if(result==NULL)
		{
			printf("[end]\n");
			printf("file-%d ,link-%d ,dir-%d \n",num_file,num_link,num_dir);
			return 0;
		}

		if(*(entry.name)=='.')
		{
			num_hide++;
			continue;
		}
		if((entry.type) == FSP_RDTYPE_FILE)
		{
			num_file++;
			printf("file  ");
		}
		else if((entry.type) == FSP_RDTYPE_DIR)
		{
			num_dir++;
			printf("dir   ");
		}
		else if((entry.type) == FSP_RDTYPE_LINK)
		{
			num_link++;
			printf("link  ");
		}
		else {
			num_unkown++;
			continue;
		}
		p=gmtime(&entry.lastmod);

		printf("%10d %10d/%02d/%02d %02d:%02d:%02d  %s\n",entry.size,(1900+p->tm_year),(1+p->tm_mon),p->tm_mday,p->tm_hour,p->tm_min,p->tm_sec,entry.name);
	}
	printf("[end]\n");
	printf("file-%d ,dir-%d\n",num_file,num_dir);
	fsp_closedir(dir);
	return 0;
}
//get all of files in a dirent
int get_dir_files_method(FSP_SESSION* s,char* f_get_dir_url,char* f_save_dir_url)
{
	FSP_DIR *dir;
	FSP_RDENTRY entry;
	FSP_RDENTRY *result;
	int rc;
	

	char save_dir_url[512];
	char get_file_url[512];
	char save_file_url[512];

	struct stat file_stat;
	
	int used_time;
	float avg_tsf_speed;

	if(f_get_dir_url==NULL || *f_get_dir_url=='\0')
	{
		printf("Failed,code=%d,reason=\"miss get directory or file name\"\n",MISS_PARAMETER_VALUE);
		return -1;
	}

	if(f_save_dir_url==NULL || *f_save_dir_url == '\0')
	{
		if(*f_get_dir_url=='/') strcpy(save_dir_url,f_get_dir_url+1);//save at the curent dir
		else  strcpy(save_dir_url,f_get_dir_url);

		if(*(save_dir_url+strlen(save_dir_url)-1)!='/')
			strcat(save_dir_url,"/");
	}
	else if(*(f_save_dir_url+strlen(f_save_dir_url)-1)!='/')
	{
		sprintf(save_dir_url,"%s/",f_save_dir_url);
	}
	else strcpy(save_dir_url,f_save_dir_url);

	dir= fsp_opendir(s,f_get_dir_url);
	if(dir == NULL)
	{
		printf("Failed,code=%d,reason=\"fsp open dir-%s error\"\n",FSP_OPEN_DIR_FAILED,f_get_dir_url);
		return -2;
	}

	while(1)
	{
		if(dir ==NULL ) break;
		if(dir->dirpos<0 || dir->dirpos % 4)
		{
			rc=-1;
			break;
		}

		rc=fsp_readdir_native(dir,&entry,&result);

		if(rc !=0) break;
		if(result==NULL) break;

		if(*(entry.name)=='.')  continue;
		if((entry.type) == FSP_RDTYPE_FILE)
		{
			sprintf(get_file_url,"%s%s",f_get_dir_url,entry.name);
			sprintf(save_file_url,"%s%s",save_dir_url,entry.name);
			//check if the file has exited at local
				if( stat(save_file_url,&file_stat) == 0 && file_stat.st_size == entry.size)
				{
				printf("%s has exited at local\n",save_file_url);
				continue;
				}
			get_file_method(s,get_file_url,save_file_url,3);
		}
		else if((entry.type) == FSP_RDTYPE_DIR)
		{
			printf("%s is directory ,jump downloading it\n",entry.name);
			continue;
		}
		else if((entry.type) == FSP_RDTYPE_LINK)
		{
			printf("%s is link ,jump downloading it\n",entry.name);
		}
	}
	fsp_closedir(dir);
	
	return 0;
}
int get_file_method(FSP_SESSION *s,char* f_get_url,char* f_save_url,int f_retry)
{
	FSP_FILE *f;
	FSP_PKT p;
	FILE* fp;
	int i;
	char* get_file_name;
	char* get_url=f_get_url;
	char save_url[512];
	int error_flag=1;

	//get file name
	get_file_name=strrchr(get_url,'/');
	if(get_file_name==NULL) get_file_name=get_url;
	else get_file_name++;
	//printf("get_file_name-%s\n",get_file_name);

	//save file url
	if(f_save_url==NULL || *f_save_url=='\0')
	{
		strcpy(save_url,get_file_name);
	}
	else if(*(f_save_url+strlen(f_save_url)-1)=='/')//e.g. /tmp/Recoder/20160101/
	{
		sprintf(save_url,"%s%s",f_save_url,get_file_name);
	}
	else strcpy(save_url,f_save_url);

	//printf("save_url-%s\n",save_url);
	//mkdir if no exist
	for(i=1;*(save_url+i)!='\0';i++)
	{
		if(*(save_url+i)== '/')
		{
			*(save_url+i)='\0';
			if(access(save_url,F_OK) == -1)
			{
				if(mkdir(save_url,00700) == -1)
					return -1;
			}
			*(save_url+i)='/';
		}
	}

	//read from remote and write at local
	if(access(save_url,F_OK) == 0)
	{
		// printf("file-%s has exit\n",save_url);
		remove(save_url);
	}

	fp=fopen(save_url,"wb+");
	if(fp==NULL)
	{
		printf("Failed,code=%d,reason=\"open file-%s error\"\n",FILE_OPEN_FAILED,save_url);
		return -2;
	}
	//printf("open file for writing-%s\n",save_url);
	f=fsp_fopen(s,get_url,"rb");
	assert(f);
	if(f==NULL)
	{
		printf("Failed,code=%d,reason=\"fsp fopen file-%s error\"\n",FSP_OPEN_FILE_FAILED,get_url);
		return -1;
	}
	printf("start get file %s\n",get_url);

	while( ( i=fsp_fread(p.buf,1,g_tsf_controller.cur_unit.pkt_size,f) ) )
	{
		error_flag=0;
		fwrite(p.buf,1,i,fp);
		printf("=");
		fflush(stdout);
	}
	if(f->err!=0)
	{
        printf("\n");
		error_flag=f->err;
		remove(save_url);
	}

	fsp_fclose(f);
	fclose(fp);
	if(error_flag==3&&f_retry>0)//timeout
	{
		remove(save_url);
		printf("Warning,code=%d,reason=\"waiting timeout,try again\n",FSP_WAITING_TIMEOUT_WARNING);
		g_tsf_controller.init_pkt_size=g_tsf_controller.max_speed_unit.avg_tsf_speed;
		init_tsf_unit(&g_tsf_controller.cur_unit,g_tsf_controller.init_pkt_size,g_tsf_controller.limit_tsf_times);	
		f_retry--;
		get_file_method(s,f_get_url,f_save_url,f_retry);
		return -3;
	}
	else if(error_flag!=0)
	{
		printf("Failed,code=%d,reason=\"the file -%s read error\"\n",FSP_READ_FILE_FAILED,get_url);
		return -5;
	}
	printf("end\n");
	return 0;
}

int main (int argc, char *argv[])
{
	FSP_SESSION* s;
	int rc;
	time_t now1;
	time_t now2;
	time_t now3;

	rc=phrase_argv(argc,argv);

	if(rc<0) return 0;
	if(*g_device_id=='\0')
	{
		printf("stderr-%s\n",g_usage);
		return 0;
	}
	if(*g_device_id=='\0')
	time(&now1);
	printf("start p2p time-%ld\n",now1);
	rc = p2p_init(g_log_dir,"fspClient",g_p2p_log_type,5,NULL,0);
	// for debug
	//set_p2p_option(0,1);
	if(rc!=0)
	{
		//printf("p2p init fail\n");
		printf("Failed,code=%d,reason=\"init p2p failed\"\n",P2P_INIT_FAILED);
		return 0;
	}

	s = fsp_open_session(g_device_id,g_invite_code,NULL,g_fsp_password);
	if(s==NULL) return 0;
	assert(s);
	s->timeout=g_max_wait_time*1000;

	time(&now2);
	printf("p2p using time-%ds\n",now2-now1);

	init_tsf_controller(&g_tsf_controller);
	/* diaplay a file list of dir*/

	if(g_fsp_method==FSP_CC_GET_DIR)
	{
		rc=read_dir_method(s,g_dir_name);
		if(rc!=0) printf("Failed,code=%d,reason=\"read dir-%s failed\"\n",FSP_READ_DIR_FAILED,g_dir_name);
	}
	/* get a file */
	else if(g_fsp_method==FSP_CC_GET_FILE)
	{
		if(*(g_get_url+strlen(g_get_url)-1)=='/')
			get_dir_files_method(s,g_get_url,g_save_url);
		else
			get_file_method(s,g_get_url,g_save_url,3);
	}
	/*change password*/
	else if(g_fsp_method==FSP_CC_CH_PASSWD)
	{
		//printf("new_password-%s\n",g_new_fsp_password);
		fsp_ch_passwd(s,g_new_fsp_password);
	}

	stop_tsf_controller(&g_tsf_controller);
	time(&now3);
	printf("fsp using time %ds\n",now3-now2);
	if(s!=NULL)	printf("resends %d, dupes %d, cum. rtt %ld, last rtt %d\n",s->resends,s->dupes,s->rtts/s->trips,s->last_rtt);
	/* bye! */
	if(s!=NULL)
	fsp_close_session(s);
	return 0;
}
