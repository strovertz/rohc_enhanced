#include "decompressor.h"
#include <stdio.h>
#include <netinet/ip.h>
#include <string.h>
#include <rohc/rohc_buf.h>
#include <time.h>
#include <stdlib.h>
#include <rohc/rohc_comp.h>
#include <rohc/rohc_decomp.h>

#define BUFFER_SIZE 2048
#define FAKE_PAYLOAD "hello, ROHC world!"

void decompressor(struct rohc_buf rohc_packet, struct rohc_buf ip_packet){
    //! [define ROHC decompressor]
	struct rohc_decomp *decompressor;       /* the ROHC decompressor */
    //! [define ROHC decompressor]
    //! [define IP and ROHC packets]
	
	/* we do not want to handle feedback in this simple example */
	struct rohc_buf *rcvd_feedback = NULL;
	struct rohc_buf *feedback_send = NULL;
    //! [define IP and ROHC packets]
	printf("\n\n\n The Received Packet: \n");
	dump_packet(rohc_packet);
	printf("\n\nIP PACKET:\n");
	dump_packet(ip_packet);
	rohc_status_t status;

    //! [create ROHC decompressor #1]
	/* Create a ROHC decompressor to operate:
	 *  - with large CIDs,
	 *  - with the maximum of 5 streams (MAX_CID = 4),
	 *  - in Unidirectional mode (U-mode).
	 */
    //! [create ROHC decompressor #1]
	printf("\ncreate the ROHC decompressor\n");
    //! [create ROHC decompressor #2]
	decompressor = rohc_decomp_new2(ROHC_LARGE_CID, 4, ROHC_U_MODE);
	if(decompressor == NULL)
	{
		fprintf(stderr, "failed create the ROHC decompressor\n");
		goto error;
	}
    //! [create ROHC decompressor #2]

	/* Enable the decompression profiles you need */
	printf("\nenable several ROHC decompression profiles\n");
    //! [enable ROHC decompression profile]
	if(!rohc_decomp_enable_profile(decompressor, ROHC_PROFILE_UNCOMPRESSED))
	{
		fprintf(stderr, "failed to enable the Uncompressed profile\n");
		goto release_decompressor;
	}
	if(!rohc_decomp_enable_profile(decompressor, ROHC_PROFILE_IP))
	{
		fprintf(stderr, "failed to enable the IP-only profile\n");
		goto release_decompressor;
	}
    //! [enable ROHC decompression profile]
    //! [enable ROHC decompression profiles]
	if(!rohc_decomp_enable_profiles(decompressor, ROHC_PROFILE_UDP,
	                                ROHC_PROFILE_ESP, -1))
	{
		fprintf(stderr, "failed to enable the IP/UDP and IP/ESP profiles\n");
		goto release_decompressor;
	}
    //! [enable ROHC decompression profiles]


	/* Now, decompress this fake ROHC packet */
	printf("\ndecompress the fake ROHC packet\n");
    //! [decompress ROHC packet #1]
	status = rohc_decompress3(decompressor, rohc_packet, &ip_packet,
	                          rcvd_feedback, feedback_send);
    //! [decompress ROHC packet #1]
	printf("\n");
    //! [decompress ROHC packet #2]
	if(status == ROHC_STATUS_OK)
	{
		/* decompression is successful */
		if(!rohc_buf_is_empty(ip_packet))
		{
			/* ip_packet.len bytes of decompressed IP data available in
			 * ip_packet: dump the IP packet on the standard output */
			printf("IP packet resulting from the ROHC decompression:\n");
			dump_packet(ip_packet);
		}
		else
		{
			/* no IP packet was decompressed because of ROHC segmentation or
			 * feedback-only packet:
			 *  - the ROHC packet was a non-final segment, so at least another
			 *    ROHC segment is required to be able to decompress the full
			 *    ROHC packet
			 *  - the ROHC packet was a feedback-only packet, it contained only
			 *    feedback information, so there was nothing to decompress */
			printf("no IP packet decompressed");
		}
	}
	else
	{
		fprintf(stderr, "\nDecomp Status: %s - %d\n", rohc_strerror(status), status);
		/* failure: decompressor failed to decompress the ROHC packet */
		fprintf(stderr, "decompression of fake ROHC packet failed\n");
		goto release_decompressor;
	}


	/* Release the ROHC decompressor when you do not need it anymore */
	printf("\n\ndestroy the ROHC decompressor\n");
    //! [destroy ROHC decompressor]
	rohc_decomp_free(decompressor);
    //! [destroy ROHC decompressor]


	printf("\nThe program ended successfully.\n");

	exit(0);

    release_decompressor:
        rohc_decomp_free(decompressor);
    error:
        fprintf(stderr, "an error occurred during program execution, "
                "abort program\n");
	exit(1);
}

void dump_packet(const struct rohc_buf packet){
	size_t i;

	for(i = 0; i < packet.len; i++)
	{
		printf("0x%02x ", rohc_buf_byte_at(packet, i));
		if(i != 0 && ((i + 1) % 8) == 0)
		{
			printf("\n");
		}
	}
	if(i != 0 && ((i + 1) % 8) != 0) /* be sure to go to the line */
	{
		printf("\n");
	}
}