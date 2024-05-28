#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main ()
{
  char b = "recv\t\t";
  ssize_t a = strlen(*b);
  printf("%d", a);
}