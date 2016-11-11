#include <assert.h>
#include <dirent.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <time.h>
#include <string.h>

#include "fsplib.h"
#include "./p2pnat/include/p2p_api.h"
#include "error_code.h"

SERVER_INFO     g_server_info;
FSP_TSF_CONTR	g_tsf_controller;
FSP_METHOD_PARAMS g_fsp_method;
int g_p2p_log_type=P2P_LOG_TYPE_NONE;

char g_version[]="0.12_v11";

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
"      -d,--debug		    yes,no\n"
"      -min,--min_pkt_size    	    the min paket size of transmission,the min value is 256\n"
"      -max,--max_pkt_size    	    the max paket size of transmission,the max value is 14708\n"
"      -h,--help                    print this help.\n"
"\n"
"for example:  ./fspClientDemo -id 8b008c8c-2209-97ab-5143-f0a4aa470023 -ic newrocktech -p newrocktech -ls Recorder/\n";

int phrase_argv(int argc, char *argv[])
{
	int i = 0;
	int int_tmp;
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

		//server info
		if(strcasecmp(argv[i],"--device_id")==0||strcasecmp(argv[i],"-id")==0)
		{
			if(i<argc-1 && *argv[i+1]!='-')
			{
				strcpy(g_server_info.device_id,argv[i+1]);
				i++;
			}
			else strcpy(g_server_info.device_id,"");
		}
		else if(strcasecmp(argv[i],"--invite_code")==0 || strcasecmp(argv[i],"-ic")==0)
		{
			if(i<argc-1 && *argv[i+1]!='-')
			{
				strcpy(g_server_info.invite_code,argv[i+1]);
				i++;
			}
			else strcpy(g_server_info.invite_code,"");

		}
		else if(strcasecmp(argv[i],"--password")==0||strcasecmp(argv[i],"-p")==0)
		{
			if(i<argc-1 && *argv[i+1]!='-')
			{
				strcpy(g_server_info.password,argv[i+1]);
				i++;
			}
			else strcpy(g_server_info.password,"");
		}
		//get file or dir method
		else if(strcasecmp(argv[i],"--get")==0||strcasecmp(argv[i],"-g")==0)
		{
			if(i<argc-1 && *argv[i+1]!='-')
			{
				strcpy(g_fsp_method.get_url,argv[i+1]);
				i++;
			}
			g_fsp_method.type_flag=FSP_CC_GET_FILE;
		}
		else if(strcasecmp(argv[i],"--save")==0||strcasecmp(argv[i],"-s")==0)
		{
			if(i<argc-1 && *argv[i+1]!='-')
			{
				strcpy(g_fsp_method.save_url,argv[i+1]);
				i++;
			}
		}
		//change password method
		else if(strcasecmp(argv[i],"--new_password")==0||strcasecmp(argv[i],"-np")==0)
		{
			if(i<argc-1 && *argv[i+1]!='-')
			{
				strcpy(g_fsp_method.new_password,argv[i+1]);
				i++;
			}
			g_fsp_method.type_flag=FSP_CC_CH_PASSWD;
		}
		//ls method
		else if(strcasecmp(argv[i],"--list")==0||strcasecmp(argv[i],"-ls")==0)
		{
			if(i<argc-1 && *argv[i+1]!='-')
			{
				strcpy(g_fsp_method.dir_name,argv[i+1]);
				i++;
			}
			g_fsp_method.type_flag=FSP_CC_GET_DIR;
		}
		//options
		else if(strcasecmp(argv[i],"--min_pkt_size")==0 || strcasecmp(argv[i],"-min")==0)
		{
			if(i<argc-1 && *argv[i+1]!='-')
			{
				int_tmp=(unsigned  int)atoi(argv[i+1]);
				if(int_tmp>=256 && int_tmp<=g_tsf_controller.max_pkt_size)
				{
					g_tsf_controller.min_pkt_size=int_tmp;
					g_tsf_controller.cur_pkt_size=int_tmp;
				}
				//printf("min_pkt_size-%d\n",g_tsf_controller.min_pkt_size);
				i++;
			}
		}
		else if(strcasecmp(argv[i],"--max_packet_size")==0 || strcasecmp(argv[i],"-max")==0)
		{
			if(i<argc-1 && *argv[i+1]!='-')
			{
				int_tmp=(unsigned int)atoi(argv[i+1]);
				if(int_tmp>= g_tsf_controller.min_pkt_size && int_tmp <= FSP_SPACE) g_tsf_controller.max_pkt_size=int_tmp;
				//printf("max_pkt_size-%d\n",g_tsf_controller.max_pkt_size);
				i++;
			}
		}
		else if(strcasecmp(argv[i],"--tsf_cicrcle_times")==0 || strcasecmp(argv[i],"-tcts")==0)
		{
			if(i<argc-1 && *argv[i+1]!='-')
			{
				int_tmp=(unsigned int)atoi(argv[i+1]);
				if(int_tmp>0) g_tsf_controller.circle_times=int_tmp;
				i++;
			}
		}
		else if(strcasecmp(argv[i],"--tsf_cicrcle_time")==0 || strcasecmp(argv[i],"-tct")==0)
		{
			if(i<argc-1 && *argv[i+1]!='-')
			{
				int_tmp=(unsigned int)atoi(argv[i+1]);
				if(int_tmp>0) g_tsf_controller.circle_time=int_tmp;
				i++;
			}
		}
		else if(strcasecmp(argv[i],"--pkt_ch_range")==0 || strcasecmp(argv[i],"-pcr")==0)
		{
			if(i<argc-1 && *argv[i+1]!='-')
			{
				int_tmp=(unsigned int)atoi(argv[i+1]);
				if(int_tmp>0) g_tsf_controller.pkt_ch_range=int_tmp;
				i++;
			}
		}

   	    //others
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
	
	int used_time_len;
	int total_count=0;
	int total_size=0;

	if(f_get_dir_url==NULL || *f_get_dir_url=='\0')
	{
		printf("\nFailed,code=%d,reason=\"miss get directory or file name\"\n",MISS_PARAMETER_VALUE);
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

	
	//get info: file count,total size ,and so on
	dir= fsp_opendir(s,f_get_dir_url);
	if(dir == NULL)
	{
		printf("\nFailed,code=%d,reason=\"fsp open dir-%s error\"\n",FSP_OPEN_DIR_FAILED,f_get_dir_url);
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
			total_count++;
			total_size+=entry.size;
		}
		else  continue;
	}
	fsp_closedir(dir);
	g_tsf_controller.total_size=total_size;
	//download
	dir= fsp_opendir(s,f_get_dir_url);
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
				g_tsf_controller.done_size+=entry.size;
				//printf("%s has exited at local\n",save_file_url);
				continue;
			}
			get_file_method(s,get_file_url,save_file_url,3);
			//done_size += entry.size;
		}
		else if((entry.type) == FSP_RDTYPE_DIR)
		{
			//printf("%s is directory ,jump downloading it\n",entry.name);
			continue;
		}
		else if((entry.type) == FSP_RDTYPE_LINK)
		{
			//printf("%s is link ,jump downloading it\n",entry.name);
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

	memset(&p,0,sizeof(FSP_PKT));

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
		printf("\nFailed,code=%d,reason=\"open file-%s error\"\n",FILE_OPEN_FAILED,save_url);
		return -2;
	}
	//printf("open file for writing-%s\n",save_url);
	f=fsp_fopen(s,get_url,"rb");
	assert(f);
	if(f==NULL)
	{
		printf("\nFailed,code=%d,reason=\"fsp fopen file-%s error\"\n",FSP_OPEN_FILE_FAILED,get_url);
		return -1;
	}
//	printf("start get file %s\n",get_url);

	while( ( i=fsp_fread(p.buf,1,g_tsf_controller.cur_unit.pkt_size,f) ) )
	{
		//error_flag=0;
		fwrite(p.buf,1,i,fp);
		//printf("=");
		//fflush(stdout);
	}
//	if(f->err!=0)
//	{
        //printf("\n");
		error_flag=f->err;
		if(error_flag!=0)	remove(save_url);
//	}

	fsp_fclose(f);
	fclose(fp);
	if(error_flag==3&&f_retry>0)//timeout
	{
		remove(save_url);
		printf("\nWarning,code=%d,reason=\"waiting timeout,try again\"\n",FSP_WAITING_TIMEOUT_WARNING);
		if(f_retry>2)
		{
			g_tsf_controller.cur_pkt_size=g_tsf_controller.max_speed_unit.pkt_size;
			g_tsf_controller.max_speed_flag=1;
		}
		else if(f_retry>1)
		{
			g_tsf_controller.cur_pkt_size=1024;
			g_tsf_controller.max_speed_flag=0;
		}
		else
		{
			g_tsf_controller.cur_pkt_size=g_tsf_controller.min_pkt_size;
			g_tsf_controller.max_speed_flag=0;
		}
		init_tsf_unit(&g_tsf_controller);
		f_retry--;
		get_file_method(s,f_get_url,f_save_url,f_retry);
		return -3;
	}
	else if(error_flag!=0)
	{
		printf("\nFailed,code=%d,reason=\"the file -%s read error\"\n",FSP_READ_FILE_FAILED,get_url);
		return -5;
	}
	//printf("end\n");
	return 0;
}

int main (int argc, char *argv[])
{
	FSP_SESSION* s;
	int rc;
	time_t now1;
	time_t now2;
	time_t now3;

	//init
	memset(&g_server_info,0,sizeof(SERVER_INFO));
	memset(&g_fsp_method,0,sizeof(FSP_METHOD_PARAMS));
	init_tsf_controller(&g_tsf_controller);
	init_tsf_unit(&g_tsf_controller);

	//get input
	rc=phrase_argv(argc,argv);
	if(rc<0) return 0;

	time(&now1);
	printf("start p2p ...\n");;

	//p2p_init
	rc = p2p_init(".","fspClient",g_p2p_log_type,5,NULL,0);
	if(rc!=0)
	{
		//printf("p2p init fail\n");
		printf("Failed,code=%d,reason=\"init p2p failed\"\n",P2P_INIT_FAILED);
		return 0;
	}

	s = fsp_open_session(&g_server_info);
	if(s==NULL) return 0;
	assert(s);

	time(&now2);
	printf("p2p used time len-%ld s\n",now2-now1);


	//do method
	/* diaplay a file list of dir*/
	if(g_fsp_method.type_flag==FSP_CC_GET_DIR)
	{
		g_tsf_controller.direction=DOWN;
		rc=read_dir_method(s,g_fsp_method.dir_name);
		if(rc!=0) printf("Failed,code=%d,reason=\"read dir-%s failed\"\n",FSP_READ_DIR_FAILED,g_fsp_method.dir_name);
	}
	/* get a file */
	else if(g_fsp_method.type_flag==FSP_CC_GET_FILE)
	{
		g_tsf_controller.direction=DOWN;
		if(*(g_fsp_method.get_url+strlen(g_fsp_method.get_url)-1)=='/')
		{
			get_dir_files_method(s,g_fsp_method.get_url,g_fsp_method.save_url);
		}
		else
		{
			get_file_method(s,g_fsp_method.get_url,g_fsp_method.save_url,3);
		}
	}
	/*change password*/
	else if(g_fsp_method.type_flag==FSP_CC_CH_PASSWD)
	{
		//printf("new_password-%s\n",g_new_fsp_password);
		fsp_ch_passwd(s,g_fsp_method.new_password);
	}

	//done method
	stop_tsf_controller(&g_tsf_controller);
	time(&now3);
	//printf("fsp using time %lds\n",now3-now2);
	if(s!=NULL)	printf("resends %d, dupes %d, cum. rtt %ld, last rtt %d\n",s->resends,s->dupes,s->rtts/s->trips,s->last_rtt);
	/* bye! */
	if(s!=NULL)
	fsp_close_session(s);
	return 0;
}
