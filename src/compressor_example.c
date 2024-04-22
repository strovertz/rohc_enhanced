#include "compressor_example.h"
#include <stdio.h>
#include <netinet/ip.h>
#include <string.h>
#include <rohc/rohc_buf.h>
#include <time.h>
#include <stdlib.h>
#include <rohc/rohc_comp.h>

/* The size (in bytes) of the buffers used in the program */
#define BUFFER_SIZE 2048
#define FAKE_PAYLOAD "hello, ROHC world!"

static int gen_random_num(const struct rohc_comp *const comp, void *const user_context) {
    return rand();
}

void create_fake_packets() {
    struct iphdr *ip_header;
    struct rohc_comp *compressor;
    uint8_t ip_buffer[BUFFER_SIZE];
    uint8_t rohc_buffer[BUFFER_SIZE];
    
    rohc_status_t rohc_status;

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

    for (int i = 0; i < 10; i++) {
        struct rohc_buf ip_packet = rohc_buf_init_empty(ip_buffer, BUFFER_SIZE);
        struct rohc_buf rohc_packet = rohc_buf_init_empty(rohc_buffer, BUFFER_SIZE);
        printf("\nBuilding fake packet: %d\n", i);

        ip_header = (struct iphdr *)rohc_buf_data(ip_packet);
        ip_header->version = 4;
        ip_header->ihl = 5;
        ip_packet.len += ip_header->ihl * 4;
        ip_header->tos = 0;
        ip_header->tot_len = htons(ip_packet.len + strlen(FAKE_PAYLOAD));
        ip_header->id = 0;
        ip_header->frag_off = 0;
        ip_header->ttl = 1;
        ip_header->protocol = 134;
        ip_header->check = 0x3fa9;
        ip_header->saddr = htonl(0x01020304);
        ip_header->daddr = htonl(0x05060708);

        rohc_buf_append(&ip_packet, (uint8_t *)FAKE_PAYLOAD, strlen(FAKE_PAYLOAD));

        for (size_t j = 0; j < ip_packet.len; j++) {
            printf("0x%02x ", rohc_buf_byte_at(ip_packet, j));
            if (j != 0 && ((j + 1) % 8) == 0) {
                printf("\n");
            }
        }

        if (i != 0 && (i % 8) != 0) {
            printf("\n");
        }

        printf("\nCompressing the IP packet\n");
        rohc_status = rohc_compress4(compressor, ip_packet, &rohc_packet);
        if (rohc_status != ROHC_STATUS_OK) {
            fprintf(stderr, "Compression of fake packet failed: %s (%d)\n", rohc_strerror(rohc_status), rohc_status);
            rohc_comp_free(compressor);
            exit(1);
        }
        if (ip_packet.len > rohc_packet.len)
        {
            printf("Resulting ROHC packet after compression\n");
            for (size_t j = 0; j < rohc_packet.len; j++) {
                printf("0x%02x ", rohc_buf_byte_at(rohc_packet, j));
                if (j != 0 && ((j + 1) % 8) == 0) {
                    printf("\n");
                }
            }

            if (i != 0 && (i % 8) != 0) {
                printf("\n");
            }
        }
    }

    printf("Destroying the ROHC compressor\n");
    rohc_comp_free(compressor);
}
