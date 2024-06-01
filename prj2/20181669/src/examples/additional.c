#include <stdio.h>
#include <string.h>
#include <syscall.h>
#include <stdlib.h>

/* Size of array to sort. */
#define SORT_SIZE 128

int
main (int argc, char *argv[]) 
{
  if(argc != 2 && argc != 5){
      printf ("usage: additional 10 20 62 40 or additional 10\n");
      return EXIT_FAILURE;
  }

  if(argc == 2) printf("%d\n", fibonacci(atoi(argv[1])));
  else{
    printf("%d\n", max_of_four_int(atoi(argv[1]), atoi(argv[2]), atoi(argv[3]), atoi(argv[4])));
  }

  return EXIT_SUCCESS;
}
