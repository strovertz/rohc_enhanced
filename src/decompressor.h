#ifndef DECOMPRESSOR_H
#define DECOMPRESSOR_H

#include <rohc/rohc_buf.h>

/* Function to create fake packets and compress them using ROHC */
void decompressor(struct rohc_buf rohc_packet, struct rohc_buf ip_packet);
void dump_packet(const struct rohc_buf packet);

#endif 