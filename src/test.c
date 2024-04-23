#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/ether.h>
#include <netinet/ip.h>
#include <rohc/rohc_comp.h>
#include <time.h>
#include <rohc/rohc_buf.h>

#define BUF_SIZ		65535
#define SEND 0
#define RECV 1
#define BUFFER_SIZE 2048
#define BUFFER_SIZE_T 2048

#define ARPHDRSIZ 28
#define ETHHDRSIZ 14
#define IPHDRSIZ 20


#define FAKE_PAYLOAD "LEK LEK LKE, ALELEKLEK LEK LEK!"

unsigned long get_ip_saddr(char *if_name, int sockfd);
unsigned long get_netmask(char *if_name, int sockfd);
uint16_t ip_checksum(void *vdata, size_t length);
struct rohc_buf constructIpHeader(struct in_addr dst, char if_name[], int sockfd, int sizePayload, struct rohc_buf rohc_packet);
void recv_message(char if_name[], struct sockaddr_ll sk_addr, int sockfd);
void send_message2(unsigned char buff[BUF_SIZ], char interfaceName[]);
//void send_message(char if_name[], struct sockaddr_ll sk_addr, char hw_addr[], char payload[], int sockfd, int type, struct ifreq if_hwaddr, int sizePayload);

struct iphdr *ip_header;
uint8_t ip_buffer[BUFFER_SIZE];
uint8_t rohc_buffer[BUFFER_SIZE];
struct rohc_buf ip_packet = rohc_buf_init_empty(ip_buffer, BUFFER_SIZE);

static int gen_random_num(const struct rohc_comp *const comp, void *const user_context) {
    return rand();
}


unsigned long get_netmask(char *if_name, int sockfd){
	struct ifreq if_idx;
	memset(&if_idx,0,sizeof(struct ifreq));
	strncpy(if_idx.ifr_name,if_name,IFNAMSIZ-1);
	if((ioctl(sockfd,SIOCGIFNETMASK,&if_idx)) == -1)
		perror("ioctl():");
	return ((struct sockaddr_in *)&if_idx.ifr_netmask)->sin_addr.s_addr;
}

unsigned long get_ip_saddr(char *if_name, int sockfd){
	printf("retrieving source IP...\n");
	struct ifreq if_idx;
	memset(&if_idx,0,sizeof(struct ifreq));
	strncpy(if_idx.ifr_name, if_name, IFNAMSIZ-1);
	if(ioctl(sockfd, SIOCGIFADDR, &if_idx) < 0)
		perror("SIOCGIFADDR aqui");
	printf("source IP obtained.\n");
	return ((struct sockaddr_in *)&if_idx.ifr_addr)->sin_addr.s_addr;
}
/*
void send_message2(unsigned char buff[BUF_SIZ], char interfaceName[]){

	// connect to interface name
	struct ifreq if_hwaddr;
	memset(&if_hwaddr,0,sizeof(struct ifreq));
	strncpy(if_hwaddr.ifr_name, interfaceName, IFNAMSIZ-1);
	if(ioctl(sockfd, SIOCGIFHWADDR, &if_hwaddr) < 0){
		perror("SIOCGIFHWADDR");
	}

	// if netmask is different, send ARP request for my router IP first
	// once I have MAC of my router, send  dst MAC router, dest IP
	unsigned long netmask = get_netmask(interfaceName,sockfd);
	unsigned long my_ip = get_ip_saddr(interfaceName,sockfd);

	//printf("size payload: %ld\n", SIZEH); 
	printf("\nSockFD: %d, OOB: %d \n", sockfd, MSG_OOB);

	int byteSent = send(sockfd, buff, BUFFER_SIZE, 0);
	if (byteSent == -1) perror("Send() Error!! ");

	printf("%d bytes sent!\n", byteSent);
}*/


void send_message(char if_name[], struct sockaddr_ll sk_addr, char hw_addr[], char payload[], int sockfd, int type, struct ifreq if_hwaddr, int sizePayload){
	// build ethernet frame
	struct ether_header frame;
	memset(&frame,0,sizeof(struct ether_header));
	memcpy(frame.ether_dhost, hw_addr, 6);
	memcpy(frame.ether_shost, if_hwaddr.ifr_hwaddr.sa_data, 6);
	switch(type){
		case 1: // IP
			frame.ether_type = htons(ETH_P_IP);
			break;
		case 2: // ARP
			frame.ether_type = htons(ETHERTYPE_ARP);
			break;
		default:
			frame.ether_type = htons(ETH_P_IP);
			break;

	}

	struct ifreq if_idx;
	memset(&if_idx,0,sizeof(struct ifreq));
	strncpy(if_idx.ifr_name, if_name, IFNAMSIZ-1);
	if(ioctl(sockfd, SIOCGIFINDEX, &if_idx) < 0){
		perror("SIOCGIFINDEX");
	}

	// pack frame header
	unsigned char buff[BUF_SIZ];
	char *eth_header = (char *)&frame;
	memcpy(buff,eth_header,ETHHDRSIZ);
	memcpy(&buff[ETHHDRSIZ],payload,sizePayload);

	sk_addr.sll_ifindex = if_idx.ifr_ifindex;
	sk_addr.sll_halen = ETH_ALEN;
	printf("size payload: %d\n",sizePayload);
	int byteSent = sendto(sockfd, buff, ETHHDRSIZ+sizePayload, 0, (struct sockaddr*)&sk_addr, sizeof(struct sockaddr_ll));
	printf("%d bytes sent!\n", byteSent);
}


// ip_checksum provided by Adam Hahn
uint16_t ip_checksum(void *vdata, size_t length){
	printf("calculating checksum...\n");
	char *data=(char *)vdata;
	uint32_t acc=0xffff;
	
	for(size_t i = 0; i+1<length; i+=2){
		uint16_t word;
		memcpy(&word,data+i,2);
		acc += ntohs(word);
		if(acc > 0xffff){
			acc -= 0xffff;
		}
	}
	if(length & 1){
		uint16_t word = 0;
		memcpy(&word,data+length-1,1);
		acc += ntohs(word);
		if(acc > 0xffff){
			acc -= 0xffff;
		}
	}
	printf("checksum calculated.\n");
	return htons(~acc);
}

struct rohc_buf constructIpHeader(struct in_addr dst, char if_name[], int sockfd, int sizePayload, struct rohc_buf rohc_packet){
	printf("constructing IP header...\n");
	struct iphdr *ip_header;
	struct rohc_buf ip_packet = rohc_buf_init_empty(ip_buffer, BUFFER_SIZE);
	struct rohc_comp *compressor;
	rohc_status_t rohc_status;
	size_t i;

	srand(time(NULL));
        compressor = rohc_comp_new2(ROHC_SMALL_CID, ROHC_SMALL_CID_MAX, gen_random_num, NULL);
        if(compressor == NULL){
            fprintf(stderr, "failed create the ROHC compressor\n");
            exit(1);
        }

        printf("enable the ip-only compression profile\n");
        if(!rohc_comp_enable_profile(compressor, ROHC_PROFILE_IP)){
            fprintf(stderr, "failed to enable the ip-only profile\n");
            rohc_comp_free(compressor);
            exit(1);
        }

	/*
	struct iphdr ip_hdr;
	ip_hdr.ihl = 5;
	ip_hdr.version = 4;
	ip_hdr.tos = 0;
	ip_hdr.tot_len = htons(IPHDRSIZ+sizePayload);
	ip_hdr.id = 4;
	ip_hdr.frag_off = 0x0;
	ip_hdr.ttl = 0x40;
	ip_hdr.protocol = 6;
	ip_hdr.check = 0;
	ip_hdr.saddr = get_ip_saddr(if_name,sockfd);
	ip_hdr.daddr = dst.s_addr;
	*/

	ip_header =  (struct iphdr *)rohc_buf_data(ip_packet);
	ip_header->version = 4;
	ip_header->ihl = 5;
	ip_packet.len += ip_header->ihl * 4;
	ip_header->tos = 0;
	ip_header->tot_len = htons(ip_packet.len + strlen(FAKE_PAYLOAD));
	ip_header->id = 4;
	ip_header->frag_off = 0x0;
	ip_header->ttl = 0x40;
	ip_header->protocol = 0x04;
	ip_header->check = 0;
	ip_header->saddr = htonl(0x01020304);
    ip_header->daddr = htonl(0x05060708);
	/*ip_header->saddr = get_ip_saddr(if_name,sockfd);
	ip_header->daddr = dst.s_addr;*/
	ip_header->check = ip_checksum(ip_header, IPHDRSIZ);


	rohc_buf_append(&ip_packet, (uint8_t *)FAKE_PAYLOAD, strlen(FAKE_PAYLOAD));

	for(i = 0; i < ip_packet.len; i++)
	{
		printf("0x%02x ", rohc_buf_byte_at(ip_packet, i));
		if(i != 0 && ((i + 1) % 8) == 0)
		{
			printf("\n");
		}
	}
	if(i != 0 && (i % 8) != 0) /* be sure to go to the line */
	{
		printf("\n");
	}

	printf("\nCompressing the IP packet\n");
	rohc_status = rohc_compress4(compressor, ip_packet, &rohc_packet);
	if (rohc_status != ROHC_STATUS_OK) {
		fprintf(stderr, "Compression of fake packet failed: %s (%d)\n", rohc_strerror(rohc_status), rohc_status);
		rohc_comp_free(compressor);
		exit(1);
	}
	
	printf("Resulting ROHC packet after compression\n");
	for(i = 0; i < rohc_packet.len; i++)        {
		printf("0x%02x ", rohc_buf_byte_at(rohc_packet, i));
		if(i != 0 && ((i + 1) % 8) == 0)
		{
			printf("\n");
		}
	}
	if(i != 0 && (i % 8) != 0) /* be sure to go to the line */
	{
		printf("\n");
	}
	printf("IP header constructed...\n");	
	return rohc_packet; 
}


int main(int argc, char *argv[])
{
	int mode;
	char buff[BUF_SIZ];
	char interfaceName[IFNAMSIZ];
	memset(buff, 0, BUF_SIZ);
	struct in_addr dst_ip;
	struct in_addr router_ip;
	
	int correct=0;
	if (argc > 1){
		if(strncmp(argv[1],"Send", 4)==0){
			if (argc == 6){
				mode=SEND; 
				inet_aton(argv[3], &dst_ip);
				inet_aton(argv[4], &router_ip);
				strncpy(buff, argv[5], BUF_SIZ);
				printf("Sending payload: %s\n", buff);
				correct=1;
			}
		}
		else if(strncmp(argv[1],"Recv", 4)==0){
			if (argc == 3){
				mode=RECV;
				correct=1;
			}
		}
		strncpy(interfaceName, argv[2], IFNAMSIZ);
		printf("interface: %s\n",interfaceName);
	 }
	 if(!correct){
		fprintf(stderr, "./455_proj2 Send <InterfaceName>  <DestIP> <RouterIP> <Message>\n");
		fprintf(stderr, "./455_proj2 Recv <InterfaceName>\n");
		exit(1);
	 }

	struct sockaddr_ll sk_addr;
	memset(&sk_addr, 0, sizeof(struct sockaddr_ll));

	if(mode == SEND){

		int sockfd = -1;
		if((sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL)))<0){
			perror("socket() failed!");
		}
		// connect to interface name
		struct ifreq if_hwaddr;
		memset(&if_hwaddr,0,sizeof(struct ifreq));
		strncpy(if_hwaddr.ifr_name, interfaceName, IFNAMSIZ-1);
		int t = ioctl(sockfd, SIOCGIFHWADDR, &if_hwaddr); 
		if(t < 0){
			perror("SIOCGIFHWADDR");
		} else printf("\n\nIOCTL ID%d", t);
		printf("\n\nDebug: \n");
		char dst_mac[6];		
		printf("\n");
		int sk_addr_size = sizeof(struct sockaddr_ll);
		//unsigned long netmask = get_netmask(interfaceName,sockfd);
		//unsigned long my_ip = get_ip_saddr(interfaceName,sockfd);
		memset(&sk_addr,0,sk_addr_size);
		struct iphdr *ip_hdr;
		struct rohc_buf rohc_packet = rohc_buf_init_empty(rohc_buffer, BUFFER_SIZE);
		
		rohc_packet = constructIpHeader(dst_ip, interfaceName, sockfd, strlen(buff), rohc_packet);
		
		char ip_payload[IPHDRSIZ+strlen(buff)+1];
		char *ip = (char *)&rohc_packet;
		memcpy(ip_payload, ip, 8);
		//memcpy(&ip_payload[IPHDRSIZ], FA, strlen(buff));
		size_t i;
		printf("Resulting ROHC packet after compression\n");
		for(i = 0; i < rohc_packet.len; i++)        {
			printf("0x%02x ", rohc_buf_byte_at(rohc_packet, i));
			if(i != 0 && ((i + 1) % 8) == 0)
			{
				printf("\n");
			}
		}
		if(i != 0 && (i % 8) != 0) /* be sure to go to the line */
		{
			printf("\n");
		}
		send_message(interfaceName, sk_addr, dst_mac, ip_payload, sockfd, 1, if_hwaddr, IPHDRSIZ+strlen(buff));

		//send_message2(buff, interfaceName);

	}
	else if (mode == RECV){
		int sockfd = -1;
		if((sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL)))<0){
			perror("socket() failed!");
		}
		// wait for ARP request
    	}
    
	return 0;
}