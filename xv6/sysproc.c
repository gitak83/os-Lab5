#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "syscall.h"

char map[26][30] = {
  "fork", 
  "exit", 
  "wait",
  "pipe",
  "read",
  "kill",
  "exec",
  "fstat",
  "chdir",
  "dup",
  "getpid",
  "sbrk",
  "sleep",
  "uptime",
  "open",
  "write",
  "mknod",
  "unlink",
  "link",
  "mkdir",
  "close",
  "",
  "move",
  "sort_syscalls",
  "most_invoked",
  "list",
};


int
sys_fork(void)
{
  record_syscall(SYS_fork);
  return fork();
}

int
sys_exit(void)
{
  record_syscall(SYS_exit);
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  record_syscall(SYS_wait);
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  record_syscall(SYS_kill);
  return kill(pid);
}

int
sys_getpid(void)
{
  record_syscall(SYS_getpid);
  return myproc()->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  record_syscall(SYS_sbrk);
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  record_syscall(SYS_sleep);
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  record_syscall(SYS_uptime);
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}


int sort_syscalls(int pid) {
    struct proc *p;
    // Ensure process exists and retrieve it
    int status = get_process_by_pid(pid, &p);
    if (status == -1) {
        cprintf("Error: Process with PID %d not found.\n", pid);
        return -1;
    }
    // Selection sort on syscall numbers
    struct syscall_entry temp;
    for (int i = 0; i < p->syscall_count - 1; i++) {
        int min_index = i;
        for (int j = i + 1; j < p->syscall_count; j++) {
            if (p->syscalls[j].syscall_number < p->syscalls[min_index].syscall_number) {
                min_index = j;
            }
        }
        // Swap if a new minimum is found
        if (min_index != i) {
            temp = p->syscalls[i];
            p->syscalls[i] = p->syscalls[min_index];
            p->syscalls[min_index] = temp;
        }
    }
    // Verify the sorting results
    cprintf("After sorting:\n");
    for (int i = 0; i < p->syscall_count; i++) {
        cprintf("Sorted - Syscall number: %d, Count: %d\n", 
                p->syscalls[i].syscall_number, p->syscalls[i].count);
    }
    return 0;
}


int
sys_sort_syscalls(void) {
    int pid;
    if (argint(0, &pid) < 0) {
        return -1; // handle error if pid not provided
    }
    return sort_syscalls(pid); // Call your actual sort function with the pid
}

int get_most_invoked_syscall(int pid){
    struct proc *p;

    // Ensure process exists and retrieve it
    int status = get_process_by_pid(pid, &p);
    if (status == -1) {
        cprintf("Error: Process with PID %d not found.\n", pid);
        return -1;
    }


    if(p->syscall_count == 0){
      cprintf("no systemcall has been invoked.\n");
      return -1; 
    }

    int max_count = p->syscalls[0].syscall_number;
    int max_index = 0;
    for (int i = 1; i < p->syscall_count; i++) {
      if(p->syscalls[i].syscall_number > max_count){
        max_count = p->syscalls[i].count;
        max_index = i;
      }
    }

    cprintf("the most got invoked syscall: %s ", map[p->syscalls[max_index].syscall_number - 1]);
    cprintf("with %d number of calls\n", max_count);
    // Verify the sorting results
    return 0;
}

int sys_get_most_invoked_syscall(void){
  int pid;
    if (argint(0, &pid) < 0) {
        return -1; // handle error if pid not provided
    }
    return get_most_invoked_syscall(pid); // Call your actual sort function with the pid
}

int sys_list_all_processes(void){
    return list_all_processes();
}

void sys_find_palindrome(void){
  int num = myproc()->tf->ebx;
  cprintf("KERNEL: sys_find_palindrome(%d)\n" , num);
  find_palindrome(num);
}
extern int close_sharedmem(void*);
extern void * open_sharedmem(int);

int sys_close_sharedmem(void)
{
  int i;
  if(argint(0,&i)<0)
    return 0;
  return close_sharedmem((void*)i);
}

void*
sys_open_sharedmem(void)
{
  int shmid;
  if(argint(0, &shmid) < 0)
    return (void*)0;

  return open_sharedmem(shmid);
}