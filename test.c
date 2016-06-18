#include <assert.h>
#include <dirent.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/fcntl.h>

#include "fsplib.h"
#include "./p2pnat/include/p2p_api.h"

char device_id[256]={0};
char fsp_password[60]={0};
char new_fsp_password[60]={0};
char get_url[512]={0};
char save_url[512]={0};
char dir_name[256]={0};
char log_dir[256]=".";
int fsp_method;
char version[]="1.0.2";

static char Usage[] =
"fsp client demo\n"
"Usage: fspClient [options]... \n"
"Option:\n"
"      -v,--version 				 display the version of fspClinet and exit.\n"
"			 -h,--help     				 print this help.\n"
"      -id,--device_id       client uses this id to find server\n"
"      -p,--password         the password of server\n"
"      -g,--get              download a file or a dirent from the server of whose device_id is device_id\n"
"      -s,--save             the local url for saving the download file or dirent \n"
"      -np,--new_password    change the  password of server\n"
"      -ls,--list            display a list of files in the dirent\n";


int phrase_argv(int argc, char *argv[])
{
    int i = 0;

    for (i=1; i<argc; i++)
    {
        if(strcasecmp(argv[i],"--help")==0 || strcasecmp(argv[i],"-h")==0)
        {
            printf("%s",Usage);
            return 0;
        }
        else if(strcasecmp(argv[i],"--version")==0 || strcasecmp(argv[i],"-v")==0)
        {
            printf("%s\n",version);
            return 0;
        }
        if(strcasecmp(argv[i],"--device_id")==0||strcasecmp(argv[i],"-id")==0)
        {
            if(i<argc-1 && argv[i+1]!='-')
                strcpy(device_id,argv[i+1]);
            else strcpy(device_id,"");
        }
        else if(strcasecmp(argv[i],"--password")==0||strcasecmp(argv[i],"-p")==0)
        {
            if(i<argc-1 && argv[i+1]!='-')
                strcpy(fsp_password,argv[i+1]);
            else strcpy(device_id,"");
        }
        else if(strcasecmp(argv[i],"--get")==0||strcasecmp(argv[i],"-g")==0)
        {
            if(i<argc-1 && argv[i+1]!='-')
                strcpy(get_url,argv[i+1]);
            else strcpy(get_url,"");
            fsp_method=FSP_CC_GET_FILE;
        }
        else if(strcasecmp(argv[i],"--save")==0||strcasecmp(argv[i],"-s")==0)
        {
            if(i<argc-1 && argv[i+1]!='-')
                strcpy(save_url,argv[i+1]);
            else strcpy(save_url,"");
        }
        else if(strcasecmp(argv[i],"--new_password")==0||strcasecmp(argv[i],"-np")==0)
        {
            if(i<argc-1 && argv[i+1]!='-')
                strcpy(new_fsp_password,argv[i+1]);
            else strcpy(new_fsp_password,"");
            fsp_method=FSP_CC_CH_PASSWD;
        }
        else if(strcasecmp(argv[i],"--list")==0||strcasecmp(argv[i],"-ls")==0)
        {
            if(i<argc-1 && argv[i+1]!='-')
                strcpy(dir_name,argv[i+1]);
            else strcpy(dir_name,"");
            fsp_method=FSP_CC_GET_DIR;
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
    struct dirent *d;

    dir= fsp_opendir(s,dir_name);
    while((d=fsp_readdir(dir))!=NULL);
    fsp_closedir(dir);
    return 0;
}
//get all of files in a dirent
int get_dir_files_method(FSP_SESSION* s,char* get_url,char* save_url)
{
    FSP_DIR *dir;
    struct dirent *d;

    dir= fsp_opendir(s,dir_name);
    while((d=fsp_readdir(dir))!=NULL)
    {
        sprintf(get_url,"%s%s",get_url,d->d_name);
        sprintf(save_url,"%s%s",save_url,d->d_name);
        get_file_method(s,get_url,save_url);
    }

    fsp_closedir(dir);
    return 0;
}
int get_file_method(FSP_SESSION *s,char* get_url,char* save_url)
{
    FSP_FILE *f;
    FSP_PKT p;
    int i;
    int write_fd;
    char* get_file_name;

    //get file name
    get_file_name=strrchr(get_url,'/');
    if(get_file_name==NULL) get_file_name=get_url;
    else get_file_name++;
    printf("get_file_name-%s\n",get_file_name);

    //save file url
    if(save_url==NULL || *save_url=='\0')
    {
        save_url=get_file_name;
    }
    else if(*(save_url+strlen(save_url)-1)=='/')//e.g. /tmp/Recoder/20160101/
    {
        strcat(save_url,get_file_name);
    }
    printf("save_url-%s\n",save_url);
    //mkdir if no exist
    for(i=1;*(save_url)!='\0';i++)
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
    write_fd=open(save_url,O_WRONLY|O_CREAT|O_TRUNC,S_IRWXU);
    if(write_fd<=0)
    {
        printf("open file-%s error\n",save_url);
        return -2;
    }
    f=fsp_fopen(s,get_url,"rb");
    assert(f);
    while( ( i=fsp_fread(p.buf,1,1000,f) ) )
    {
        write(write_fd,p.buf,i);
    }
    fsp_fclose(f);
    close(write_fd);
    return 0;
}

int main (int argc, char *argv[])
{
    FSP_SESSION* s;
    int rc;
    int p2p_log_type=P2P_LOG_TYPE_FILE;

    phrase_argv(argc,argv);

    if(*device_id=='\0')
    {
        fprintf(stderr,"\n",Usage);
        return 0;
    }
    rc = p2p_init(log_dir,"fspClient",p2p_log_type,5,NULL,0);
    if(rc!=0)
    {
        //printf("p2p init fail\n");
        return 0;
    }

    s = fsp_open_session(device_id,NULL,fsp_password);
    assert(s);
    s->timeout=9000;

    /* diaplay a file list of dir*/
    if(fsp_method==FSP_CC_GET_DIR)
    {
        read_dir_method(s,dir_name);
    }
    /* get a file */
    else if(fsp_method==FSP_CC_GET_FILE)
    {
        if(*(get_url+strlen(get_url)-1)=='/')
            get_dir_files_method(s,get_url,save_url);
        else
            get_file_method(s,get_url,save_url);
    }
    /*change password*/
    else if(fsp_method==FSP_CC_CH_PASSWD)
    {
        fsp_ch_passwd(s,new_fsp_password);
    }
    printf("resends %d, dupes %d, cum. rtt %ld, last rtt %d\n",s->resends,s->dupes,s->rtts/s->trips,s->last_rtt);
    /* bye! */
    fsp_close_session(s);
    return 0;
}
