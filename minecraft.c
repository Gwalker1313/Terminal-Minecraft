#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

#define X_PIXELS 200
#define Y_PIXELS 200

#define X_BLOCKS 20
#define Y_BLOCKS 20
#define Z_BLOCKS 10

typedef struct Vector1
{
    int x;
    int y;
    int z;
} vect1;

typedef struct Vector2
{
    int theta;
    int phi;
} vect2;

typedef struct Vector_Vector2
{
    vect1 pos;
    vect2 view;
} player_pos_view;

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
        key_state[(unsigned char)c] = 1;
        printf("You pressed: %c\n", c);
    }
}

int key_pressed(char key) { return key_state[(unsigned char)key]; }

char** init_picture()
{
    char*** picture = malloc(sizeof(char*) * Y_PIXELS);
    for (int i = 0; i < Y_PIXELS; i++)
    {
        picture[i] = malloc(sizeof(char) * X_PIXELS);
    }
}

char*** init_blocks() { return NULL; }

player_pos_view init_player()
{
    player_pos_view player;
    player.pos.x = 5;
    player.pos.y = 5;
    player.pos.z = 5;
    player.view.theta = 0;
    player.view.phi = 0;
    return player;
}

int main()
{
    init_terminal();
    char** picture = init_picture();
    char*** blocks = init_blocks();

    // Game Loop
    while (1)
    {
        handle_input();
        // if (key_pressed('q'))
        // {
        //     exit(0);
        // }
    }

    restore_terminal();
    return 0;
}
