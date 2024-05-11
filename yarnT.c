#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>

// defines

#define CTRL_KEY(k) ((k) & 0x1f)

struct termios orig_termios; // store original terminal attributes

void die(const char *s)
{
    perror(s);
    exit(1);
}

void disableRawMode()
{
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1)
        die("tcsetattr");
}

// enable raw mode for terminal input
void enableRawMode()
{
    // get original terminal attributes
    if (tcgetattr(STDIN_FILENO, &orig_termios) == -1)
        die("tcgetattr");

    // save terminal attributes
    tcgetattr(STDIN_FILENO, &orig_termios);

    atexit(disableRawMode);

    struct termios raw = orig_termios; 

    tcgetattr(STDIN_FILENO, &raw); 

    // modify terminal attributes for raw mode
    raw.c_lflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON); 
    raw.c_lflag &= ~(OPOST); 
    raw.c_lflag |= (CS8); 
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG); 
    raw.c_cc[VMIN] = 0; 
    raw.c_cc[VTIME] = 1;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1)
        die("tcsetattr");
}

// read keyboard input
char editorReadKey() 
{
    int nread;
    char c;

    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        if (nread == -1 && errno != EAGAIN)
            die("read");
    }
    return c;
}

void editorProcessKeypress() 
{
    char c = editorReadKey();

    switch (c) 
    {
        case CTRL_KEY('q'): exit(0); break;
    }
}

int main() 
{
    enableRawMode();

    while (1) {
        editorProcessKeypress();
    }
    return 0;
}