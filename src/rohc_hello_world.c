#include <stdio.h>		/* for the printf() function */
#include <netinet/ip.h>		/* for the IPv4 header */
#include <string.h>		/* for strlen() */
#include <rohc/rohc_buf.h>	/* for the rohc_buf_*() functions */
#include <time.h>
#include <rohc/rohc_comp.h>

/* The size (in bytes) of the buffers used in the program */
#define BUFFER_SIZE 2048

/* The payload for the fake IP packet */
#define FAKE_PAYLOAD "hello, ROHC world!"

static int gen_random_num(const struct rohc_comp *const comp, void *const user_context) {
   return rand();
}

void initialize(){
    struct Vector *vector = malloc(sizeof(*v));
    if (!vector) {
        perror("malloc failed");
        exit(1);
    }
    vector->elements = NULL;
    vector->length = 0;
    return vector;
}

/* The main entry point of the program (arguments are not used) */
int main (int argc, char **argv){
  
  struct rohc_buf rohc_packet;
  typedef struct PacketList {
    struct rohc_buf packet;
    struct PacketList* next_packet;
  } Packet; 
  
  struct rohc_comp *compressor;
    /* the buffer that will contain ipv4 packet to compress */
  uint8_t ip_buffer[BUFFER_SIZE];
  /* o pacote que vai conter o ipv4 packet */
  struct rohc_buf ip_packet = rohc_buf_init_empty(ip_buffer, BUFFER_SIZE);
  struct iphdr *ip_header;

  /*struct pra guardar o resultado do pacote rohc*/
  uint8_t rohc_buffer[BUFFER_SIZE];
  struct rohc_buf rohc_packet = rohc_buf_init_empty(rohc_buffer, BUFFER_SIZE)
  rohc_status_t rohc_status;

  size_t i;
  /* print the purpose of the program on the console */
  printf ("This program will compress one single IPv4 packet\n");
  
  Packet *packets_head = initialize_vector;

  srand(time(NULL));

  /* criando um compressor ROHC com parametros padrao*/
  printf("create the ORHC compressor\n");
  compressor = rohc_comp_new2(ROHC_SMALL_CID, ROHC_SMALL_CID_MAX, gen_random_num, NULL);
  if(compressor == NULL){
      fprintf(stderr, "failed create the ROHC compressor\n");
      return 1;
  }

  printf("enable the ip-only compression profile\n");
  if(!rohc_comp_enable_profile(compressor, ROHC_PROFILE_IP)){
      fprintf(stderr, "failed to enable the ip-only profiel\n");
      rohc_comp_free(compressor);
      return 1;
  
  }

  /* cria um pacote ip falso para teste*/
  printf("buildando pacote fake: 0\n");
  ip_header = (struct iphdr *)rohc_buf_data(ip_packet);
  ip_header->version=4; 
  ip_header->ihl = 5; /*tamanho minimo de uma header ipv4*/
  ip_packet.len += ip_header->ihl * 4; 
  ip_header->tos = 0; /*trash for this example*/
  ip_header->tot_len = htons(ip_packet.len + strlen(FAKE_PAYLOAD)); /* htons converte little endian pra big endian  */ 
  ip_header->id = 0; /* id is not important now*/
  ip_header->frag_off = 0; /* sem fragmentacao de packets */
  ip_header->ttl = 1; /* ttl tbm nao e importante aq */
  ip_header->protocol = 134; /* numero do protocolo */
  ip_header->check = 0x3fa9; /* checksum  */
  ip_header->saddr = htonl(0x01020304); /*source ip 1.2.3.4*/
  ip_header->daddr = htonl(0x05060708); /* dest 5.6.7.8  */

  /* bota o payload logo apos o ip header*/
  rohc_buf_append(&ip_packet, (uint8_t *)FAKE_PAYLOAD, strlen(FAKE_PAYLOAD));
  /* dump o packet no terminal  */
  for(i = 0; i < ip_packet.len; i++){
      printf("0x%02x ", rohc_buf_byte_at(ip_packet, i));
    if(i != 0 && ((i + 1) % 8) == 0) {
      printf("\n");
    }
  }
  if(i != 0 && (i % 8) != 0){ /**/
    printf("\n");
  }
  
  /*comprimindo o pacote IP falso*/
  printf("compress the ip packet\n");
  rohc_status = rohc_compress4(compressor, ip_packet, &rohc_packet);
  if(rohc_status != ROHC_STATUS_OK){
    fprintf(stderr, "compression of fake Packet failed: %s (%d)\n", rohc_strerror(rohc_status), rohc_status);
    /* limpando o compressor*/
    rohc_comp_free(compressor);
    return 1;
  }

  /*jogando o pacote comprimido no terminal*/
  printf("ROHC packet resultante da compressao ROHC\n");
  for(i = 0; i < rohc_packet.len; i++){
    printf("0x%02x ", rohc_buf_byte_at(rohc_packet, i));
    if( i != 0 && ((i+1) % 8) == 0){
        printf("\n");
    }
  }

  if(i != 0 && (i % 8) != 0) {
      printf("\n");
  }

  /*libera o ROHC*/
  printf("destroy the ROHC compressor\n");
  rohc_comp_free(compressor);

  return 0;
}
