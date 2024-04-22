#include <pcap/pcap.h>
#include <stdio.h> 
#include <stdlib.h> 
#include <errno.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h> 
#include <netinet/if_ether.h>
#include "packets_capture.h"



int capture(int argc,char **argv) 
{ 
    char errbuf[PCAP_ERRBUF_SIZE];
	pcap_create("any", errbuf);
	printf("ERROR %s\n", errbuf);
	return 0;	
}