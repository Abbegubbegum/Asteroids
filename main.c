#include "raylib.h"
#include <math.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

typedef struct
{
    Vector2 pos;
    int h;
    int w;
    float angle;
} player_t;

typedef struct
{
    Vector2 pos;
    Vector2 dir;
} bullet_t;

typedef struct
{
    Vector2 pos;
    Vector2 dir;
} asteroid_t;

const int HEIGHT = 800;
const int WIDTH = 800;

const int BULLET_RADIUS = 5;
const int BULLET_SPEED = 10;
bullet_t bullets[128];
int current_bullet_index = 0;

const int ASTEROID_RADIUS = 30;
int ASTEROID_SPEED = 10;
asteroid_t asteroids[128];
int current_asteroid_index = 0;

Vector2 get_dir_vector(float angle)
{
    return (Vector2){cos(angle), sin(angle)};
}

Vector2 get_center_vector(Vector2 pos, int h, int w)
{
    return (Vector2){pos.x + w / 2, pos.y + h / 2};
}

void draw_player(player_t player)
{
    Vector2 center = get_center_vector(player.pos, player.h, player.w);
    Vector2 dir = get_dir_vector(player.angle);
    DrawRectangleV(player.pos, (Vector2){player.h, player.w}, WHITE);
    DrawLineV(center, (Vector2){center.x + dir.x * 40, center.y + dir.y * 40}, WHITE);
}

void move_player(player_t *player, int dir)
{
    static int speed = 5;
    Vector2 p_dir = get_dir_vector(player->angle);
    player->pos.x += p_dir.x * dir * speed;
    player->pos.y += p_dir.y * dir * speed;
}

// DIR = -1 === CTR CLOCKWISE, DIR = 1 === CLOCKWISE
void rotate_player(player_t *player, int dir)
{
    static float rotation_speed = 0.07;

    player->angle += dir * rotation_speed;
}

void create_bullet(player_t player)
{
    bullet_t bullet = {get_center_vector(player.pos, player.h, player.w), get_dir_vector(player.angle)};

    bullets[current_bullet_index] = bullet;
    current_bullet_index++;
}

void remove_bullet(int index)
{
    current_bullet_index--;
    bullets[index] = bullets[current_bullet_index];
}

void update_bullets()
{
    for (int i = 0; i < current_bullet_index; i++)
    {
        bullets[i].pos.x += bullets[i].dir.x * BULLET_SPEED;
        bullets[i].pos.y += bullets[i].dir.y * BULLET_SPEED;

        // IF OUTSIDE
        if (bullets[i].pos.x + BULLET_RADIUS < 0 || bullets[i].pos.x - BULLET_RADIUS > WIDTH || bullets[i].pos.y + BULLET_RADIUS < 0 || bullets[i].pos.y - BULLET_RADIUS > HEIGHT)
        {
            remove_bullet(i);
        }
    }
}

void draw_bullets()
{
    for (int i = 0; i < current_bullet_index; i++)
    {
        DrawCircleV(bullets[i].pos, BULLET_RADIUS, RED);
    }
}

void create_random_asteroid()
{
    int side = rand() % 4;
}

int main()
{
    srand(time(NULL));

    player_t player = {{300, 300}, 20, 20, 0};

    InitWindow(WIDTH, HEIGHT, "Asteroids");
    SetTargetFPS(60);

    while (!WindowShouldClose())
    {
        // UPDATE
        if (IsKeyDown(KEY_A))
        {
            rotate_player(&player, -1);
        }
        if (IsKeyDown(KEY_D))
        {
            rotate_player(&player, 1);
        }
        if (IsKeyDown(KEY_W))
        {
            move_player(&player, 1);
        }
        if (IsKeyDown(KEY_S))
        {
            move_player(&player, -1);
        }
        if (IsKeyPressed(KEY_SPACE))
        {
            create_bullet(player);
        }

        update_bullets();

        // DRAW
        BeginDrawing();
        ClearBackground(BLACK);
        draw_player(player);
        draw_bullets();
        EndDrawing();
    }

    CloseWindow();
    return 0;
}