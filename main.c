/* CSCI 680: Programming Project #3: Concurrency Management
 * Programmer (Student ID): Ben Corriette (@02956064)
 * Last modified date: 03/25/2022
 * 
 * Summary: Write a program that uses (a) POSIX semaphores 
 * to provide mutual exclusion and (b) UNIX shared memory 
 * segments to store shared variables.
 *
 * References: https://github.com/gentleben8282/csci-401-lab-3-shared-memory-communication/
 * http://www.csc.villanova.edu/~mdamian/threads/posixsem.html
 * https://linux.die.net/man/2/shmat
 */

#include <errno.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

sem_t mutex_semaphore;
int errsv;
int account = 0;
const int MAX_ACCT_BALANCE = 100;

// Generate a random number within a range
int generate_random_number(int upper, int lower) {
    return (rand() % (upper - lower + 1)) + 1;
}

// Start mutual exclusion by locking the critical section
void wait_sem(sem_t mutex_sem) {
  if (sem_wait(&mutex_sem) == -1) {
    errsv = errno;
    printf("The semaphore failed to begin waiting; Error # %d\n", errsv);
    exit(EXIT_FAILURE);
  } else {
    printf("The semaphore has begun waiting...\n");
  }
}

// End mutual exclusion by unlocking the critical section
void signal_sem(sem_t mutex_sem) {
  if (sem_post(&mutex_sem) == -1) {
    errsv = errno;
    printf("The semaphore failed to increment; Error # %d\n", errsv);
    exit(EXIT_FAILURE);
  } else {
    printf("The semaphore has incremented...\n");
  }
}

// Allow the parent process to deposit money
void deposit_money(int shm_memory[], int bank_acct) {
  wait_sem(mutex_semaphore);
  
  int random_amount = generate_random_number(0, MAX_ACCT_BALANCE);

    bank_acct += random_amount;
    printf("Dear old Dad: Deposits $%d / Balance = $%d\n", random_amount, bank_acct);
  
  shm_memory[0] = bank_acct;

  signal_sem(mutex_semaphore);
}

// Allow the child process to withdraw money
void withdraw_money(int shm_memory[], int balance, int account) {
  wait_sem(mutex_semaphore);

  account -= balance;
  printf("Poor Student: Withdraws $%d / Balance = $%d\n", balance, account);
  
  shm_memory[0] = account;

  signal_sem(mutex_semaphore);
}

int main(void) {
  int shm_id;
  int *shm_ptr;
  pid_t process_id;
  int child_status;
  int bank_account = 0;

  printf("The concurrency management program begins...\n");

  // Not using shared semaphore; initial value is 0
  if(sem_init(&mutex_semaphore, 0, 1) == -1) {
    errsv = errno;
    printf("There was a problem with semaphore initialization; Error # %d\n", errsv);
    exit(EXIT_FAILURE);
  } else {
    printf("The semaphore has been initiliazed...\n");
  }
  
  // Create shared memory segment
  shm_id = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666);
  if (shm_id == -1) {
      errsv = errno;
      printf("Shared memory segment failed to be created; Error # %d\n", errsv);
      exit(EXIT_FAILURE);
  } else {
    printf("Parent process has received a shared memory of one integer that represents a bank account...\n");
  }

  // Attach shared memory segment to suitable memory address
  shm_ptr = (int *) shmat(shm_id, NULL, 0);
  if (*shm_ptr == -1) {
      errsv = errno;
      printf("Shared memory segment failed to attach to memory address; Error # %d\n", errsv);
      exit(EXIT_FAILURE);
  } else {
    printf("Parent process has attached the shared memory...\n");
  }

  shm_ptr[0] = bank_account;

  // Create two processes (parent and child)
  printf("Parent process is about to fork a child process...\n");
  process_id = fork();
	
  if (process_id == -1) {
      errsv = errno;
      printf("Child process failed to be forked off from parent; Error # %d\n", errsv);
      exit(EXIT_FAILURE);
  }

  // Handle parent and child processes

  // Child Process
  if (process_id == 0) {

    // Perform the account withdrawal
    int random_balance = generate_random_number(0, 50);
    account = shm_ptr[0];
    printf("Poor Student needs $%d\n", random_balance);
    withdraw_money(shm_ptr, random_balance, account);
			
    printf("Child process exits...\n");
    exit(EXIT_SUCCESS);
  } else if (process_id > 0) { // Parent Process

    // Perform the account deposit
    account = shm_ptr[0]; 
    if (account <= MAX_ACCT_BALANCE) {
      deposit_money(shm_ptr, account);
    }
    
    printf("Parent process exits...\n");
  }
  
  // Child process returns status to parent process
  if (wait(&child_status) == -1) {
      errsv = errno;
      printf("Child process failed to return status to parent; Error # %d\n", errsv);
      exit(EXIT_FAILURE);
  } else {
    printf("Parent process has detected the completion of its child...\n");
  }
  
  // Detach shared memory from memory address
	if (shmdt((void *) shm_ptr) == -1) {
		errsv = errno;
		printf("Shared memory failed to detach from memory address; Error # %d\n", errsv);
		exit(EXIT_FAILURE);
	} else { printf("Parent process has detached its shared memory...\n");
  }
  
  // Destroy shared memory segment
	if (shmctl(shm_id, IPC_RMID, NULL) == -1) {
      errsv = errno;
      printf("Shared memory segment failed to be destroyed; Error # %d\n", errsv);
      exit(EXIT_FAILURE);
	} else { printf("Parent process has removed its shared memory...\n");
  }

  // Destroy semaphore
  if (sem_destroy(&mutex_semaphore) == -1) {
    errsv = errno;
    printf("Semaphore failed to be destroyed; Error # %d\n", errsv);
    exit(EXIT_FAILURE);
} else { printf("Semaphore has been destroyed...\n");
  }
  printf("The program ends...\n");
  return 0;
}