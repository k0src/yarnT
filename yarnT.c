#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <sys/ioctl.h>
// hellllo
// defines
#define CTRL_KEY(k) ((k) & 0x1f)

struct editorConfig 
{
    int screen_rows;
    int screen_cols;
    struct termios orig_termios; 
};

struct editorConfig E;

void die(const char *s)
{
    write(STDOUT_FILENO, "\x1b[2j", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);

    perror(s);
    exit(1);
}

void disableRawMode()
{
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1)
        die("tcsetattr");
}

// enable raw mode for terminal input
void enableRawMode()
{
    // get original terminal attributes
    if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1)
        die("tcgetattr");

    // save terminal attributes
    tcgetattr(STDIN_FILENO, &E.orig_termios);

    atexit(disableRawMode);

    struct termios raw = E.orig_termios; 

    tcgetattr(STDIN_FILENO, &raw); 

    // modify terminal attributes for raw mode
    raw.c_lflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON); 
    raw.c_lflag &= ~(OPOST); 
    raw.c_lflag |= (CS8); 
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG); 
    raw.c_cc[VMIN] = 0; 
    raw.c_cc[VTIME] = 1;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1)
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

int getWindowSize(int* rows, int* cols)
{
    struct winsize ws;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0)
        return -1;
    else {
        *rows = ws.ws_row;
        *cols = ws.ws_col;
        return 0;
    }
}

void editorProcessKeypress() 
{
    char c = editorReadKey();

    switch (c) 
    {
        case CTRL_KEY('q'): exit(0); break;
    }
}

// output
void editorDrawRows() 
{
    int y;
    for (y = 0; y < E.screen_rows; y++) {
        write(STDOUT_FILENO, "@\r\n", 3);
    }
}

void editorRefreshScreen() 
{
    write(STDOUT_FILENO, "\x1b[2j", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);

    editorDrawRows();

    write(STDOUT_FILENO, "\x1b[H", 3);
}

void initEditor() {
    if (getWindowSize(&E.screen_rows, &E.screen_cols) == -1)
        die("getWindowSize");
}

int main() 
{
    enableRawMode();
    initEditor();

    while (1) {
        editorRefreshScreen();
        editorProcessKeypress();
    }
    return 0;
}
