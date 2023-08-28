#include "user.h" // contains printf

int main() {
  printf(1,"Size of char: %d byte\n", sizeof(char));
  printf(1,"Size of short: %d bytes\n", sizeof(short));
  printf(1,"Size of int: %d bytes\n", sizeof(int));
  printf(1,"Size of long: %d bytes\n", sizeof(long));
  printf(1,"Size of long long: %d bytes\n", sizeof(long long));
  printf(1,"Size of float: %d bytes\n", sizeof(float));
  printf(1,"Size of double: %d bytes\n", sizeof(double));
  printf(1,"Size of long double: %d bytes\n", sizeof(long double));
  exit();
}
