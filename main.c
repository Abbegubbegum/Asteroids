#include "raylib.h"
#include <math.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

typedef struct
{
	Vector2 pos;
	Vector2 vel;
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
	bool is_on_screen;
} asteroid_t;

const int HEIGHT = 800;
const int WIDTH = 800;

const int BULLET_RADIUS = 5;
const int BULLET_SPEED = 10;
bullet_t bullets[128];
int current_bullet_index = 0;

const int ASTEROID_RADIUS = 15;
int ASTEROID_SPEED = 5;
asteroid_t asteroids[128];
int current_asteroid_index = 0;

int blink_length = 200;

int points = 0;
bool should_close = false;

Vector2 get_dir_vector(float angle)
{
	return (Vector2){cos(angle), sin(angle)};
}

Vector2 get_center_vector(Vector2 pos, int h, int w)
{
	return (Vector2){pos.x + w / 2, pos.y + h / 2};
}

void normalize_vector(Vector2 *p)
{
	float w = sqrt(p->x * p->x + p->y * p->y);
	p->x /= w;
	p->y /= w;
}

Vector2 get_inbetween_dir_vector(Vector2 source, Vector2 target)
{
	Vector2 dir = {target.x - source.x, target.y - source.y};
	normalize_vector(&dir);
	return dir;
}

void draw_player(player_t player)
{
	Vector2 center = get_center_vector(player.pos, player.h, player.w);
	Vector2 dir = get_dir_vector(player.angle);
	DrawRectangleV(player.pos, (Vector2){player.h, player.w}, WHITE);
	DrawLineV(center, (Vector2){center.x + dir.x * 40, center.y + dir.y * 40}, WHITE);
}

void move_player_forward(player_t *player, int dir)
{
	static int speed = 5;
	Vector2 p_dir = get_dir_vector(player->angle);
	player->pos.x += p_dir.x * dir * speed;
	player->pos.y += p_dir.y * dir * speed;
}

void move_player_strafe(player_t *player, int xdir, int ydir)
{
	static int speed = 5;
	player->pos.x += xdir * speed;
	player->pos.y += ydir * speed;

	player->vel.x += xdir;
	player->vel.y += ydir;
}

void blink_player_dir(player_t *player)
{
	Vector2 dir = get_dir_vector(player->angle);

	player->pos.x += dir.x * blink_length;
	player->pos.y += dir.y * blink_length;
}

void blink_player_vel(player_t *player)
{
	player->pos.x += player->vel.x * blink_length;
	player->pos.y += player->vel.y * blink_length;
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

		// IF OUTSIDE OF SCREEN
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

void create_random_asteroid(Vector2 target_pos)
{
	double angle = (rand() % 360) * DEG2RAD;

	asteroid_t asteroid = {
		.pos = {WIDTH / 2 + cos(angle) * WIDTH, HEIGHT / 2 + sin(angle) * HEIGHT},
		.is_on_screen = false,
	};

	asteroid.dir = get_inbetween_dir_vector(asteroid.pos, target_pos);

	asteroids[current_asteroid_index] = asteroid;
	current_asteroid_index++;
}

void remove_asteroid(int index)
{
	current_asteroid_index--;
	asteroids[index] = asteroids[current_asteroid_index];
}

void update_asteroids(player_t player)
{
	for (int i = 0; i < current_asteroid_index; i++)
	{
		asteroids[i].pos.x += asteroids[i].dir.x * ASTEROID_SPEED;
		asteroids[i].pos.y += asteroids[i].dir.y * ASTEROID_SPEED;

		if (CheckCollisionCircleRec(asteroids[i].pos, ASTEROID_RADIUS, (Rectangle){.x = player.pos.x, .y = player.pos.y, .width = player.w, .height = player.h}))
		{
			should_close = true;
		}

		for (int j = 0; j < current_bullet_index; j++)
		{
			if (CheckCollisionCircles(asteroids[i].pos, ASTEROID_RADIUS, bullets[j].pos, BULLET_RADIUS))
			{
				remove_asteroid(i);
				remove_bullet(j);
				points++;
			}
		}

		// IF OUTSIDE OF SCREEN
		if (asteroids[i].pos.x + ASTEROID_RADIUS < 0 || asteroids[i].pos.x - ASTEROID_RADIUS > WIDTH || asteroids[i].pos.y + ASTEROID_RADIUS < 0 || asteroids[i].pos.y - ASTEROID_RADIUS > HEIGHT)
		{
			if (asteroids[i].is_on_screen)
			{
				remove_asteroid(i);
			}
		}
		else if (asteroids[i].is_on_screen == false)
		{
			asteroids[i].is_on_screen = true;
		}
	}
}

void draw_asteroids()
{
	for (int i = 0; i < current_asteroid_index; i++)
	{
		DrawCircleV(asteroids[i].pos, ASTEROID_RADIUS, GREEN);
	}
}

void update_mouse_controls(player_t *player)
{
	Vector2 mouse_pos = GetMousePosition();

	Vector2 pc = get_center_vector(player->pos, player->h, player->w);

	Vector2 dir = get_inbetween_dir_vector(pc, mouse_pos);

	player->angle = atan2(dir.y, dir.x);
}

int main()
{
	srand(time(NULL));

	player_t player = (player_t){.pos = {300, 300}, .w = 20, .h = 20, .angle = 0};
	float asteroid_spawn_timer = 0;
	float blink_timer = 0;
	bool mouseControls = true;

	InitWindow(WIDTH, HEIGHT, "Asteroids");
	SetTargetFPS(60);

	while (!WindowShouldClose())
	{
		// UPDATE
		asteroid_spawn_timer += GetFrameTime();
		blink_timer -= GetFrameTime();

		player.vel = (Vector2){0, 0};

		if (asteroid_spawn_timer > 0.5)
		{
			create_random_asteroid(player.pos);
			asteroid_spawn_timer = 0;
		}

		if (mouseControls)
		{
			update_mouse_controls(&player);
		}

		if (IsKeyDown(KEY_A))
		{
			if (mouseControls)
			{
				move_player_strafe(&player, -1, 0);
			}
			else
			{
				rotate_player(&player, -1);
			}
		}
		if (IsKeyDown(KEY_D))
		{
			if (mouseControls)
			{
				move_player_strafe(&player, 1, 0);
			}
			else
			{
				rotate_player(&player, 1);
			}
		}
		if (IsKeyDown(KEY_W))
		{
			if (mouseControls)
			{
				move_player_strafe(&player, 0, -1);
			}
			else
			{
				move_player_forward(&player, 1);
			}
		}
		if (IsKeyDown(KEY_S))
		{
			if (mouseControls)
			{
				move_player_strafe(&player, 0, 1);
			}
			else
			{
				move_player_forward(&player, -1);
			}
		}

		if (IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(0))
		{
			create_bullet(player);
		}

		if (IsKeyPressed(KEY_E) || IsMouseButtonPressed(1))
		{
			if (blink_timer <= 0)
			{
				blink_player_dir(&player);
				// blink_player_vel(&player);

				blink_timer = 5;
			}
		}

		update_bullets();

		update_asteroids(player);

		// DRAW
		BeginDrawing();
		ClearBackground(BLACK);
		draw_player(player);
		draw_asteroids();
		draw_bullets();
		DrawText(TextFormat("%d", points), 10, 10, 64, WHITE);
		DrawRectangle(10, 100, blink_timer <= 0 ? 0 : 200 * (blink_timer / 5), 20, RED);
		EndDrawing();

		if (should_close)
		{
			break;
		}
	}

	while (!WindowShouldClose() && should_close)
	{
		BeginDrawing();

		ClearBackground(BLACK);

		DrawText(TextFormat("%d", points), WIDTH / 2 - MeasureText(TextFormat("%d", points), 128), HEIGHT / 2 - 20, 128, WHITE);

		EndDrawing();
	}

	CloseWindow();
	return 0;
}