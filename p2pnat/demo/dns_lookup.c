#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

/* Structs */
struct resolv_header {
	int id;
	int qr,opcode,aa,tc,rd,ra,rcode;
	int qdcount;
	int ancount;
	int nscount;
	int arcount;
};

struct resolv_question {
	char * dotted;
	int qtype;
	int qclass;
};

struct resolv_answer {
	char * dotted;
	int atype;
	int aclass;
	int ttl;
	int rdlength;
	unsigned char * rdata;
	int rdoffset;
	char* buf;
	size_t buflen;
	size_t add_count;
};

enum etc_hosts_action {
    GET_HOSTS_BYNAME = 0,
    GETHOSTENT,
    GET_HOSTS_BYADDR,
};

#define T_A 1
#define C_IN 1
#define NAMESERVER_PORT 53
#define REPLY_TIMEOUT 3

#define PACKETSZ 512
#define MAXDNAME 256
#define HFIXEDSZ 12
#define RRFIXEDSZ 10

#define DPRINTF printf

static char p2p_dns_servers[3][100];

static int __length_dotted(const unsigned char *data, int offset);
static int __encode_dotted(const char *dotted, unsigned char *dest, int maxlen);
static int __decode_dotted(const unsigned char *data, int offset, char *dest, int maxlen);

static int __encode_header(struct resolv_header *h, unsigned char *dest, int maxlen)
{
	if (maxlen < HFIXEDSZ)
		return -1;

	dest[0] = (h->id & 0xff00) >> 8;
	dest[1] = (h->id & 0x00ff) >> 0;
	dest[2] = (h->qr ? 0x80 : 0) |
		((h->opcode & 0x0f) << 3) |
		(h->aa ? 0x04 : 0) |
		(h->tc ? 0x02 : 0) |
		(h->rd ? 0x01 : 0);
	dest[3] = (h->ra ? 0x80 : 0) | (h->rcode & 0x0f);
	dest[4] = (h->qdcount & 0xff00) >> 8;
	dest[5] = (h->qdcount & 0x00ff) >> 0;
	dest[6] = (h->ancount & 0xff00) >> 8;
	dest[7] = (h->ancount & 0x00ff) >> 0;
	dest[8] = (h->nscount & 0xff00) >> 8;
	dest[9] = (h->nscount & 0x00ff) >> 0;
	dest[10] = (h->arcount & 0xff00) >> 8;
	dest[11] = (h->arcount & 0x00ff) >> 0;

	return HFIXEDSZ;
}

static int __decode_header(unsigned char *data, struct resolv_header *h)
{
	h->id = (data[0] << 8) | data[1];
	h->qr = (data[2] & 0x80) ? 1 : 0;
	h->opcode = (data[2] >> 3) & 0x0f;
	h->aa = (data[2] & 0x04) ? 1 : 0;
	h->tc = (data[2] & 0x02) ? 1 : 0;
	h->rd = (data[2] & 0x01) ? 1 : 0;
	h->ra = (data[3] & 0x80) ? 1 : 0;
	h->rcode = data[3] & 0x0f;
	h->qdcount = (data[4] << 8) | data[5];
	h->ancount = (data[6] << 8) | data[7];
	h->nscount = (data[8] << 8) | data[9];
	h->arcount = (data[10] << 8) | data[11];

	return HFIXEDSZ;
}

static int __encode_question(struct resolv_question *q,
					unsigned char *dest, int maxlen)
{
	int i;

	i = __encode_dotted(q->dotted, dest, maxlen);
	if (i < 0)
		return i;

	dest += i;
	maxlen -= i;

	if (maxlen < 4)
		return -1;

	dest[0] = (q->qtype & 0xff00) >> 8;
	dest[1] = (q->qtype & 0x00ff) >> 0;
	dest[2] = (q->qclass & 0xff00) >> 8;
	dest[3] = (q->qclass & 0x00ff) >> 0;

	return i + 4;
}

#if 0
static int __decode_question(unsigned char *message, int offset,
					struct resolv_question *q)
{
	char temp[256];
	int i;

	i = __decode_dotted(message, offset, temp, sizeof(temp));
	if (i < 0)
		return i;

	offset += i;

	q->dotted = strdup(temp);
	q->qtype = (message[offset + 0] << 8) | message[offset + 1];
	q->qclass = (message[offset + 2] << 8) | message[offset + 3];

	return i + 4;
}
#endif

static int __length_question(unsigned char *message, int offset)
{
	int i;

	i = __length_dotted(message, offset);
	if (i < 0)
		return i;

	return i + 4;
}

static int __encode_dotted(const char *dotted, unsigned char *dest, int maxlen)
{
	int used = 0;

	while (dotted && *dotted) {
		char *c = strchr(dotted, '.');
		int l = c ? c - dotted : strlen(dotted);

		if (l >= (maxlen - used - 1))
			return -1;

		dest[used++] = l;
		memcpy(dest + used, dotted, l);
		used += l;

		if (c)
			dotted = c + 1;
		else
			break;
	}

	if (maxlen < 1)
		return -1;

	dest[used++] = 0;

	return used;
}

static int __decode_dotted(const unsigned char *data, int offset,
				  char *dest, int maxlen)
{
	int l;
	int measure = 1;
	int total = 0;
	int used = 0;

	if (!data)
		return -1;

	while ((l=data[offset++])) {
		if (measure)
		    total++;
		if ((l & 0xc0) == (0xc0)) {
			if (measure)
				total++;
			/* compressed item, redirect */
			offset = ((l & 0x3f) << 8) | data[offset];
			measure = 0;
			continue;
		}

		if ((used + l + 1) >= maxlen)
			return -1;

		memcpy(dest + used, data + offset, l);
		offset += l;
		used += l;
		if (measure)
			total += l;

		if (data[offset] != 0)
			dest[used++] = '.';
		else
			dest[used++] = '\0';
	}

	/* The null byte must be counted too */
	if (measure) {
	    total++;
	}

	// DPRINTF("Total decode len = %d\n", total);

	return total;
}

static int __length_dotted(const unsigned char *data, int offset)
{
	int orig_offset = offset;
	int l;

	if (!data)
		return -1;
	while ((l = data[offset++])) {

		if ((l & 0xc0) == (0xc0)) {
			offset++;
			break;
		}
		offset += l;
	}
	return offset - orig_offset;
}

static int __decode_answer(unsigned char *message, int offset,
				  struct resolv_answer *a)
{
	char temp[256];
	int i;

	i = __decode_dotted(message, offset, temp, sizeof(temp));
	if (i < 0)
		return i;

	message += offset + i;

	a->dotted = strdup(temp);
	a->atype = (message[0] << 8) | message[1];
	message += 2;
	a->aclass = (message[0] << 8) | message[1];
	message += 2;
	a->ttl = (message[0] << 24) |
		(message[1] << 16) | (message[2] << 8) | (message[3] << 0);
	message += 4;
	a->rdlength = (message[0] << 8) | message[1];
	message += 2;
	a->rdata = message;
	a->rdoffset = offset + i + RRFIXEDSZ;

	// DPRINTF("i=%d,rdlength=%d\n", i, a->rdlength);
	return i + RRFIXEDSZ + a->rdlength;
}

static char *rdata_to_ipstr(unsigned char *v, char *buf)
{
	sprintf(buf, "%d.%d.%d.%d", v[0], v[1], v[2], v[3]);
	return buf;
}

int p2p_demo_dns_lookup(char *name, char ip_str[3][25])
{
	static int local_id = 1;
	int i,j,len,fd,pos,rc, dns_idx;
	struct resolv_header h;
	struct resolv_question q;
	struct resolv_answer ma;
	unsigned char * packet = malloc(PACKETSZ);
	unsigned char * recv_packet = malloc(PACKETSZ);
	char *lookup = malloc(MAXDNAME);
	struct sockaddr_in sa;
	struct timeval tv;
	fd_set fds;
	int res_cnt = 0;

	if(recv_packet == NULL || packet == NULL || lookup == NULL){
		if (lookup)
			free(lookup);
		if (recv_packet)
			free(recv_packet);
		if (packet)
			free(packet);
		return -1;
	}
	memset(packet, 0, PACKETSZ);
	memset(&h, 0, sizeof(h));
	fd = -1;
	len = 0;

	local_id++;
	local_id &= 0xffff;
	h.id = local_id;
	h.qdcount = 1;
	h.rd = 1;
	dns_idx = 0;
	i = __encode_header(&h, packet, PACKETSZ);
	if (i < 0)
	{
		goto fail;
	}

	strncpy(lookup,name,MAXDNAME);
	q.dotted = (char *)lookup;
	q.qtype = T_A;
	q.qclass = C_IN;
	j = __encode_question(&q, packet+i, PACKETSZ-i);
	if (j < 0)
	{
		goto fail;
	}

	len = i + j;

try_next_srv:
	memset(recv_packet, 0, PACKETSZ);
	fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (fd < 0)
	{
		goto fail;
	}
	if(p2p_dns_servers[dns_idx] == NULL || strlen(p2p_dns_servers[dns_idx]) == 0){
		goto fail;
	}
	sa.sin_family = AF_INET;
	sa.sin_port = htons(NAMESERVER_PORT);
	sa.sin_addr.s_addr = inet_addr(p2p_dns_servers[dns_idx]);
	rc = connect(fd, (struct sockaddr *) &sa, sizeof(sa));
	if (rc < 0)
	{
		//if (errno == ENETUNREACH){
		//	printf("network not reach\n");
		//}
		goto fail;
	}
	// printf("Transmitting packet of length %d, id=%d, qr=%d\n",	len, h.id, h.qr);
	send(fd, packet, len, 0);
	FD_ZERO(&fds);
	FD_SET(fd, &fds);
	tv.tv_sec = REPLY_TIMEOUT;
	tv.tv_usec = 0;
	if (select(fd + 1, &fds, NULL, NULL, &tv) <= 0) {
	    // printf("Timeout\n");
		/* timed out, so retry send and receive,
		 * to next nameserver on queue */
		goto fail;
	}
	len = recv(fd, recv_packet, 512, 0);
	__decode_header(recv_packet, &h);
	// printf("id = %d, qr = %d\n", h.id, h.qr);
	if ((h.id != local_id) || (!h.qr)) {
		/* unsolicited */
		goto fail;
	}
	if ((h.rcode) || (h.ancount < 1)) {
		/* negative result, not present */
		goto fail;
	}

	pos = HFIXEDSZ;

	for (j = 0; j < h.qdcount; j++) {
		i = __length_question(recv_packet, pos);
		// printf("Skipping question, Length of question %d is %d\n", j, i);
		if (i < 0)
			goto fail;
		pos += i;
	}
	for (j=0;j<h.ancount;j++,pos += i)
	{
		if(res_cnt >= 3)
			break;
		i = __decode_answer(recv_packet, pos, &ma);
		if (i<0)
			goto fail;
		free(ma.dotted);
		if (ma.atype != T_A)
		{
			continue;
		}
		rdata_to_ipstr(ma.rdata, ip_str[res_cnt++]);
	}
	close(fd);
	free(packet);
	free(recv_packet);
	free(lookup);
	return res_cnt;
fail:
	if (fd != -1){
	    close(fd);
	    fd = -1;
	}
	dns_idx += 1;
	if(dns_idx < 3)
	{
		goto try_next_srv;
	}
	if (lookup)
	    free(lookup);
	if (recv_packet)
	    free(recv_packet);
	if (packet)
	    free(packet);
	return -1;
}

void p2p_demo_set_dns_servers(char *dns1, char *dns2, char *dns3) {
	if(dns1 != NULL)
		strcpy(p2p_dns_servers[0], dns1);
	if(dns2 != NULL)
		strcpy(p2p_dns_servers[1], dns2);
	if(dns3 != NULL)
		strcpy(p2p_dns_servers[2], dns3);
	return;
}

#if 0
int main(int argc, char *argv[])
{
	int rc;
	char buf[32] = {0};
	if(argc < 2)
	{
		printf("miss param\n");
		return 1;
	}
	p2p_set_dns_servers("192.168.20.81", NULL, "192.168.20.80");
	rc = p2p_dns_lookup(argv[1], buf);
	printf("rc %d, %s\n", rc, buf);
	return 0;
}
#endif
