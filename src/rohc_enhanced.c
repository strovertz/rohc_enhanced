#include <stdio.h>              /* for the printf() function */
#include "compressor.c"
#include "decompressor.c"
#include "packets_capture.c"

/* The main entry point of the program (arguments are not used) */
int main (int argc, char **argv){
  capture();
  printf("criando novos packets\n");
  create_fake_packets();
  return 0;
}