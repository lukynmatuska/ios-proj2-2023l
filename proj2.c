#include <stdio.h>
#include <stdlib.h>
#include <time.h>     // for get_random_from_range()
#include <unistd.h>   // for POSIX types
#include <stdarg.h>   // for my_print()
#include <sys/mman.h> // for shared memory
#include <fcntl.h>    /* For O_* constants */
#include <sys/stat.h> /* For mode constants */
#include <semaphore.h>
#include <wait.h>

// wtf idk inspirace z interfernetu
#define MMAP(ptr)                                                                                  \
  {                                                                                                \
    (ptr) = mmap(NULL, sizeof(*(ptr)), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0); \
  }
#define UNMAP(ptr)                 \
  {                                \
    munmap((ptr), sizeof(*(ptr))); \
  }
// end of wtf idk inspirace z interfernetu

// Define (pointer) global variable for output file
FILE *output_file;
int *row_number;
sem_t *output;

// Intialize global parameters variables
int NZ = -1;
int NU = -1;
int TZ = -1;
int TU = -1;
int F = -1;

void parse_params(int argc, char *argv[])
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
  NZ = atoi(argv[1]);
  NU = atoi(argv[2]);
  TZ = atoi(argv[3]);
  TU = atoi(argv[4]);
  F = atoi(argv[5]);

  // Check values of arguments
  if (NZ < 0 || NU < 0 || TZ < 0 || TU < 0 || F < 0 || TZ > 10000 || TU > 100 || F > 10000)
  {
    fprintf(
        stderr,
        "Invalid value of argument(s)!\nUse: 0 <= TZ <= 10000, 0 <= TU <= 100, 0 <= F <= 10000\n");
    exit(1);
  }
}

useconds_t get_random_from_range(int min, int max)
{
  return min + (rand() % (max - min + 1));
}

void prepare_output()
{
  // Create shared memory
  MMAP(output);

  // Init semaphore
  sem_init(output, 1, 1);

  // Create shared memories
  MMAP(row_number);
  MMAP(output_file);

  // Open output file
  output_file = fopen("proj2.out", "w");
  if (output_file == NULL)
  {
    fprintf(stderr, "Cannot open file.");
    exit(1);
  }
}

void cleanup_output()
{
  // Close output file
  fclose(output_file);

  // Destroy shared memories
  UNMAP(output_file);
  UNMAP(row_number);

  // Destroy semaphore and their shared memory
  sem_destroy(output);
  UNMAP(output);
}

void my_print(const char *format, ...)
{
  sem_wait(output);

  // Get function params
  va_list args;
  va_start(args, format);

  // Print number of row
  fprintf(output_file, "%d: ", ++(*row_number));

  // Print the string from params
  vfprintf(output_file, format, args);

  // Flush buffer
  fflush(output_file);

  // Cleanup function params
  va_end(args);

  sem_post(output);
}

int main(int argc, char *argv[])
{
  parse_params(argc, argv);
  prepare_output();

  // Hlavní proces vytváří ihned po spuštění NZ procesů zákazníků a NU procesů úředníků

  // Čeká pomocí volání usleep náhodný čas v intervalu <F/2,F>
  usleep(get_random_from_range(F / 2, F));

  // Vypíše: A: closing
  my_print("closing\n");

  // Poté čeká na ukončení všech procesů, které aplikace vytváří.
  // Wait for children suicide
  while (wait(NULL) > 0)
    ;

  cleanup_output();

  // Jakmile jsou tyto procesy ukončeny, ukončí se i hlavní proces s kódem (exit code) 0.
  return 0;
}
