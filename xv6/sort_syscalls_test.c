#include "types.h"
#include "stat.h"
#include "user.h"

//#include "mmu.h"
//#include "x86.h"
//#include "param.h"


int main() {
    int pid = 2;

    sort_syscalls(pid);

    wait();
    wait();
    wait();

    exit();
}