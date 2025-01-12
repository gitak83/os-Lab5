// Console input and output.
// Input is from the keyboard or serial port.
// Output is written to the screen and serial port.

#include "types.h"
#include "defs.h"
#include "param.h"
#include "traps.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"



int back_count = 0;

static void consputc(int);

static int panicked = 0;

static struct {
  struct spinlock lock;
  int locking;
} cons;

static void
printint(int xx, int base, int sign)
{
  static char digits[] = "0123456789abcdef";
  char buf[16];
  int i;
  uint x;

  if(sign && (sign = xx < 0))
    x = -xx;
  else
    x = xx;

  i = 0;
  do{
    buf[i++] = digits[x % base];
  }while((x /= base) != 0);

  if(sign)
    buf[i++] = '-';

  while(--i >= 0)
    consputc(buf[i]);
}
//PAGEBREAK: 50

// Print to the console. only understands %d, %x, %p, %s.
void
cprintf(char *fmt, ...)
{
  int i, c, locking;
  uint *argp;
  char *s;

  locking = cons.locking;
  if(locking)
    acquire(&cons.lock);

  if (fmt == 0)
    panic("null fmt");

  argp = (uint*)(void*)(&fmt + 1);
  for(i = 0; (c = fmt[i] & 0xff) != 0; i++){
    if(c != '%'){
      consputc(c);
      continue;
    }
    c = fmt[++i] & 0xff;
    if(c == 0)
      break;
    switch(c){
    case 'd':
      printint(*argp++, 10, 1);
      break;
    case 'x':
    case 'p':
      printint(*argp++, 16, 0);
      break;
    case 's':
      if((s = (char*)*argp++) == 0)
        s = "(null)";
      for(; *s; s++)
        consputc(*s);
      break;
    case '%':
      consputc('%');
      break;
    default:
      // Print unknown % sequence to draw attention.
      consputc('%');
      consputc(c);
      break;
    }
  }

  if(locking)
    release(&cons.lock);
}

void
panic(char *s)
{
  int i;
  uint pcs[10];

  cli();
  cons.locking = 0;
  // use lapiccpunum so that we can call panic from mycpu()
  cprintf("lapicid %d: panic: ", lapicid());
  cprintf(s);
  cprintf("\n");
  getcallerpcs(&s, pcs);
  for(i=0; i<10; i++)
    cprintf(" %p", pcs[i]);
  panicked = 1; // freeze other CPU
  for(;;)
    ;
}

//PAGEBREAK: 50
#define BACKSPACE       0x100
#define CRTPORT         0x3d4
#define LEFT            0xe4
#define RIGHT           0xe5
#define KEY_UP          0xE2
#define KEY_DN          0xE3
static ushort *crt = (ushort*)P2V(0xb8000);  // CGA memory

int get_pos(){
  // Cursor position: col + 80*row.
  //with this part of the code we get the pos of the cursor
  outb(CRTPORT, 14);
  int pos = inb(CRTPORT+1) << 8;
  outb(CRTPORT, 15);
  pos |= inb(CRTPORT+1);
  return pos;
}

static
void
pos_change(int pos){
  outb(CRTPORT, 14);
  outb(CRTPORT+1, pos>>8);
  outb(CRTPORT, 15);
  outb(CRTPORT+1, pos);
}

#define INPUT_BUF 128
struct Input {
  char buf[INPUT_BUF];
  uint r;  // Read index
  uint w;  // Write index
  uint e;  // Edit index
  uint last; //Last index
} input;

struct 
{
  struct Input hist[10];
  int index;
  int count;
  int last;
}history;


static void
cgaputc(int c)
{
  int pos;
  pos = get_pos();
  

  if(c == '\n')
    pos += 80 - pos%80;
  else if(c == BACKSPACE){
    if(pos > 0) --pos;
  } 
  
  else 
    crt[pos++] = (c&0xff) | 0x0700;  // black on white

  if(pos < 0 || pos > 25*80)
    panic("pos under/overflow");

  if((pos/80) >= 24){  // Scroll up.
    memmove(crt, crt+80, sizeof(crt[0])*23*80);
    pos -= 80;
    memset(crt+pos, 0, sizeof(crt[0])*(24*80 - pos));
  }

  pos_change(pos);
  crt[pos] = ' ' | 0x0700;
}



void
consputc(int c)
{
  if(panicked){
    cli();
    for(;;)
      ;
  }

  if(c == BACKSPACE){
    uartputc('\b'); uartputc(' '); uartputc('\b');
  } else
    uartputc(c);
  cgaputc(c);
}


#define C(x)  ((x)-'@')  // Control-x

int hist_check = 0;
int op_state = 0;

void hand_in(char c){
  if(c != 0 && input.e-input.r < INPUT_BUF){
    c = (c == '\r') ? '\n' : c;
    input.buf[input.e++ % INPUT_BUF] = c;
    consputc(c);
    if(c == '\n' || c == C('D') || input.e == input.r+INPUT_BUF){
      input.w = input.e;
      input.last = input.e;
      wakeup(&input.r);
    }
  }
}

static void show_hist_up(){
  input = history.hist[history.index ];
  history.index -- ;
  input.e -- ;
  input.buf[input.e] = '\0';

  consputc(10);

  for (int i = input.w ; i < input.e; i++)
  {
    consputc(input.buf[i]);
  }
}

static void show_hist_down(){
  if ((history.index < 9)&&(history.index + 1 < history.last )){
    input = history.hist[history.index + 2 ];
    history.index ++ ;
    input.e -- ;
    input.buf[input.e] = '\0';
  }
  
  consputc(10);
  
  for (int i = input.w ; i < input.e; i++)
  {
    consputc(input.buf[i]);
  }
}

int my_atoi(char str) {
    int result = 0;

    result = result * 10 + str - '0';
    return result;
}

int first_num = 0;
int sec_num = 0;
int first_start = 0;
int operator = 0;
int flag = 0;
int counter = 0;
char temp[128];

void copy_handel(int c){
  if(flag == 1){
    temp[counter] = c;
    counter++;
  }
}


void
consoleintr(int (*getc)(void))
{
  int c, doprocdump = 0;

  acquire(&cons.lock);
  while((c = getc()) >= 0){

    switch(c){
    case C('P'):  // Process listing.
      // procdump() locks cons.lock indirectly; invoke later
      hist_check = 0;
      doprocdump = 1;
      break;
    case C('U'):  // Kill line.
      hist_check = 0;
      while(input.e != input.w &&
            input.buf[(input.e-1) % INPUT_BUF] != '\n'){
        input.e--;
        consputc(BACKSPACE);
      }
      break;
    case C('H'): case '\x7f':  // Backspace
      hist_check--;
      if(input.e != input.w){
        input.e++;
        consputc(BACKSPACE);
        input.last--;
      }
      break;

    case C('S'):
      flag=1;
      counter = 0;
     	//temp[counter]=c;
     	//counter++;
      break;

    case C('F'):
      if(flag==1){
        int i=0;
        while(i<counter)
        {
      	  //consputc(input.buf[i]);
          consputc(temp[i]);
     		  i++;
        }
     	  
      }
      flag=0;
      counter = 0;
      break;

    case RIGHT:
      //hist_check = 0;
      if(back_count > 0){
        input.e++;
        int pos = get_pos();

        pos++;

        pos_change(pos);
        back_count--;        

      }
      break;
    case LEFT:
  
      //hist_check = 0;
      if((input.e - back_count) > input.w){

        int pos = get_pos();
        input.e--;
        if (crt[pos - 2] != (('$' & 0xff) | 0x0700)){
          pos--;
        }

        pos_change(pos);
        back_count++;
      }

      break;
    case KEY_UP:
      if ((history.count != 0)  && (history.last - history.index < history.count))
        show_hist_up();
      break;

    case KEY_DN:
      if ((history.count != 0 ) && (history.last - history.index > 0))
        show_hist_down();
      break;

    case 104:
      copy_handel(c);
      hist_check = 1;
      hand_in(c);
      break;

    case 105:
      copy_handel(c);
      if (hist_check == 1)
        hist_check = 2;
      else
        hist_check = 0;
      hand_in(c);
      break;

    case 115:
      copy_handel(c);
      if (hist_check == 2)
        hist_check =3;
      else
        hist_check = 0;
      hand_in(c);
      break;

    case 116:
      copy_handel(c);
      if (hist_check == 3)
        hist_check = 4;
      else
        hist_check = 0;
      hand_in(c);
      break;

    case 111:
      copy_handel(c);
      if (hist_check == 4)
        hist_check = 5;
      else
        hist_check = 0;
      hand_in(c);
      break;

    case 114:
      copy_handel(c);
      if (hist_check == 5)
        hist_check = 6;
      else
        hist_check = 0;
      hand_in(c);
      break;

    case 121:
      copy_handel(c);
      if (hist_check == 6)
        hist_check = 7;
      else
        hist_check = 0;
      hand_in(c);
      break;
    case 10:
      copy_handel(c);
      if(hist_check == 7){
        hist_check = 0;
        while ((history.count != 0)  && (history.last - history.index < history.count))
          show_hist_up();
      }
      else
        hist_check = 0;

    default:
      //hist_check = 0;
      if(c != 0 && input.e-input.r < INPUT_BUF){
        if(flag == 1){
          temp[counter] = c;
          counter++;
        }
        //int count = back_count;
        //int i = input.last - 1;
        /*if (back_count){
          while(i > input.e){
            input.buf[(i + 1)] = input.buf[i];
            i--;
          }
          input.last++;
        }*/
        /*while (count > 0){
          printint(input.e, 10, 0);
          input.buf[(input.e + count + 1) % INPUT_BUF] = input.buf[(input.e + count) % INPUT_BUF] ;
          count--;
        }*/
        hist_check = 0;
        c = (c == '\r') ? '\n' : c;
        input.buf[input.e++ % INPUT_BUF] = c;
        consputc(c);
        /*if (back_count){
          int j = input.e;
          while (j < input.last){
            printint(30, 10, 0);
            //consputc(input.buf[j]);
            j++;
          }
        }*/
        if(c == '\n' || c == C('D') || input.e == input.r+INPUT_BUF){
          if (history.count < 9){
            history.hist[history.last + 1] = input;
            history.last ++ ;
            history.index = history.last;
            history.count ++ ;
          }
          else{
            for (int h = 0; h < 9; h++) {
              history.hist[h] = history.hist[h+1]; 
            }
            history.hist[9] = input;
            history.index = 9;
            history.last = 9;
            history.count = 10;
          }
          input.w = input.e;
      
          wakeup(&input.r);
        }
      }
      break;
    }

    if (c >= 48 && c <= 57){
      switch(op_state){
      case 0 :
        first_start = input.e;
        op_state = 1;
        first_num = first_num * 10 + c - 48;
        break;

      case 1:
        first_num = first_num * 10 + c - 48;
        break;

      case 2:
        op_state = 3;
        sec_num = sec_num * 10 + c - 48;
      break;

      case 3:
        sec_num = sec_num * 10 + c - 48;
        break;

      default:
        break;
      }
    }

    if (c == 45 /*-*/|| c == 43 /*+*/|| c == 42 /*mult*/|| c == 37 /*%*/|| c == 47/*divide*/){
      if (op_state == 1){
        op_state = 2;
        operator = c;
      }
    }

    if(c== 61 /*=*/){
      if (op_state == 3){
        op_state = 4;
      }
    }

    if(c == 63 /*?*/){
      int result = 0;
      if (op_state == 4){

        switch (operator)
        {
        case 45:
          result = first_num - sec_num;
          break;
        
        case 43:
          result = first_num + sec_num;
          break;

        case 42:
          result = first_num * sec_num;
          break;

        case 37:
          result = first_num % sec_num;
          break;

        case 47:
          result = first_num / sec_num;
          break;

        default:
          break;
        }
        
        while(input.e >= first_start){
          consputc(BACKSPACE);
          input.e--;
        }
        int sign_ = 0;
        if (result < 0)
          sign_ = 1;
        printint(result, 10, sign_);
        first_num = 0;
        sec_num = 0;
        first_start = 0;
        operator = 0;
        op_state = 0;
      }
    }
  }
  release(&cons.lock);
  if(doprocdump) {
    procdump();  // now call procdump() wo. cons.lock held
  }
}

int
consoleread(struct inode *ip, char *dst, int n)
{
  uint target;
  int c;

  iunlock(ip);
  target = n;
  acquire(&cons.lock);
  while(n > 0){
    while(input.r == input.w){
      if(myproc()->killed){
        release(&cons.lock);
        ilock(ip);
        return -1;
      }
      sleep(&input.r, &cons.lock);
    }
    c = input.buf[input.r++ % INPUT_BUF];
    if(c == C('D')){  // EOF
      if(n < target){
        // Save ^D for next time, to make sure
        // caller gets a 0-byte result.
        input.r--;
      }
      break;
    }
    *dst++ = c;
    --n;
    if(c == '\n')
      break;
  }
  release(&cons.lock);
  ilock(ip);

  return target - n;
}

int
consolewrite(struct inode *ip, char *buf, int n)
{
  int i;

  iunlock(ip);
  acquire(&cons.lock);
  for(i = 0; i < n; i++)
    consputc(buf[i] & 0xff);
  release(&cons.lock);
  ilock(ip);

  return n;
}

void
consoleinit(void)
{
  initlock(&cons.lock, "console");

  devsw[CONSOLE].write = consolewrite;
  devsw[CONSOLE].read = consoleread;
  cons.locking = 1;

  ioapicenable(IRQ_KBD, 0);
}

