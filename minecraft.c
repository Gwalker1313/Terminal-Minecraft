#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

#define X_PIXELS 400
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
    fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL, 0) | O_NONBLOCK);
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
    // Flush existing key states.
    for (int i = 0; i < 256; i++)
    {
        key_state[i] = 0;
    }

    while (read(STDIN_FILENO, &c, 1) > 0)
    {
        // printf("You pressed: %c\n", c);
        key_state[(unsigned char)c] = 1;
    }
}

int key_pressed(char key) { return key_state[(unsigned char)key]; }

char** init_picture()
{
    char** picture = malloc(sizeof(char*) * Y_PIXELS);
    for (int i = 0; i < Y_PIXELS; i++)
    {
        picture[i] = malloc(sizeof(char) * X_PIXELS);
    }
    return picture;
}

/// @brief Defines the layout of the blocks in the world.
/// @return A 3D array of blocks representing an empty world.
char*** init_blocks()
{
    char*** blocks = malloc(sizeof(char**) * Z_BLOCKS);
    for (int i = 0; i < Z_BLOCKS; i++)
    {
        blocks[i] = malloc(sizeof(char*) * Y_BLOCKS);
        for (int j = 0; j < Y_BLOCKS; j++)
        {
            blocks[i][j] = malloc(sizeof(char) * X_BLOCKS);
            for (int k = 0; k < X_BLOCKS; k++)
            {
                blocks[i][j][k] = ' ';
            }
        }
    }

    return blocks;
}

player_pos_view init_posview()
{
    player_pos_view player;
    player.pos.x = 5;
    player.pos.y = 5;
    player.pos.z = 5;
    player.view.theta = 0;
    player.view.phi = 0;
    return player;
}

vect1** init_directions(vect2 view)
{
    vect1** dir = malloc(sizeof(vect1*) * Y_PIXELS);
    for (int i = 0; i < Y_PIXELS; i++)
    {
        dir[i] = malloc(sizeof(vect1) * X_PIXELS);
    }
    for (int y = 0; y < Y_PIXELS; y++)
    {
        for (int x = 0; x < X_PIXELS; x++)
        {
            vect1 temp;
            dir[y][x] = temp;
        }
    }

    return dir;
}

char raytrace(vect1 pos, vect1 dir, char*** blocks) { return 'b'; }

char** get_picture(char** picture, player_pos_view player, char*** blocks)
{
    vect1** directions = init_directions(player.view);
    for (int y = 0; y < Y_PIXELS; y++)
    {
        for (int x = 0; x < X_PIXELS; x++)
        {
            picture[y][x] = raytrace(player.pos, directions[y][x], blocks);
        }
    }
}

void draw_ASCII(char** picture)
{
    fflush(stdout);
    for (int i = 0; i < Y_PIXELS; i++)
    {
        for (int j = 0; j < X_PIXELS; j++)
        {
            printf("%c", picture[i][j]);
        }
        printf("\n");
    }
    return;
}

int main()
{
    init_terminal();
    char** picture = init_picture();
    char*** blocks = init_blocks();

    for (int i = 0; i < X_BLOCKS; i++)
    {
        for (int j = 0; j < Y_BLOCKS; j++)
        {
            for (int k = 0; k < 4; k++)
            {
                blocks[k][j][i] = '@';
            }
        }
    }

    player_pos_view player = init_posview();
    // Game Loop
    while (1)
    {
        handle_input();
        // printf("WE'RE FREE!\n");
        usleep(20000);
        if (key_pressed('q'))
        {
            restore_terminal();
            exit(0);
        }

        // Get picture
        get_picture(picture, player, blocks);

        // Draw ASCII
        draw_ASCII(picture);
    }

    restore_terminal();
    return 0;
}
