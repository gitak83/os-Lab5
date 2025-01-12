#include "types.h"
#include "stat.h"
#include "user.h"



int main() {
    list_all_processes();

    wait();
    wait();
    wait();

    exit();
}