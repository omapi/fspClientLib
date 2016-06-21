#include <assert.h>
#include <dirent.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/fcntl.h>

#include "fsplib.h"
#include "./p2pnat/include/p2p_api.h"

char g_device_id[256]={0};
char g_fsp_password[60]={0};
char g_new_fsp_password[60]={0};
char g_get_url[512]={0};
char g_save_url[512]={0};
char g_dir_name[256]={0};
char g_log_dir[256]=".";
int g_fsp_method;
char g_version[]="0.12_v3";

static char g_usage[] =
"fsp client demo\n"
"g_usage: fspClientDemo [options]... \n"
"Option:\n"
"      -v,--version          display the version of fspClinet and exit.\n"
"      -h,--help             print this help.\n"
"      -id,--device_id       client uses this id to find server\n"
"      -p,--password         the password of server\n"
"      -g,--get              download a file or a dirent from the server of whose device_id is device_id\n"
"      -s,--save             the local url for saving the download file or dirent \n"
"      -np,--new_password    change the  password of server\n"
"      -ls,--list            display a list of files in the dirent\n";


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
            return 0;
        }
        else if(strcasecmp(argv[i],"--version")==0 || strcasecmp(argv[i],"-v")==0)
        {
            printf("%s\n",g_version);
            return 0;
        }
        if(strcasecmp(argv[i],"--device_id")==0||strcasecmp(argv[i],"-id")==0)
        {
            if(i<argc-1 && argv[i+1]!='-')
            {
                strcpy(g_device_id,argv[i+1]);
                i++;
            }
            else strcpy(g_device_id,"");
        }
        else if(strcasecmp(argv[i],"--password")==0||strcasecmp(argv[i],"-p")==0)
        {
            if(i<argc-1 && argv[i+1]!='-')
            {
                strcpy(g_fsp_password,argv[i+1]);
                i++;
            }
            else strcpy(g_fsp_password,"");
        }
        else if(strcasecmp(argv[i],"--get")==0||strcasecmp(argv[i],"-g")==0)
        {
            if(i<argc-1 && argv[i+1]!='-')
            {
                strcpy(g_get_url,argv[i+1]);
                i++;
            }
            else strcpy(g_get_url,"");
            g_fsp_method=FSP_CC_GET_FILE;
        }
        else if(strcasecmp(argv[i],"--save")==0||strcasecmp(argv[i],"-s")==0)
        {
            if(i<argc-1 && argv[i+1]!='-')
            {
                strcpy(g_save_url,argv[i+1]);
                i++;
            }
            else strcpy(g_save_url,"");
        }
        else if(strcasecmp(argv[i],"--new_password")==0||strcasecmp(argv[i],"-np")==0)
        {
            if(i<argc-1 && argv[i+1]!='-')
            {
                strcpy(g_new_fsp_password,argv[i+1]);
                i++;
            }
            else strcpy(g_new_fsp_password,"");
            g_fsp_method=FSP_CC_CH_PASSWD;
        }
        else if(strcasecmp(argv[i],"--list")==0||strcasecmp(argv[i],"-ls")==0)
        {
            if(i<argc-1 && argv[i+1]!='-')
            {
                strcpy(g_dir_name,argv[i+1]);
                i++;
            }
            else strcpy(g_dir_name,"");
            g_fsp_method=FSP_CC_GET_DIR;
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

    dir= fsp_opendir(s,dir_name);
    while(1)
    {
        if(dir ==NULL ) return -1;
        if(dir->dirpos<0 || dir->dirpos % 4) return -2;

        rc=fsp_readdir_native(dir,&entry,&result);

        if(rc !=0) return rc;
        if(result==NULL) return 0;

        if(*(entry.name)=='.')  continue;
        if((entry.type) == FSP_RDTYPE_FILE)
        {
            printf("file ");
        }
        else if((entry.type) == FSP_RDTYPE_DIR)
        {
            printf("dir  ");
        }
        else if((entry.type) == FSP_RDTYPE_LINK)
        {
            printf("link ");
        }
        p=gmtime(&entry.lastmod);
        printf("%10d %10d/%02d/%02d %02d:%02d:%02d  %s\n",entry.size,(1900+p->tm_year),(1+p->tm_mon),p->tm_mday,p->tm_hour,p->tm_min,p->tm_sec,entry.name);
    }

    fsp_closedir(dir);
    return 0;
}
//get all of files in a dirent
int get_dir_files_method(FSP_SESSION* s,char* f_get_dir_url,char* f_save_dir_url)
{
    FSP_DIR *dir;
    struct dirent *d;
    char save_dir_url[512];
    char get_file_url[512];
    char save_file_url[512];
    if(f_get_dir_url==NULL || *f_get_dir_url=='\0') return -1;

    if(f_save_dir_url==NULL ||*f_save_dir_url == '\0')
    {
        strcpy(save_dir_url,f_get_dir_url);
    }
    else if(*(f_save_dir_url+strlen(f_save_dir_url)-1)!='/')
    {
        sprintf(save_dir_url,"%s/",f_save_dir_url);
    }
    else strcpy(save_dir_url,f_save_dir_url);

    dir= fsp_opendir(s,f_get_dir_url);
    while((d=fsp_readdir(dir))!=NULL)
    {
        if(*(d->d_name)=='.') continue;

        sprintf(get_file_url,"%s%s",f_get_dir_url,d->d_name);
        sprintf(save_file_url,"%s%s",save_dir_url,d->d_name);
        printf("get -%s ,save -%s\n",get_file_url,save_file_url);
        get_file_method(s,get_file_url,save_file_url);
    }

    fsp_closedir(dir);
    return 0;
}
int get_file_method(FSP_SESSION *s,char* f_get_url,char* f_save_url)
{
    FSP_FILE *f;
    FSP_PKT p;
    FILE* fp;
    int i;
    char* get_file_name;
    char* get_url=f_get_url;
    char save_url[512];

    //get file name
    get_file_name=strrchr(get_url,'/');
    if(get_file_name==NULL) get_file_name=get_url;
    else get_file_name++;
    printf("get_file_name-%s\n",get_file_name);

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

    printf("save_url-%s\n",save_url);
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
        //printf("open file-%s error\n",save_url);
        return -2;
    }
    //printf("open file for writing-%s\n",save_url);
    f=fsp_fopen(s,get_url,"rb");
    assert(f);
    while( ( i=fsp_fread(p.buf,1,1000,f) ) )
    {
        fwrite(p.buf,1,i,fp);
    }
    fsp_fclose(f);
    fclose(fp);
    //printf("write over\n");
    return 0;
}

int main (int argc, char *argv[])
{
    FSP_SESSION* s;
    int rc;
    int p2p_log_type=P2P_LOG_TYPE_FILE;

    phrase_argv(argc,argv);

    if(*g_device_id=='\0')
    {
        fprintf(stderr,"\n",g_usage);
        return 0;
    }
    rc = p2p_init(g_log_dir,"fspClient",p2p_log_type,5,NULL,0);
    if(rc!=0)
    {
        //printf("p2p init fail\n");
        return 0;
    }

    s = fsp_open_session(g_device_id,NULL,g_fsp_password);
    if(s==NULL) return 0;
    assert(s);
    s->timeout=9000;

    /* diaplay a file list of dir*/
    if(g_fsp_method==FSP_CC_GET_DIR)
    {
        read_dir_method(s,g_dir_name);
    }
    /* get a file */
    else if(g_fsp_method==FSP_CC_GET_FILE)
    {
        if(*(g_get_url+strlen(g_get_url)-1)=='/')
            get_dir_files_method(s,g_get_url,g_save_url);
        else
            get_file_method(s,g_get_url,g_save_url);
    }
    /*change password*/
    else if(g_fsp_method==FSP_CC_CH_PASSWD)
    {
        fsp_ch_passwd(s,g_new_fsp_password);
    }
    //printf("resends %d, dupes %d, cum. rtt %ld, last rtt %d\n",s->resends,s->dupes,s->rtts/s->trips,s->last_rtt);
    /* bye! */
    fsp_close_session(s);
    return 0;
}
