/*
   This file is part of fsplib - FSP protocol stack implemented in C
   language. See http://fsp.sourceforge.net for more information.

   Copyright (c) 2003-2005 by Radim HSN Kolar (hsn@sendmail.cz)

   You may copy or modify this file in any manner you wish, provided
   that this notice is always included, and that you hold the author
   harmless for any loss or damage resulting from the installation or
   use of this software.

   This is a free software.  Be creative.
   Let me know of any bugs and suggestions.
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>

#include <time.h>

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

#include "fsplib.h"
#include "lock.h"
//p2pnat
#include "p2p_api.h"
#include "error_code.h"
extern unsigned int g_preferred_size;
extern unsigned int g_min_packet_size;
extern unsigned int g_first_resend_time;
extern int g_tmp_num;

int g_no_retry_count=0;

double  g_lost_packet_rate=0;
double  g_min_lost_packet_rate=0;
double g_min_lost_packet_size=512;

int g_send_packet_count=0;

int g_last_preferred_size=FSP_SPACE;
/* ************ Internal functions **************** */

/* builds filename in packet output buffer, appends password if needed */
static int buildfilename(const FSP_SESSION *s,FSP_PKT *out,const char *dirname)
{
	int len;

	len=strlen(dirname);
	if(len >= FSP_SPACE - 1)
	{
		errno = ENAMETOOLONG;
		return -1;
	}
	/* copy name + \0 */
	memcpy(out->buf,dirname,len+1);
	out->len=len;
	if(s->password)
	{
		out->buf[len]='\n';
		out->len++;

		len=strlen(s->password);
		if(out->len+ len >= FSP_SPACE -1 )
		{
			errno = ENAMETOOLONG;
			return -1;
		}
		memcpy(out->buf+out->len,s->password,len+1);
		out->len+=len;
	}
	/* add terminating \0 */
	out->len++;
	return 0;
}

/* simple FSP command */
static int simplecommand(FSP_SESSION *s,const char *directory,unsigned char command)
{
	FSP_PKT in,out;

	if(buildfilename(s,&out,directory))
		return -1;
	out.cmd=command;
	out.xlen=0;
	out.pos=0;

	if(fsp_transaction(s,&out,&in))
		return -1;

	if(in.cmd == FSP_CC_ERR)
	{
		errno = EPERM;
		return -1;
	}

	errno = 0;
	return  0;
}
/* Get directory part of filename. You must free() the result */
static char * directoryfromfilename(const char *filename)
{
	char *result;
	char *tmp;
	int pos;

	result=strrchr(filename,'/');
	if (result == NULL)
		return strdup("");
	pos=result-filename;
	tmp=malloc(pos+1);
	if(!tmp)
		return NULL;
	memcpy(tmp,filename,pos);
	tmp[pos]='\0';
	return tmp;
}

/* ************  Packet encoding / decoding *************** */

/* write binary representation of FSP packet p into *space. */
/* returns number of bytes used or zero on error            */
/* Space must be long enough to hold created packet.        */
/* Maximum created packet size is FSP_MAXPACKET             */

size_t fsp_pkt_write(const FSP_PKT *p,void *space)
{
	size_t used;
	unsigned char *ptr;
	int checksum;
	size_t i;

	if(p->xlen + p->len > FSP_SPACE )
	{
		/* not enough space */
		errno = EMSGSIZE;
		return 0;
	}
	ptr=space;

	/* pack header */
	ptr[FSP_OFFSET_CMD]=p->cmd;
	ptr[FSP_OFFSET_SUM]=0;
	*(uint16_t *)(ptr+FSP_OFFSET_KEY)=htons(p->key);
	*(uint16_t *)(ptr+FSP_OFFSET_SEQ)=htons(p->seq);
	*(uint16_t *)(ptr+FSP_OFFSET_LEN)=htons(p->len);
	*(uint32_t *)(ptr+FSP_OFFSET_POS)=htonl(p->pos);
	used=FSP_HSIZE;
	/* copy data block */
	memcpy(ptr+FSP_HSIZE,p->buf,p->len);
	used+=p->len;
	/* copy extra data block */
	memcpy(ptr+used,p->buf+p->len,p->xlen);
	used+=p->xlen;

	/* compute checksum */
	checksum = 0;
	for(i=0;i<used;i++)
	{
		checksum += ptr[i];
	}
	checksum +=used;
	ptr[FSP_OFFSET_SUM] =  checksum + (checksum >> 8);
	return used;
}

/* read binary representation of FSP packet received from network into p  */
/* return zero on success */
int fsp_pkt_read(FSP_PKT *p,const void *space,size_t recv_len)
{
	int mysum;
	size_t i;
	const unsigned char *ptr;


	if(recv_len<FSP_HSIZE)
	{
		/* too short */
		errno = ERANGE;
		return -1;
	}
	if(recv_len>FSP_MAXPACKET)
	{
		/* too long */
		errno = EMSGSIZE;
		return -1;
	}

	ptr=space;
	/* check sum */
	mysum=-ptr[FSP_OFFSET_SUM];
	for(i=0;i<recv_len;i++)
	{
		mysum+=ptr[i];
	}

	mysum = (mysum + (mysum >> 8)) & 0xff;

	if(mysum != ptr[FSP_OFFSET_SUM])
	{
		/* checksum failed */

#ifdef MAINTAINER_MODE
		printf("mysum: %x, got %x\n",mysum,ptr[FSP_OFFSET_SUM]);
#endif
		errno = EIO;
		return -1;
	}

	/* unpack header */
	p->cmd=ptr[FSP_OFFSET_CMD];
	p->sum=mysum;
	p->key=ntohs( *(const uint16_t *)(ptr+FSP_OFFSET_KEY) );
	p->seq=ntohs( *(const uint16_t *)(ptr+FSP_OFFSET_SEQ) );
	p->len=ntohs( *(const uint16_t *)(ptr+FSP_OFFSET_LEN) );
	p->pos=ntohl( *(const uint32_t *)(ptr+FSP_OFFSET_POS) );
	if(p->len > recv_len)
	{
		/* bad length field, should not never happen */
		errno = EMSGSIZE;
		return -1;
	}
	p->xlen=recv_len - p->len - FSP_HSIZE;
	/* now copy data */
	memcpy(p->buf,ptr+FSP_HSIZE,recv_len - FSP_HSIZE);
	return 0;
}

/* ****************** packet sending functions ************** */

/* make one send + receive transaction with server */
/* outgoing packet is in p, incomming in rpkt */
int fsp_transaction(FSP_SESSION *s,FSP_PKT *p,FSP_PKT *rpkt)
{
	char buf[FSP_MAXPACKET];
	size_t l;
	ssize_t r;
	fd_set mask;
	struct timeval start[8],stop;
	int i;
	unsigned int retry,dupes;
	int w_delay; /* how long to wait on next packet */
	int f_delay; /* how long to wait after first send */
	int l_delay; /* last delay */
	unsigned int t_delay; /* time from first send */

	memset(buf,0,sizeof(buf));

	if(p == rpkt)
	{
		errno = EINVAL;
		printf("EINVAL\n");
		return -2;
	}
	FD_ZERO(&mask);
	/* get the next key */
	p->key = client_get_key((FSP_LOCK *)s->lock);

	retry = random() & 0xfff8;
	if (s->seq == retry)
		s->seq ^= 0x1080;
	else
		s->seq = retry;
	dupes = retry = 0;
	t_delay = 0;
	/* compute initial delay here */
	/* we are using hardcoded value for now */
	f_delay = g_first_resend_time;//changed by xxfan
	l_delay = 0;
	for(;;retry++)
	{
		//if(retry!=0) printf("retry-%d\n",retry);
		if(t_delay >= s->timeout)
		{
			client_set_key((FSP_LOCK *)s->lock,p->key);
			errno = ETIMEDOUT;
			//printf("ETIMEOUT\n");
			//need error code//xxfan
			return -3;
		}
		/* make a packet */
		p->seq = (s->seq) | (retry & 0x7);
		l=fsp_pkt_write(p,buf);

		/* We should compute next delay wait time here */
		gettimeofday(&start[retry & 0x7],NULL);
		if(retry == 0 )
			w_delay=f_delay;
		else
		{
			w_delay=l_delay*3/2;
		}

		l_delay=w_delay;
		/* send packet */
		if( send(s->fd,buf,l,0) < 0 )
		{
#ifdef MAINTAINER_MODE
			printf("Send failed.\n");
#endif
			if(errno == EBADF || errno == ENOTSOCK)
			{
				client_set_key((FSP_LOCK *)s->lock,p->key);
				errno = EBADF;
				//printf("EBADF\n");
				return -1;
			}
			/* io terror */
			sleep(1);
			/* avoid wasting retry slot */
			retry--;
			t_delay += 1000;
			continue;
		}
		//printf("resend\n");

		/* keep delay value within sane limits */
		if (w_delay > (int) s->maxdelay)
			w_delay=s->maxdelay;
		else
			if(w_delay < 1000 )
				w_delay = 1000;

		t_delay += w_delay;
		/* receive loop */
		while(1)
		{
			if(w_delay <= 0 ) break;
			/* convert w_delay to timeval */
			stop.tv_sec=w_delay/1000;
			stop.tv_usec=(w_delay % 1000)*1000;
			FD_SET(s->fd,&mask);
			i=select(s->fd+1,&mask,NULL,NULL,&stop);
			if(i==0)
				break; /* timed out */
			if(i<0)
			{
				if(errno==EINTR)
				{
					/* lower w_delay */
					gettimeofday(&stop,NULL);
					w_delay-=1000*(stop.tv_sec -  start[retry & 0x7].tv_sec);
					w_delay-=     (stop.tv_usec -  start[retry & 0x7].tv_usec)/1000;
					continue;
				}
				/* hard select error */
				client_set_key((FSP_LOCK *)s->lock,p->key);
				return -1;
			}
			r=recv(s->fd,buf,FSP_MAXPACKET,0);
			if(r < 0 )
			{
				/* serious recv error */
				client_set_key((FSP_LOCK *)s->lock,p->key);
				printf("RECV Error\n");
				return -1;
			}

			gettimeofday(&stop,NULL);
			w_delay-=1000*(stop.tv_sec -  start[retry & 0x7].tv_sec);
			w_delay-=     (stop.tv_usec -  start[retry & 0x7].tv_usec)/1000;

			/* process received packet */
			if ( fsp_pkt_read(rpkt,buf,r) < 0)
			{
				/* unpack failed */
				//printf("unpack error\n");
				continue;
			}

			/* check sequence number */
			if( (rpkt->seq & 0xfff8) != s->seq )
			{
				//#ifdef MAINTAINER_MODE
				printf("dupe\n");
				//#endif
				/* duplicate */
				dupes++;
				continue;
			}

			/* check command code */
			if( (rpkt->cmd != p->cmd) && (rpkt->cmd != FSP_CC_ERR))
			{
				dupes++;
				continue;
			}

			/* check correct filepos */
			if( (rpkt->pos != p->pos) && ( p->cmd == FSP_CC_GET_DIR ||
						p->cmd == FSP_CC_GET_FILE || p->cmd == FSP_CC_UP_LOAD ||
						p->cmd == FSP_CC_GRAB_FILE || p->cmd == FSP_CC_INFO) )
			{
				dupes++;
				continue;
			}

			//Add by xxfan 2016-6-27
			if(rpkt->cmd == FSP_CC_ERR)
			{
				printf("\nFailed,code=%d,reason=\"%s\"\n",FSP_CC_ERR,rpkt->buf);
			}
			/* now we have a correct packet */

			/* compute rtt delay */
			w_delay=1000*(stop.tv_sec - start[retry & 0x7].tv_sec);
			w_delay+=(stop.tv_usec -  start[retry & 0x7].tv_usec)/1000;
			/* update last stats */
			s->last_rtt=w_delay;
			s->last_delay=f_delay;
			s->last_dupes=dupes;
			s->last_resends=retry;
			/* update cumul. stats */
			s->dupes+=dupes;
			s->resends+=retry;
			s->trips++;
			s->rtts+=w_delay;

			/* grab a next key */
			client_set_key((FSP_LOCK *)s->lock,rpkt->key);
			errno = 0;
			//Add by xxfan
			/*
			g_transfer_size_count+=g_preferred_size;
			if(retry==0) g_no_retry_count++;
			else
			{
			        g_retry_count+=retry;
				else if(retry==1)
				{
					printf("lost one packet,try again,size-%d\n",g_preferred_size);
				}
			else if(retry>=2)
			{
				g_no_retry_count=0;
				if(g_preferred_size-500>=g_min_packet_size)
				{
					g_preferred_size-=500;
					printf("g_preferred_size--,%d\n",g_preferred_size);
				}
				g_last_preferred_size=g_preferred_size;
			}
			if(g_no_retry_count%100==0)
			{
				if(g_preferred_size+50<g_preferred_size)
				{
					g_preferred_size+=50;
					printf("g_preferred_size++,%d\n",g_preferred_size);
				}
				g_no_retry_count=0;
			}
			}
			if(g_transfer_times==100)
			{
			  g_cur_transfer_speed=g_transfer_size_count/g_transfer_use_time;
			  if(g_cur_transfer_speed<g_last_transfer_speed)
			  {
			  }
			  else g_preferred_size=g_last_preferred_size;
			}*/
			//?end
			return 0;
		}
	}
}

/* ******************* Session management functions ************ */

/* initializes a session */

FSP_SESSION * fsp_open_session(const char* tid,const char* invite_code ,const char* key, const char *password)
{
	FSP_SESSION *s;
	FSP_LOCK *lock;

	//p2pnat add by xxfan 2016-03-11
	int rc;
	int peer_fd;
	int status;
	char server_addr[60];
	char cid[41],ped[25];
	char r_ed[25]={0}, r_ped[25]={0};
	char udp_message[1024];
	REND_EPT_HANDLE p2p_endpoint = NULL;
	REND_CONN_HANDLE p2p_conn = NULL;
	struct sockaddr_in peer_addr;

	peer_fd = new_udp_socket(0,NULL);
	p2p_endpoint = new_rendezvous_endpoint(NULL,"FSP",NULL,NULL,key,peer_fd);

	//
	time_t do_p2p_reg_time;
	time_t now;

	if(p2p_endpoint == NULL)
	{
		printf("Failed,code=%d,reason=\"create new p2p endpoint fail\"\n",P2P_NEW_ENDPOINT_FAILED);
		close(peer_fd);
		return NULL;
	}

	rendezvous_endpoint_reg(p2p_endpoint);

	time(&do_p2p_reg_time);

	while(1)
	{
		//check if reg timeout
		time(&now);
		if(now-do_p2p_reg_time>=60)//60s timeout
		{
			printf("Failed,code=%d,reason=\"p2p endpoint register timeout\"\n",P2P_ENDPOINT_REGISTER_FAILED);
			return NULL;
		}

		rendezvous_status_handle();
		if(get_rendezvous_endpoint(p2p_endpoint,&status,cid,NULL,ped) == 0)
		{
			if( status == ENDPOINT_REGISTER_OK && p2p_conn == NULL)
			{
				p2p_conn = new_rendezvous_connection(p2p_endpoint, tid, "FSP", "default" ,invite_code);
				if(p2p_conn != NULL)
				{
					//printf("p2p conn create successful\n");
					continue;
				}
			}
			else if(status == ENDPOINT_REGISTER_FAIL )
			{
				printf("Failed,code=%d,reason=\"p2p endpoint register failed\"\n",P2P_ENDPOINT_REGISTER_FAILED);
				return NULL;
			}
		}
		if(get_rendezvous_connection(p2p_conn, &status, r_ed, r_ped) == 0){
			if(status == CONNECTION_OK) {
				//如果连接建立成功，则可以发送应用数据
				//private net address
				if(strlen(r_ed) > 0){
					//printf("send data to %s\n", r_ed);
					strcpy(server_addr, r_ed);
				}
				//public net address
				else if(strlen(r_ped) > 0){
					//printf("send data to %s\n", r_ped);
					strcpy(server_addr, r_ped);
				}

				break;
			}
			else if(status == CONNECTION_FAILED)
			{
				printf("Failed,code=%d,reason=\"p2p connection error\"\n",P2P_CONNECTION_FAILED);
				return NULL;
			}
		}
		memset(udp_message, 0, sizeof(udp_message));
		rc = recv_udp_packet(peer_fd, udp_message, sizeof(udp_message), &peer_addr);
		if(rc <= 0){
			continue;
		}

		//- by xxfan 2016-06-4
		//rc = decode_rendezvous_packet(udp_message, &peer_addr, &resp_packet);
		//+
		rc = handle_rendezvous_packet(peer_fd,udp_message,&peer_addr);


		if(rc == 0){
			//- by xxfan 2016-06-04
			//rendezvous_message_handle(peer_fd,&resp_packet);
			continue;
		}
	}

	rc = endpoint_to_address(server_addr, &peer_addr);
	if(rc != 0)
	{
		//printf("Failed,reson=\"invalid ip port:%s\"\n", server_addr);
		return NULL;
	}
	//?end p2pnat

	if( connect(peer_fd, &peer_addr, sizeof(struct sockaddr_in)))
	{
		printf("Failed,code=%d,reason=\"p2p connection failed\"\n",P2P_CONNECTION_FAILED);
		return NULL;
	}

	s=calloc(1,sizeof(FSP_SESSION));
	if ( !s )
	{
		close(peer_fd);
		errno = ENOMEM;
		return NULL;
	}

	lock=malloc(sizeof(FSP_LOCK));

	if ( !lock )
	{
		close(peer_fd);
		free(s);
		errno = ENOMEM;
		return NULL;
	}

	s->lock=lock;

	/* init locking subsystem */
	if ( client_init_key( (FSP_LOCK *)s->lock,peer_addr.sin_addr.s_addr,ntohs(peer_addr.sin_port)))
	{
		free(s);
		close(peer_fd);
		free(lock);
		return NULL;
	}

	s->fd=peer_fd;
	s->timeout=300000; /* 5 minutes */
	s->maxdelay=60000; /* 1 minute  */
	s->seq=random() & 0xfff8;
	s->p2p_endpoint=p2p_endpoint;
	if ( password )
		s->password = strdup(password);
	return s;

}

/* closes a session */
void fsp_close_session(FSP_SESSION *s)
{
	FSP_PKT bye,in;

	if( s == NULL)
		return;
	if ( s->fd == -1)
		return;
	/* Send bye packet */
	bye.cmd=FSP_CC_BYE;
	bye.len=bye.xlen=0;
	bye.pos=0;
	s->timeout=7000;
	fsp_transaction(s,&bye,&in);

	close(s->fd);
	if (s->password) free(s->password);
	client_destroy_key((FSP_LOCK *)s->lock);
	free(s->lock);
	memset(s,0,sizeof(FSP_SESSION));
	s->fd=-1;
	free(s);
}

/* *************** Directory listing functions *************** */

/* get a directory listing from a server */
FSP_DIR * fsp_opendir(FSP_SESSION *s,const char *dirname)
{
	FSP_PKT in,out;
	int pos;
	unsigned short blocksize;
	FSP_DIR *dir;
	unsigned char *tmp;

	if (s == NULL) return NULL;
	if (dirname == NULL) return NULL;

	if(buildfilename(s,&out,dirname))
	{
		return NULL;
	}

	pos=0;
	blocksize=0;
	dir=NULL;
	out.cmd = FSP_CC_GET_DIR;

	//out.xlen=0;
	//Add by xxfan 2016/08/25
	//*(uint32_t*)(out.buf+out.len)= htons((uint32_t)g_preferred_size);
	*(uint32_t*)(out.buf+out.len)= htons((uint32_t)768);
	out.xlen=2;

	/* load directory listing from the server */
	while(1)
	{
		out.pos=pos;
		if ( fsp_transaction(s,&out,&in) )
		{
			pos = -1;
		}
		if ( in.cmd == FSP_CC_ERR )
		{
			/* bad reply from the server */
			//	printf("bad reply from the server\n");
			pos = -1;
			break;
		}
		/* End of directory? */
		if ( in.len == 0)
		{
			//printf("in.len=0\n");
			break;
		}
		/* set blocksize */
		if (blocksize == 0 )
		{
			blocksize = in.len;
			//printf("in.len-%d\n",blocksize);
		}

		/* alloc directory */
		if (dir == NULL)
		{
			dir = calloc(1,sizeof(FSP_DIR));
			if (dir == NULL)
			{
				pos = -1;
				break;
			}
		}
		//printf("pos-%d\n",pos);
		if (pos == -1)
		{
			/* failure */
			if (dir)
			{
				if(dir->data)
					free(dir->data);
				free(dir);
			}
			errno = EPERM;
			return NULL;
		}


		/* append data */
		tmp=realloc(dir->data,pos+in.len);
		if(tmp == NULL)
		{
			pos = -1;
			break;
		}
		dir->data=tmp;
		memcpy(dir->data + pos, in.buf,in.len);
		pos += in.len;
		if (in.len < blocksize)
		{
			//printf("in.len-%d < blocksize-%d ,break\n",in.len,blocksize);
			/* last block is smaller */
			break;
		}
	}

	if(dir!=NULL)
	{
		dir->inuse=1;
		dir->blocksize=blocksize;
		dir->dirname=strdup(dirname);
		dir->datasize=pos;
	}
	errno = 0;
	return dir;
}

int fsp_readdir_r(FSP_DIR *dir,struct dirent *entry, struct dirent **result)
{
	FSP_RDENTRY fentry,*fresult;
	int rc;
	char *c;

	//- by xxfan 2016-06-13
	//if (dir == NULL || entry == NULL || *result == NULL)
	//+
	if (dir == NULL || entry == NULL )//?end +
	{
		return -EINVAL;
	}
	if (dir->dirpos<0 || dir->dirpos % 4)
	{
		return -ESPIPE;
	}

	rc=fsp_readdir_native(dir,&fentry,&fresult);

	if (rc != 0)
		return rc;

#ifdef HAVE_DIRENT_TYPE
	/* convert FSP dirent to OS dirent */

	if (fentry.type == FSP_RDTYPE_DIR )
		entry->d_type=DT_DIR;
	else
		entry->d_type=DT_REG;
#endif

	/* remove symlink destination */
	c=strchr(fentry.name,'\n');
	if (c)
	{
		*c='\0';
		rc=fentry.namlen-strlen(fentry.name);
		fentry.reclen-=rc;
		fentry.namlen-=rc;
	}

#ifdef HAVE_DIRENT_FILENO
	entry->d_fileno = 10;
#endif
	entry->d_reclen = fentry.reclen;
	strncpy(entry->d_name,fentry.name,MAXNAMLEN);

	if (fentry.namlen >= MAXNAMLEN)
	{
		entry->d_name[MAXNAMLEN] = '\0';
#ifdef HAVE_DIRENT_NAMLEN
		entry->d_namlen = MAXNAMLEN;
	}
	else
	{
		entry->d_namlen = fentry.namlen;
#endif
	}

	if (fresult == &fentry )
	{
		*result = entry;
	}
	else
		*result = NULL;

	return 0;
}

/* native FSP directory reader */
int fsp_readdir_native(FSP_DIR *dir,FSP_RDENTRY *entry, FSP_RDENTRY **result)
{
	unsigned char ftype;
	int namelen;
	g_tmp_num++;
	int old_dir_pos;
	if (dir == NULL || entry == NULL || result == NULL)
	{
		//printf("dir ==NULL,entry==NULL,result==NULL\n");
		return -EINVAL;
	}
	if (dir->dirpos<0 || dir->dirpos % 4)
	{
		//printf("dir->dirpos-%d,error\n",dir->dirpos);
		return -ESPIPE;
	}
	//printf("dir_pos-%d\n",dir->dirpos);
	old_dir_pos=dir->dirpos;
	//printf("dir->blocksize-%d\n",dir->blocksize);
	while(1)
	{
		if ( dir->dirpos >= (int)dir->datasize )
		{
			/* end of the directory */
			*result = NULL;
			//printf("dir->dirp-%d > = dir -> datasize-%d \n",dir->dirpos,dir->datasize);
			return 0;
		}
		if (dir->blocksize - (dir->dirpos % dir->blocksize) < 9)
		{
			//printf("file type FSP_RDTYPE_SKIP\n");
			ftype= FSP_RDTYPE_SKIP;
		}
		else
			/* get the file type */
			ftype=dir->data[dir->dirpos+8];

		if (ftype == FSP_RDTYPE_END )
		{
			//printf("dir->dirpos-%d,dir->datasize-%d\n",dir->dirpos,dir->datasize);
			dir->dirpos=dir->datasize;
			continue;
		}
		if (ftype == FSP_RDTYPE_SKIP )
		{
			/* skip to next directory block */
			dir->dirpos = ( dir->dirpos / dir->blocksize + 1 ) * dir->blocksize;
			//#ifdef MAINTAINER_MODE
			//dir->dirpos +=12;
			//#ifdef MAINTAINER_MODE
			//#endif
			continue;
		}
		/* extract binary data */
		entry->lastmod=ntohl( *(const uint32_t *)( dir->data+ dir->dirpos ));
		//printf("entry->time-%s\n",ctime(&(entry->lastmod)));
		entry->size=ntohl( *(const uint32_t *)(dir->data+ dir->dirpos +4 ));
		//printf("entry->size-%d\n",entry->size);
		entry->type=ftype;
		/* if(ftype==1)
		   {
		   printf("entry->type-file\n");
		   }
		   else if(ftype==2)
		   {
		   printf("entry-type-dir\n");
		   }

		   printf("entry->type-%d\n",(int)(entry->type));
		 */

		/* skip file date and file size */
		dir->dirpos += 9;
		/* read file name */
		entry->name[255] = '\0';
		strncpy(entry->name,(char *)( dir->data + dir->dirpos ),255);
		//printf("entry->name-%s\n",entry->name);
		/* check for ASCIIZ encoded filename */
		if (memchr(dir->data + dir->dirpos,0,dir->datasize - dir->dirpos) != NULL)
		{
			namelen = strlen( (char *) dir->data+dir->dirpos);
		}
		else
		{
			/* \0 terminator not found at end of filename */
			printf("\0 terminator not found\n");
			*result = NULL;
			return 0;
		}
		/* skip over file name */
		dir->dirpos += namelen +1;

		/* set entry namelen field */
		if (namelen > 255)
			entry->namlen = 255;
		else
			entry->namlen = namelen;
		/* set record length */
		entry->reclen = 10+namelen;

		/* pad to 4 byte boundary */
		/* Del by xxfan
		   while( dir->dirpos & 0x3 )
		   {
		   dir->dirpos++;
		   entry->reclen++;
		   }*/
		int n;
		//printf("entry->reclen=%d,dirpos-%d\n",entry->reclen);
		if(entry->reclen%4!=0){

			n=4-(entry->reclen)%4;
			entry->reclen+=n;
			dir->dirpos+=n;
		}

		//printf("old_dir_pos-%d,new_dirpos-%d, len=%d\n",old_dir_pos,dir->dirpos,dir->dirpos-old_dir_pos);
		/* and return it */
		*result=entry;
		return 0;
	}
}

struct dirent * fsp_readdir(FSP_DIR *dirp)
{
	static dirent_workaround entry;
	struct dirent *result;


	if (dirp == NULL) return NULL;
	if ( fsp_readdir_r(dirp,&entry.dirent,&result) )
	{
		// printf("NULL\n");
		return NULL;
	}
	else
	{
		return result;
	}
}

long fsp_telldir(FSP_DIR *dirp)
{
	return dirp->dirpos;
}

void fsp_seekdir(FSP_DIR *dirp, long loc)
{
	dirp->dirpos=loc;
}

void fsp_rewinddir(FSP_DIR *dirp)
{
	dirp->dirpos=0;
}

int fsp_closedir(FSP_DIR *dirp)
{
	if (dirp == NULL)
		return -1;
	if(dirp->dirname) free(dirp->dirname);
	free(dirp->data);
	free(dirp);
	return 0;
}

/*  ***************** File input/output functions *********  */
FSP_FILE * fsp_fopen(FSP_SESSION *session, const char *path,const char *modeflags)
{
	FSP_FILE   *f;

	if(session == NULL || path == NULL || modeflags == NULL)
	{
		errno = EINVAL;
		return NULL;
	}

	f=calloc(1,sizeof(FSP_FILE));
	if (f == NULL)
	{
		return NULL;
	}

	/* check and parse flags */
	switch (*modeflags++)
	{
		case 'r':
			break;
		case 'w':
			f->writing=1;
			break;
		case 'a':
			/* not supported */
			free(f);
			errno = ENOTSUP;
			return NULL;
		default:
			free(f);
			errno = EINVAL;
			return NULL;
	}

	if (*modeflags == '+' || ( *modeflags=='b' && modeflags[1]=='+'))
	{
		free(f);
		errno = ENOTSUP;
		return NULL;
	}

	f->out.xlen=0;//add by xxfan
	/* build request packet */
	if(f->writing)
	{
		f->out.cmd=FSP_CC_UP_LOAD;
	}
	else
	{
		if(buildfilename(session,&f->out,path))
		{
			free(f);
			return NULL;
		}
		f->bufpos=FSP_SPACE;
		f->out.cmd=FSP_CC_GET_FILE;
		//Add by xxfan 2016/08/16
		*(int32_t*)(f->out.buf+f->out.len)= htonl((int32_t)g_preferred_size);
		f->out.xlen=2;
		f->out.len+=2;
		//?end add
	}
	// f->out.xlen=0; //delete by xxfan

	/* setup control variables */
	f->s=session;
	f->name=strdup(path);
	if(f->name == NULL)
	{
		free(f);
		errno = ENOMEM;
		return NULL;
	}

	return f;
}

size_t fsp_fread(void *dest,size_t size,size_t count,FSP_FILE *file)
{
	size_t total,done,havebytes;
	char *ptr;
	int rc=0;

	total=count*size;
	done=0;
	ptr=dest;


	if(file->eof) return 0;
	while(1)
	{

		/* need more data? */
		if(file->bufpos>=FSP_SPACE)
		{
			/* fill the buffer */
			file->out.pos=file->pos;
			if((rc=fsp_transaction(file->s,&file->out,&file->in))!=0)
			{
				if(rc==-3)//timeout
					file->err=3;
				else file->err=1;
				return done/size;
			}
			if(file->in.cmd == FSP_CC_ERR)
			{
				errno = EIO;
				file->err=2;
				return done/size;
			}
			file->bufpos=FSP_SPACE-file->in.len;
			if(file->bufpos > 0)
			{
				memmove(file->in.buf+file->bufpos,file->in.buf,file->in.len);
			}
			file->pos+=file->in.len;
		}
		havebytes=FSP_SPACE - file->bufpos;
		if (havebytes == 0 )
		{
			/* end of file! */
			file->eof=1;
			errno = 0;
			return done/size;
		}
		/* copy ready data to output buffer */
		if(havebytes < total )
		{
			/* copy all we have */
			memcpy(ptr,file->in.buf+file->bufpos,havebytes);
			ptr+=havebytes;
			file->bufpos=FSP_SPACE;
			done+=havebytes;
			total-=havebytes;
		}
		else
		{
			/* copy bytes left */
			memcpy(ptr,file->in.buf+file->bufpos,total);
			file->bufpos+=total;
			errno = 0;
			return count;
		}
	}
}

size_t fsp_fwrite(const void * source, size_t size, size_t count, FSP_FILE * file)
{
	size_t total,done,freebytes;
	const char *ptr;

	if(file->eof || file->err)
		return 0;

	file->out.len=FSP_SPACE;
	total=count*size;
	done=0;
	ptr=source;

	while(1)
	{
		/* need to write some data? */
		if(file->bufpos>=FSP_SPACE)
		{
			/* fill the buffer */
			file->out.pos=file->pos;
			if(fsp_transaction(file->s,&file->out,&file->in))
			{
				file->err=1;
				return done/size;
			}
			if(file->in.cmd == FSP_CC_ERR)
			{
				errno = EIO;
				file->err=1;
				return done/size;
			}
			file->bufpos=0;
			file->pos+=file->out.len;
			done+=file->out.len;
		}
		freebytes=FSP_SPACE - file->bufpos;
		/* copy input data to output buffer */
		if(freebytes <= total )
		{
			/* copy all we have */
			memcpy(file->out.buf+file->bufpos,ptr,freebytes);
			ptr+=freebytes;
			file->bufpos=FSP_SPACE;
			total-=freebytes;
		} else
		{
			/* copy bytes left */
			memcpy(file->out.buf+file->bufpos,ptr,total);
			file->bufpos+=total;
			errno = 0;
			return count;
		}
	}
}

int fsp_fpurge(FSP_FILE *file)
{
	if(file->writing)
	{
		file->bufpos=0;
	}
	else
	{
		file->bufpos=FSP_SPACE;
	}
	errno = 0;
	return 0;
}

int fsp_fflush(FSP_FILE *file)
{
	if(file == NULL)
	{
		errno = ENOTSUP;
		return -1;
	}
	if(!file->writing)
	{
		errno = EBADF;
		return -1;
	}
	if(file->eof || file->bufpos==0)
	{
		errno = 0;
		return 0;
	}

	file->out.pos=file->pos;
	file->out.len=file->bufpos;
	if(fsp_transaction(file->s,&file->out,&file->in))
	{
		file->err=1;
		return -1;
	}
	if(file->in.cmd == FSP_CC_ERR)
	{
		errno = EIO;
		file->err=1;
		return -1;
	}
	file->bufpos=0;
	file->pos+=file->out.len;

	errno = 0;
	return 0;
}



int fsp_fclose(FSP_FILE *file)
{
	int rc;

	rc=0;
	errno = 0;
	if(file->writing)
	{
		if(fsp_fflush(file))
		{
			rc=-1;
		}
		else if(fsp_install(file->s,file->name,0))
		{
			rc=-1;
		}
	}
	free(file->name);
	free(file);
	return rc;
}

int fsp_fseek(FSP_FILE *stream, long offset, int whence)
{
	long newoffset;

	switch(whence)
	{
		case SEEK_SET:
			newoffset = offset;
			break;
		case SEEK_CUR:
			newoffset = stream->pos + offset;
			break;
		case SEEK_END:
			errno = ENOTSUP;
			return -1;
		default:
			errno = EINVAL;
			return -1;
	}
	if(stream->writing)
	{
		if(fsp_fflush(stream))
		{
			return -1;
		}
	}
	stream->pos=newoffset;
	stream->eof=0;
	fsp_fpurge(stream);
	return 0;
}

long fsp_ftell(FSP_FILE *f)
{
	return f->pos + f->bufpos;
}

void fsp_rewind(FSP_FILE *f)
{
	if(f->writing)
		fsp_fflush(f);
	f->pos=0;
	f->err=0;
	f->eof=0;
	fsp_fpurge(f);
}

/*  **************** Utility functions ****************** */

/* return 0 if user has enough privs for uploading the file */
int fsp_canupload(FSP_SESSION *s,const char *fname)
{
	char *dir;
	unsigned char dirpro;
	int rc;
	struct stat sb;

	dir=directoryfromfilename(fname);
	if(dir == NULL)
	{
		errno = ENOMEM;
		return -1;
	}

	rc=fsp_getpro(s,dir,&dirpro);
	free(dir);

	if(rc)
	{
		return -1;
	}

	if(dirpro & FSP_DIR_OWNER)
		return 0;

	if( ! (dirpro & FSP_DIR_ADD))
		return -1;

	if (dirpro & FSP_DIR_DEL)
		return 0;

	/* we need to check file existence, because we can not overwrite files */

	rc = fsp_stat(s,fname,&sb);

	if (rc == 0)
		return -1;
	else
		return 0;
}

/* install file opened for writing */
int fsp_install(FSP_SESSION *s,const char *fname,time_t timestamp)
{
	int rc;
	FSP_PKT in,out;

	/* and install a new file */
	out.cmd=FSP_CC_INSTALL;
	out.xlen=0;
	out.pos=0;
	rc=0;
	if( buildfilename(s,&out,fname) )
		rc=-1;
	else
	{
		if (timestamp != 0)
		{
			/* add timestamp extra data */
			*(uint32_t *)(out.buf+out.len)=htonl(timestamp);
			out.xlen=4;
			out.pos=4;
		}
		if(fsp_transaction(s,&out,&in))
		{
			rc=-1;
		} else
		{
			if(in.cmd == FSP_CC_ERR)
			{
				rc=-1;
				errno = EPERM;
			}
		}
	}

	return rc;
}
/* Get protection byte from the directory */
int fsp_getpro(FSP_SESSION *s,const char *directory,unsigned char *result)
{
	FSP_PKT in,out;

	if(buildfilename(s,&out,directory))
		return -1;
	out.cmd=FSP_CC_GET_PRO;
	out.xlen=0;
	out.pos=0;

	if(fsp_transaction(s,&out,&in))
		return -1;

	if(in.cmd == FSP_CC_ERR)
	{
		errno = ENOENT;
		return -1;
	}
	if(in.pos != FSP_PRO_BYTES)
	{
		errno = ENOMSG;
		return -1;
	}

	if(result)
		*result=in.buf[in.len];
	errno = 0;
	return  0;
}

int fsp_stat(FSP_SESSION *s,const char *path,struct stat *sb)
{
	FSP_PKT in,out;
	unsigned char ftype;

	if(buildfilename(s,&out,path))
		return -1;
	out.cmd=FSP_CC_STAT;
	out.xlen=0;
	out.pos=0;

	if(fsp_transaction(s,&out,&in))
		return -1;

	if(in.cmd == FSP_CC_ERR)
	{
		errno = ENOTSUP;
		return -1;
	}
	/* parse results */
	ftype=in.buf[8];
	if(ftype == 0)
	{
		errno = ENOENT;
		return -1;
	}
	sb->st_uid=sb->st_gid=0;
	sb->st_mtime=sb->st_ctime=sb->st_atime=ntohl( *(const uint32_t *)( in.buf ));
	sb->st_size=ntohl( *(const uint32_t *)(in.buf + 4 ));
	sb->st_blocks=(sb->st_size+511)/512;
	if (ftype==FSP_RDTYPE_DIR)
	{
		sb->st_mode=S_IFDIR | 0755;
		sb->st_nlink=2;
	}
	else
	{
		sb->st_mode=S_IFREG | 0644;
		sb->st_nlink=1;
	}

	errno = 0;
	return  0;
}

int fsp_mkdir(FSP_SESSION *s,const char *directory)
{
	return simplecommand(s,directory,FSP_CC_MAKE_DIR);
}

int fsp_rmdir(FSP_SESSION *s,const char *directory)
{
	return simplecommand(s,directory,FSP_CC_DEL_DIR);
}

int fsp_unlink(FSP_SESSION *s,const char *directory)
{
	return simplecommand(s,directory,FSP_CC_DEL_FILE);
}

int fsp_rename(FSP_SESSION *s,const char *from, const char *to)
{
	FSP_PKT in,out;
	int l;

	if(buildfilename(s,&out,from))
		return -1;
	/* append target name */
	l=strlen(to)+1;
	if( l + out.len > FSP_SPACE )
	{
		errno = ENAMETOOLONG;
		return -1;
	}
	memcpy(out.buf+out.len,to,l);
	out.xlen = l;

	if(s->password)
	{
		l=strlen(s->password)+1;
		if(out.len + out.xlen + l > FSP_SPACE)
		{
			errno = ENAMETOOLONG;
			return -1;
		}
		out.buf[out.len+out.xlen-1] = '\n';
		memcpy(out.buf+out.len+out.xlen,s->password,l);
		out.xlen += l;
	}

	out.cmd=FSP_CC_RENAME;
	out.pos=out.xlen;

	if(fsp_transaction(s,&out,&in))
		return -1;

	if(in.cmd == FSP_CC_ERR)
	{
		errno = EPERM;
		return -1;
	}

	errno = 0;
	return 0;
}

int fsp_access(FSP_SESSION *s,const char *path, int mode)
{
	struct stat sb;
	int rc;
	unsigned char dirpro;
	char *dir;

	rc=fsp_stat(s,path,&sb);
	if(rc == -1)
	{
		/* not found */
		/* errno is set by fsp_stat */
		return -1;
	}

	/* just test file existence */
	if(mode == F_OK)
	{
		errno = 0;
		return  0;
	}

	/* deny execute access to file */
	if (mode & X_OK)
	{
		if(S_ISREG(sb.st_mode))
		{
			errno = EACCES;
			return -1;
		}
	}

	/* Need to get ACL of directory */
	if(S_ISDIR(sb.st_mode))
		dir=NULL;
	else
		dir=directoryfromfilename(path);

	rc=fsp_getpro(s,dir==NULL?path:dir,&dirpro);
	/* get pro failure */
	if(rc)
	{
		if(dir) free(dir);
		errno = EACCES;
		return -1;
	}
	/* owner shortcut */
	if(dirpro & FSP_DIR_OWNER)
	{
		if(dir) free(dir);
		errno = 0;
		return 0;
	}
	/* check read access */
	if(mode & R_OK)
	{
		if(dir)
		{
			if(! (dirpro & FSP_DIR_GET))
			{
				free(dir);
				errno = EACCES;
				return -1;
			}
		} else
		{
			if(! (dirpro & FSP_DIR_LIST))
			{
				errno = EACCES;
				return -1;
			}
		}
	}
	/* check write access */
	if(mode & W_OK)
	{
		if(dir)
		{
			if( !(dirpro & FSP_DIR_DEL) || !(dirpro & FSP_DIR_ADD))
			{
				free(dir);
				errno = EACCES;
				return -1;
			}
		} else
		{
			/* when checking directory for write access we are cheating
			   by allowing ADD or DEL right */
			if( !(dirpro & FSP_DIR_DEL) && !(dirpro & FSP_DIR_ADD))
			{
				errno = EACCES;
				return -1;
			}
		}
	}

	if(dir) free(dir);
	errno = 0;
	return 0;
}
// Add by xxfan 2016-03-30
int fsp_ch_passwd(FSP_SESSION *s,const char *new_fsp_password)
{
	FSP_PKT pkt_in,pkt_out;
	FSP_PKT* out=&pkt_out;
	FSP_PKT* in=&pkt_in;

	//the password file is saved in  the root dir
	//date format like: dir name\nold passwd\nnew passwd
	int len=strlen("\n");
	memcpy(out->buf,"\n",len+1);
	out->len=len;

	if(s->password)
	{
		len=strlen(s->password);
		if(out->len+ len >= FSP_SPACE -1 )
		{
			errno = ENAMETOOLONG;
			return -1;
		}
		memcpy(out->buf+out->len,s->password,len+1);
		out->len+=len;
	}

	len=strlen("\n");

	if(out->len + len >= FSP_SPACE -1 )
	{
		errno = ENAMETOOLONG;
		return -1;
	}
	memcpy(out->buf+out->len,"\n",len+1);
	out->len+=len;

	if(new_fsp_password!=NULL)
	{
		len = strlen(new_fsp_password);
		if(out->len + len >= FSP_SPACE -1 )
		{
			errno = ENAMETOOLONG;
			return -1;

		}
		memcpy(out->buf+out->len,new_fsp_password,len+1);
		out->len+=len;
	}

	/* add terminating \0 */
	out->len++;

	out->cmd=FSP_CC_CH_PASSWD;
	out->xlen=0;
	out->pos=0;

	if(fsp_transaction(s,out,in))
		return -1;

	if(in->cmd == FSP_CC_ERR)
	{
		errno = EPERM;
		return -1;
	}

	errno = 0;
	return  0;
}


