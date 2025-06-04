#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

// Function headers
void init_terminal();
void restore_terminal();
void handle_input();

static struct termios org_term_state, new_term_state;
static char key_state[256] = {};

void init_terminal()
{
    tcgetattr(STDIN_FILENO, &org_term_state);
    new_term_state = org_term_state;
    new_term_state.c_lflag &= ~(ICANON | ECHO);        // Disable flags canonical mode and echo for non-blocking input
    tcsetattr(STDIN_FILENO, TCSANOW, &new_term_state); // Enable flag to apply changes immediately
    fflush(stdout);
}

void restore_terminal()
{
    tcsetattr(STDIN_FILENO, TCSANOW, &org_term_state);
    fflush(stdout);
    printf("Terminal restored\n");
}

void handle_input()
{
    char c;
    while (read(STDIN_FILENO, &c, 1) > 0)
    {
        // Exit program on user quit
        if (c == 'q')
        {
            restore_terminal();
            exit(0);
        }

        printf("You pressed: %c\n", c);
    }
}

int main()
{
    init_terminal();

    // Game Loop
    while (1)
    {
        handle_input();
    }

    restore_terminal();
    return 0;
}
