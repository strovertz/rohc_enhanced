# rohc_enhanced

Robust Header Compression Tests

### How To Use

#### Build With GCC:

```C
gcc -o rohc_hello_world -Wall \
  $(pkg-config rohc --cflags) \
  rohc_hello_world.c \
  $(pkg-config rohc --libs) 

```
