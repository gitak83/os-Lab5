#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

int main() {
    char* src_file="src4";
    char* dest_dir="dst4";
    int result = move_file(src_file, dest_dir);
    if (result < 0) {
        printf(2, "Error: Failed to move file from %s to %s\n", src_file, dest_dir);
    } else {
        printf(1, "Successfully moved file from %s to %s\n", src_file, dest_dir);
    }

    exit();
}
