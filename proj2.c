#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
  // Check count of arguments
  if (argc != 6)
  {
    fprintf(
        stderr,
        "Invalid count of arguments!\nUse: ./proj2 NZ NU TZ TU F\n");
    exit(1);
  }

  // Get values of arguments
  int NZ = atoi(argv[1]);
  int NU = atoi(argv[2]);
  int TZ = atoi(argv[3]);
  int TU = atoi(argv[4]);
  int F = atoi(argv[5]);

  // Check values of arguments
  if (NZ < 0 || NU < 0 || TZ < 0 || TU < 0 || F < 0 || TZ > 10000 || TU > 100 || F > 10000)
  {
    fprintf(
        stderr,
        "Invalid value of argument(s)!\nUse: 0 <= TZ <= 10000, 0 <= TU <= 100, 0 <= F <= 10000\n");
    exit(1);
  }

  return 0;
}
