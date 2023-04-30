#include <stdio.h>
#include <stdlib.h>
#include <time.h>     // for get_random_from_range()
#include <unistd.h>   // for forking and POSIX types
#include <stdarg.h>   // for my_print()
#include <sys/mman.h> // for shared memory
#include <fcntl.h>    /* For O_* constants */
#include <sys/stat.h> /* For mode constants */
#include <semaphore.h>
#include <wait.h>
#include <stdbool.h>

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

static const unsigned int COUNT_OF_REQUEST_TYPES = 3;
static const unsigned int SLEEP_INTERVAL_MAX = 10;

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

// Declaration of shared variables
bool *post_office_open;

sem_t *shared_variables_mutex;

sem_t *letterCustomerQueue;
int *letterCustomerCount;

sem_t *packageCustomerQueue;
int *packageCustomerCount;

sem_t *moneyCustomerQueue;
int *moneyCustomerCount;

enum post_office_request
{
  letter = 1,
  package = 2,
  money = 3
};

void parse_params(int argc, char *argv[])
{
  // Check count of arguments
  if (argc != 6)
  {
    fprintf(
        stderr,
        "Invalid count of arguments!\nUse: ./proj2 NZ NU TZ TU F\n");
    exit(EXIT_FAILURE);
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
    exit(EXIT_FAILURE);
  }
}

useconds_t get_random_from_range(int min, int max)
{
  srand(time(0) * getpid());
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
    fprintf(stderr, "Cannot open file.\n");
    exit(EXIT_FAILURE);
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

void prepare_shared_variables()
{
  /* Post office status */
  MMAP(post_office_open);
  *post_office_open = true;
  /* End of post office status*/

  MMAP(shared_variables_mutex);
  sem_init(shared_variables_mutex, 1, 1);

  MMAP(letterCustomerQueue);
  sem_init(letterCustomerQueue, 1, 0);
  MMAP(letterCustomerCount);
  *letterCustomerCount = 0;

  MMAP(packageCustomerQueue);
  sem_init(packageCustomerQueue, 1, 0);
  MMAP(packageCustomerCount);
  *packageCustomerCount = 0;

  MMAP(moneyCustomerQueue);
  sem_init(moneyCustomerQueue, 1, 0);
  MMAP(moneyCustomerCount);
  *moneyCustomerCount = 0;
}

void cleanup_shared_variables()
{
  /* Post office status */
  UNMAP(post_office_open);

  sem_destroy(shared_variables_mutex);
  UNMAP(shared_variables_mutex);

  sem_destroy(letterCustomerQueue);
  UNMAP(letterCustomerQueue);
  UNMAP(letterCustomerCount);

  sem_destroy(packageCustomerQueue);
  UNMAP(packageCustomerQueue);
  UNMAP(packageCustomerCount);

  sem_destroy(moneyCustomerQueue);
  UNMAP(moneyCustomerQueue);
  UNMAP(moneyCustomerCount);
}

bool is_post_office_empty()
{
  bool result = true;
  for (size_t i = 0; i < COUNT_OF_REQUEST_TYPES; i++)
  {
    switch (i)
    {
    case letter:
      if (*letterCustomerCount > 0)
      {
        result = false;
      }
      break;

    case package:
      if (*packageCustomerCount > 0)
      {
        result = false;
      }
      break;

    case money:
      if (*moneyCustomerCount > 0)
      {
        result = false;
      }
      break;
    }
  }
  return result;
}

void process_customer(int idZ)
{
  // Každý zákazník je jednoznačně identifikován číslem idZ, 0<idZ<=NZ
  if (0 >= idZ || idZ > NZ)
  {
    fprintf(stderr, "Wrong idZ: %d\n", idZ);
    exit(EXIT_FAILURE);
  }

  // Po spuštění vypíše: A: Z idZ: started
  my_print("Z %d: started\n", idZ);

  // Následně čeká pomocí volání usleep náhodný čas v intervalu <0,TZ>
  usleep(get_random_from_range(0, TZ) * 1000);

  sem_wait(shared_variables_mutex); // lock mutex
  // Pokud je pošta otevřená
  if (*post_office_open)
  {
    sem_post(shared_variables_mutex); // unlock mutex
    // Pokud je pošta otevřená, náhodně vybere činnost X---číslo z intervalu <1,3>
    int X = get_random_from_range(1, COUNT_OF_REQUEST_TYPES);

    // Vypíše: A: Z idZ: entering office for a service X
    my_print("Z %d: entering office for a service %d\n", idZ, X);

    // Zařadí se do fronty X a čeká na zavolání úředníkem.
    switch (X)
    {
    case letter:
      sem_wait(shared_variables_mutex); // lock mutex
      *(letterCustomerCount) += 1;
      sem_post(shared_variables_mutex); // unlock mutex
      sem_wait(letterCustomerQueue);    // add to queue
      break;

    case package:
      sem_wait(shared_variables_mutex); // lock mutex
      *(packageCustomerCount) += 1;
      sem_post(shared_variables_mutex); // unlock mutex
      sem_wait(packageCustomerQueue);   // add to queue
      break;

    case money:
      sem_wait(shared_variables_mutex); // lock mutex
      *(moneyCustomerCount) += 1;
      sem_post(shared_variables_mutex); // unlock mutex
      sem_wait(moneyCustomerQueue);     // add to queue
      break;
    }

    // Vypíše: Z idZ: called by office worker
    my_print("Z %d: called by office worker\n", idZ);

    // Následně čeká pomocí volání usleep náhodný čas v intervalu <0,10> (synchronizace s úředníkem na dokončení žádosti není vyžadována).
    usleep(get_random_from_range(0, SLEEP_INTERVAL_MAX) * 1000);
  }
  else
  {
    sem_post(shared_variables_mutex); // unlock mutex
  }

  // Pokud je pošta uzavřena (a zároveň po dokončení činnosti, když je otevřená)
  // Vypíše: A: Z idZ: going home
  my_print("Z %d: going home\n", idZ);

  // Proces končí
  exit(EXIT_SUCCESS);
}

void process_officer(int idU)
{
  // Každý úředník je jednoznačně identifikován číslem idU, 0<idU<=NU
  if (idU < 0 || idU > NU)
  {
    fprintf(stderr, "Wrong idU: %d\n", idU);
    exit(EXIT_FAILURE);
  }

  // Po spuštění vypíše: A: U idU: started
  my_print("U %d: started\n", idU);

  // [začátek cyklu]
  while (true)
  {
    sem_wait(shared_variables_mutex); // lock mutex
    // Pokud v žádné frontě nečeká zákazník a pošta je zavřená
    if (is_post_office_empty() && !(*post_office_open))
    {
      sem_post(shared_variables_mutex); // unlock mutex
      // - Vypíše: A: U idU: going home
      my_print("U %d: going home\n", idU);

      // - Proces končí
      exit(EXIT_SUCCESS);
    }
    // Pokud v žádné frontě nečeká zákazník a pošta je otevřená
    else if (is_post_office_empty() && *post_office_open)
    {
      sem_post(shared_variables_mutex); // unlock mutex
      // - Vypíše: A: U idU: taking break
      my_print("U %d: taking break\n", idU);

      // - Následně čeká pomocí volání usleep náhodný čas v intervalu <0,TU>
      usleep(get_random_from_range(0, TU) * 1000);

      // - Vypíše: A: U idU: break finished
      my_print("U %d: break finished\n", idU);

      // - Pokračuje na [začátek cyklu]
      continue;
    }
    else if (!is_post_office_empty())
    {
      sem_post(shared_variables_mutex); // unlock mutex
      // Úředník jde obsloužit zákazníka z fronty X (vybere náhodně libovolnou neprázdnou).
      bool is_empty = false;
      int X = -1;
      do
      {
        X = get_random_from_range(1, COUNT_OF_REQUEST_TYPES);
        sem_wait(shared_variables_mutex); // lock mutex
        switch (X)
        {
        case letter:
          if (*letterCustomerCount <= 0)
          {
            sem_post(shared_variables_mutex); // unlock mutex
            is_empty = true;
          }
          else
          {
            *(letterCustomerCount) -= 1;
            sem_post(shared_variables_mutex); // unlock mutex
            sem_post(letterCustomerQueue);    // remove from queue
            is_empty = false;
          }
          break;

        case package:
          if (*packageCustomerCount <= 0)
          {
            sem_post(shared_variables_mutex); // unlock mutex
            is_empty = true;
          }
          else
          {
            *(packageCustomerCount) -= 1;
            sem_post(shared_variables_mutex); // unlock mutex
            sem_post(packageCustomerQueue);   // remove from queue
            is_empty = false;
          }
          break;

        case money:
          if (*moneyCustomerCount <= 0)
          {
            sem_post(shared_variables_mutex); // unlock mutex
            is_empty = true;
          }
          else
          {
            *(moneyCustomerCount) -= 1;
            sem_post(shared_variables_mutex); // unlock mutex
            sem_post(moneyCustomerQueue);     // remove from queue
            is_empty = false;
          }
          break;
        }
      } while (is_empty);
      // Nyní má úředník vybráno z náhodné neprázdné fronty

      // - Vypíše: A: U idU: serving a service of type X
      my_print("U %d: serving a service of type %d\n", idU, X);

      // - Následně čeká pomocí volání usleep náhodný čas v intervalu <0,10>
      usleep(get_random_from_range(0, SLEEP_INTERVAL_MAX) * 1000);

      // - Vypíše: A: U idU: service finished
      my_print("U %d: service finished\n", idU);

      // - Pokračuje na [začátek cyklu]
      continue;
    }
  }
}

int main(int argc, char *argv[])
{
  parse_params(argc, argv);
  prepare_output();
  prepare_shared_variables();

  // Hlavní proces vytváří ihned po spuštění:
  // NZ procesů zákazníků
  for (int i = 1; i <= NZ; i++)
  {
    pid_t id = fork();
    if (id == 0)
    {
      process_customer(i);
      exit(EXIT_SUCCESS);
    }
    else if (id < 0)
    {
      fprintf(stderr, "Error forking customer number %d, pid: %d.\n", i, id);
      exit(EXIT_FAILURE);
    }
  }

  // NU procesů úředníků
  for (int i = 1; i <= NU; i++)
  {
    pid_t id = fork();
    if (id == 0)
    {
      process_officer(i);
      exit(EXIT_SUCCESS);
    }
    else if (id < 0)
    {
      fprintf(stderr, "Error forking officer number %d, pid: %d.\n", i, id);
      exit(EXIT_FAILURE);
    }
  }

  // Čeká pomocí volání usleep náhodný čas v intervalu <F/2,F>
  usleep(get_random_from_range(F / 2, F) * 1000);

  // Close post office
  sem_wait(shared_variables_mutex); // lock mutex
  *post_office_open = false;
  sem_post(shared_variables_mutex); // unlock mutex

  // Vypíše: A: closing
  my_print("closing\n");

  // Poté čeká na ukončení všech procesů, které aplikace vytváří.
  // Wait for children suicide
  while (wait(NULL) > 0)
    ;

  cleanup_shared_variables();
  cleanup_output();

  // Jakmile jsou tyto procesy ukončeny, ukončí se i hlavní proces s kódem (exit code) 0.
  return 0;
}
