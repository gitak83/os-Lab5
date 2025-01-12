#include "types.h"
#include "user.h"
#include "stat.h"
#include "fcntl.h"    
#define KEY 25

char encode(char c) {
    if (c >= 'a' && c <= 'z') {
        return ((c - 'a' + KEY + 26) % 26) + 'a';
    } else if (c >= 'A' && c <= 'Z') {
        return ((c - 'A' + KEY + 26) % 26) + 'A';
    }
    return c;
}



int main(int argc,char *argv[]) {
  //  unlink("result.txt");

    int fd = open("result.txt", O_CREATE|O_RDWR );
    for (int i = 1; i < argc ; i++) {
        for (int j=0; j< strlen(argv[i]); j++){
            char processedChar = encode(argv[i][j]);
            write(fd, &processedChar, 1);
        }
        
        write(fd," ",1);                
    }
    write(fd,"\n",1);

    close(fd);
    exit();
}
