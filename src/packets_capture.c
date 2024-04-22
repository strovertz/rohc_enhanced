#include <pcap.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <errno.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h> 
#include <netinet/if_ether.h>
#include "packets_capture.h"


int capture(){
	char errbuf[PCAP_ERRBUF_SIZE];	
	int dev;
	struct pcap_if **pcap_if_t;
	dev = pcap_findalldevs(pcap_if_t, errbuf);
	if(dev == 0){
		printf("Error with capture interface\n");
		exit(0);
	}
	printf("Success");
	
	return 0;
}