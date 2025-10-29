#include<unistd.h>
#include<termios.h>
#include<stdlib.h>
#include<errno.h>
#include<ctype.h>
#include<stdio.h>
#include<sys/ioctl.h>
#define CTRL_KEY(k) ((k) & 0x1f)
// ------------data----------
struct editorConfig{
  int screenRows;
  int screenCols;
  struct termios orig_termios;
};
struct editorConfig E;

//create a copy of the fd struct frpm the terminql
//-------terminal-------------
void die(char *s){
  write(STDIN_FILENO, "\x1b[2J", 4);
  write(STDIN_FILENO, "\x1b[H", 3);
  perror(s);
  exit(1);
}
void diable_raw_mode(){
  if( tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1){
    die("tcgetattr");
  }

}
void enable_raw_mode(){
  if(tcgetattr(STDIN_FILENO, &E.orig_termios) == -1){
    die("tcgetattr");
  }
  atexit(diable_raw_mode);
  // create a copy of the original terminos struct to mke changes locally 
  struct termios raw = E.orig_termios;
  raw.c_iflag &= ~( BRKINT | INPCK | ISTRIP |IXON | ICRNL);
  raw.c_cflag |= ~(CS8);
  raw.c_oflag &= ~(OPOST);
  raw.c_lflag &= ~(ECHO | ICANON | ISIG |IEXTEN);
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 1;
  if(tcsetattr(STDIN_FILENO,TCSAFLUSH,&raw) == -1){
    die("tcsetattr");
  }
}

int getWindowSize(int *rows, int *cols){
  struct winsize ws;

  if(ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0){
    return -1;
  }
  else{
    *cols = ws.ws_col;
    *rows = ws.ws_row;
    return 0;
  }
}
//-------------input-----------------

char editorReadKey(){
  int nread;
  char c;
  while((nread = read(STDIN_FILENO, &c, 1))!=1){
    if(nread == -1 && errno != EAGAIN) die("read");
  }
  return c;
}

void editorProcessKeyPress(){
  char c= editorReadKey();
  switch (c) {
    case CTRL_KEY('q'):
  write(STDIN_FILENO, "\x1b[2J", 4);
  write(STDIN_FILENO, "\x1b[H", 3);
      exit(0);
      break;
  }

}

//------------output-----------------

void editorDrawRows(){
  int y;
  for(y = 0;y<E.screenRows; y++){
    write(STDIN_FILENO, "~", 1);

    if(y<E.screenRows -1){
      write(STDIN_FILENO, "\r\n", 2);//print the last tilde int he terminal
    }
  }
}

void editorRefreshScreen(){
  write(STDIN_FILENO, "\x1b[2J", 4);
  write(STDIN_FILENO, "\x1b[H", 3);
  editorDrawRows();
  write(STDIN_FILENO, "\x1b[H", 3);
}

//------------init-----------------
//

void initEditor(){
  if(getWindowSize(&E.screenRows, &E.screenCols) == -1) die("getWindowSize0");
}

int main(){
  enable_raw_mode();
  initEditor();
  //read input from the terminal
      //control charecterare non printable
      //the above function is from ctype.h is a function to check if the charecter is a control charecter
  
    while (1) {
    editorRefreshScreen();
     editorProcessKeyPress();
  }
  return 0;
}
