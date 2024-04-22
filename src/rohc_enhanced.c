#include <stdio.h>              /* for the printf() function */
#include "compressor_example.c"
#include "decompressor.c"


/* The main entry point of the program (arguments are not used) */
int main (int argc, char **argv){
  printf("criando novos packets\n");
  create_fake_packets();
  return 0;
}
