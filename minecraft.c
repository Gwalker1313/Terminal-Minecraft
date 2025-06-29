#include <fcntl.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

#define X_PIXELS 900
#define Y_PIXELS 180

#define X_BLOCKS 20
#define Y_BLOCKS 20
#define Z_BLOCKS 10
#define BLOCK_BORDER_SIZE 0.025

#define VIEW_HEIGHT 0.7
#define VIEW_WIDTH 1

#define EYES_HEIGHT 1.5

typedef struct Vector1
{
    float x;
    float y;
    float z;
} vect1;

typedef struct Vector2
{
    float theta;
    float phi;
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
    player.pos.z = 4 + EYES_HEIGHT;
    player.view.phi = 0;
    player.view.theta = 0;
    return player;
}

vect1 angles_to_vector(vect2 angles)
{
    vect1 res;
    res.x = cos(angles.theta) * cos(angles.phi);
    res.y = cos(angles.theta) * sin(angles.phi);
    res.z = sin(angles.theta);
    return res;
}

vect1 vect_add(vect1 v1, vect1 v2)
{
    vect1 res = {v1.x + v2.x, v1.y + v2.y, v1.z + v2.z};
    return res;
}

vect1 vect_sub(vect1 v1, vect1 v2)
{
    vect1 res = {v1.x - v2.x, v1.y - v2.y, v1.z - v2.z};
    return res;
}

vect1 vect_scale(float s, vect1 v)
{
    vect1 res = {s * v.x, s * v.y, s * v.z};
    return res;
}

void vect_normalize(vect1* v)
{
    float length = sqrt(v->x * v->x + v->y * v->y + v->z * v->z);
    v->x /= length;
    v->y /= length;
    v->z /= length;
}

/// @brief Initializes directional vectors for each pixel on the screen from the player's view.
/// @param view The player's view position and orientation in spherical coordinates.
/// @return A pointer to a 2D array of directional vectors for each pixel on the screen.
vect1** init_directions(vect2 view)
{
    view.theta -= VIEW_HEIGHT / 2.0;
    vect1 screen_down = angles_to_vector(view);
    view.theta += VIEW_HEIGHT;
    vect1 screen_up = angles_to_vector(view);
    view.theta -= VIEW_HEIGHT / 2.0;
    view.phi -= VIEW_WIDTH / 2.0;
    vect1 screen_left = angles_to_vector(view);
    view.phi += VIEW_WIDTH;
    vect1 screen_right = angles_to_vector(view);
    view.phi -= VIEW_WIDTH / 2.0;

    vect1 screen_center_vert = vect_scale(0.5, vect_add(screen_up, screen_down));
    vect1 screen_center_hori = vect_scale(0.5, vect_add(screen_left, screen_right));

    vect1 center_to_left = vect_sub(screen_left, screen_center_hori);
    vect1 center_to_up = vect_sub(screen_up, screen_center_vert);

    vect1** dir = malloc(sizeof(vect1*) * Y_PIXELS);
    for (int i = 0; i < Y_PIXELS; i++)
    {
        dir[i] = malloc(sizeof(vect1) * X_PIXELS);
    }
    for (int y = 0; y < Y_PIXELS; y++)
    {
        for (int x = 0; x < X_PIXELS; x++)
        {
            vect1 temp = vect_add(vect_add(screen_center_hori, center_to_left), center_to_up);
            temp = vect_sub(temp, vect_scale(((float)x / (X_PIXELS - 1)) * 2, center_to_left));
            temp = vect_sub(temp, vect_scale(((float)y / (Y_PIXELS - 1)) * 2, center_to_up));
            vect_normalize(&temp);
            dir[y][x] = temp;
        }
    }

    return dir;
}

float min(float a, float b)
{
    if (a < b)
    {
        return a;
    }
    return b;
}

/// @brief Checks if a given point is along any two given axises (voxel face) within a given threshold (the border size).
/// @param pos A point in 3D space stored as a vector struct.
/// @return True if the given point is on the border of a voxel face, false otherwise.
int on_block_border(vect1 pos)
{
    int aligned_axises = 0;
    if (fabsf(pos.x - roundf(pos.x)) < BLOCK_BORDER_SIZE)
    {
        aligned_axises++;
    }
    if (fabsf(pos.y - roundf(pos.y)) < BLOCK_BORDER_SIZE)
    {
        aligned_axises++;
    }
    if (fabsf(pos.z - roundf(pos.z)) < BLOCK_BORDER_SIZE)
    {
        aligned_axises++;
    }
    if (aligned_axises >= 2)
    {
        return 1;
    }
    return 0;
}

/// @brief Checks if a given point is outside the bounds of the 3D grid world.
/// @param pos A point in 3D space stored as a vector struct.
/// @return True if the given point is out-of-bounds, false otherwise.
int ray_outside(vect1 pos)
{
    if (pos.x >= X_BLOCKS || pos.y >= Y_BLOCKS || pos.z >= Z_BLOCKS || pos.x < 0 || pos.y < 0 || pos.z < 0)
    {
        return 1;
    }
    return 0;
}

char raytrace(vect1 pos, vect1 dir, char*** blocks)
{
    float eps = 0.02; // Mantissa buffer to avoid floating point edge cases.
    while (!ray_outside(pos))
    {
        char c = blocks[(int)pos.z][(int)pos.y][(int)pos.x];
        if (c != ' ')
        {
            if (on_block_border(pos))
            {
                return '-';
            }
            else
            {
                return c;
            }
        }
        // Step along each axis using the smallest distance to ensure hitting the nearest axis.
        float dist = 2;
        if (dir.x > eps)
        {
            dist = min(dist, ((int)(pos.x + 1) - pos.x) / dir.x);
        }
        else if (dir.x < -eps)
        {
            dist = min(dist, ((int)pos.x - pos.x) / dir.x);
        }
        if (dir.y > eps)
        {
            dist = min(dist, ((int)(pos.y + 1) - pos.y) / dir.y);
        }
        else if (dir.y < -eps)
        {
            dist = min(dist, ((int)pos.y - pos.y) / dir.y);
        }
        if (dir.z > eps)
        {
            dist = min(dist, ((int)(pos.z + 1) - pos.z) / dir.z);
        }
        else if (dir.z < -eps)
        {
            dist = min(dist, ((int)pos.z - pos.z) / dir.z);
        }
        pos = vect_add(pos, vect_scale(dist + eps, dir));
    }
    return ' ';
}

void get_picture(char** picture, player_pos_view player, char*** blocks)
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
    // printf("\033[0;0H");        // Clear the screen by resetting the cursor.
    printf("\033[2j\033[0;0H"); // Clear the screen by resetting the cursor.
    for (int i = 0; i < Y_PIXELS; i++)
    {
        for (int j = 0; j < X_PIXELS; j++)
        {
            printf("%c", picture[i][j]);
        }
        // printf("\033[0m\n");
        printf("\n");
    }
    return;
}

void update_player(player_pos_view* player, char*** blocks)
{
    float movement_eps = 0.30;
    float tilt_eps = 0.1;
    if (key_pressed('w'))
    {
        player->view.theta += tilt_eps;
    }
    if (key_pressed('a'))
    {
        player->view.phi -= tilt_eps;
    }
    if (key_pressed('s'))
    {
        player->view.theta -= tilt_eps;
    }
    if (key_pressed('d'))
    {
        player->view.phi += tilt_eps;
    }
}

int main()
{
    init_terminal();
    // Green - 32m, Cyan - 36m, Red - 31m, Blue - 34m, Yellow - 33m, White - 37m
    printf("\033[36m\033[40m"); // Set terminal output color to green via ANSI escape code.
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
        if (key_pressed('q'))
        {
            restore_terminal();
            exit(0);
        }
        update_player(&player, blocks);

        // Get picture
        get_picture(picture, player, blocks);

        // Draw ASCII
        draw_ASCII(picture);
        usleep(20000);
    }

    restore_terminal();
    return 0;
}
