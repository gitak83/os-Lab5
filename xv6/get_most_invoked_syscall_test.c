#include "types.h"
#include "stat.h"
#include "user.h"


int main() {
    int pid = 2;

    get_most_invoked_syscall(pid);

    wait();
    wait();
    wait();

    exit();
}