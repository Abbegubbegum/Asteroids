#include "raylib.h"
#include <math.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>

typedef enum
{
	GS_NONE,
	GS_RUNNING,
	GS_GAMEOVER,
} gamestate_e;

typedef enum
{
	PW_NONE,
	PW_BULLET_TIME,
} powerup_e;

typedef struct
{
	Vector2 pos;
	Vector2 vel;
	int h;
	int w;
	float angle;
	powerup_e active_powerup;
	float powerup_timer;
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
	float size;
	int speed;
	int point_value;
	bool is_on_screen;
	powerup_e powerup;
} asteroid_t;

typedef struct
{
	Vector2 pos;
	Vector2 dir;
	float speed;
	float size;
} particle_t;

typedef struct
{
	Vector2 origin;
	particle_t *particles;
} particle_group_t;

const int HEIGHT = 800;
const int WIDTH = 800;

const int BULLET_RADIUS = 5;
const int BULLET_SPEED = 10;
bullet_t bullets[128];
int current_bullet_index = 0;

const float ASTEROID_BASE_RADIUS = 15;
const float ASTEROID_BASE_SPEED = 5;
asteroid_t asteroids[128];
int current_asteroid_index = 0;

const int PARTICLE_COUNT = 32;
const int PARTICLE_START_SPEED = 10;
const int PARTICLE_START_SIZE = 5;
const int MAX_PARTICLE_GROUPS = 32;
particle_group_t particle_groups[32];
int current_particle_index = 0;

const int BLINK_LENGTH = 200;
const int BLINK_COOLDOWN = 5;

const int BULLET_TIME_DURATION = 7;
float global_speed_multiplier = 1;

float kill_cooldown = 10;
float kill_timer;

int points = 0;
bool mouse_controls = true;

gamestate_e gamestate = GS_RUNNING;

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

void move_player_strafe(player_t *player)
{
	static int speed = 5;
	player->pos.x += player->vel.x * speed;
	player->pos.y += player->vel.y * speed;
}

void blink_player_dir(player_t *player)
{
	Vector2 dir = get_dir_vector(player->angle);

	player->pos.x += dir.x * BLINK_LENGTH;
	player->pos.y += dir.y * BLINK_LENGTH;
}

void blink_player_vel(player_t *player)
{
	player->pos.x += player->vel.x * BLINK_LENGTH;
	player->pos.y += player->vel.y * BLINK_LENGTH;
}

// DIR = -1 === CTR CLOCKWISE, DIR = 1 === CLOCKWISE
void rotate_player(player_t *player, int dir)
{
	static float rotation_speed = 0.07;

	player->angle += dir * rotation_speed;
}

void add_powerup(player_t *player, powerup_e powerup)
{
	player->active_powerup = powerup;

	if (powerup == PW_BULLET_TIME)
	{
		player->powerup_timer = BULLET_TIME_DURATION;
		global_speed_multiplier = 0.3;
	}
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

void update_bullets(void)
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

void draw_bullets(void)
{
	for (int i = 0; i < current_bullet_index; i++)
	{
		DrawCircleV(bullets[i].pos, BULLET_RADIUS, RED);
	}
}

void create_particle_group(Vector2 origin)
{
	clock_t start = clock();

	particle_group_t group = (particle_group_t){
		.origin = origin,
		.particles = calloc(PARTICLE_COUNT, sizeof(particle_t)),
	};

	for (int i = 0; i < PARTICLE_COUNT; i++)
	{
		float angle = (rand() % 360) * DEG2RAD;

		group.particles[i] = (particle_t){
			.pos = origin,
			.dir = get_dir_vector(angle),
			.size = PARTICLE_START_SIZE,
			.speed = PARTICLE_START_SPEED,
		};
	}

	particle_groups[current_particle_index] = group;
	current_particle_index++;

	current_particle_index = current_particle_index % MAX_PARTICLE_GROUPS;

	clock_t end = clock();

	printf("%f\n", ((double)(end - start)) / CLOCKS_PER_SEC);
}

void remove_particle_group(int index)
{
	free(particle_groups[index].particles);
	current_particle_index--;
	particle_groups[index] = particle_groups[current_particle_index];
}

void update_particles(void)
{
	particle_t *particle;

	for (int i = 0; i < current_particle_index; i++)
	{
		for (int j = 0; j < PARTICLE_COUNT; j++)
		{
			particle = &particle_groups[i].particles[j];

			particle->speed *= 0.8 + (0.2 * (1.0 - global_speed_multiplier));

			particle->size -= 0.3 * global_speed_multiplier;

			particle->pos.x += particle->dir.x * particle->speed * global_speed_multiplier;
			particle->pos.y += particle->dir.y * particle->speed * global_speed_multiplier;

			if (j == PARTICLE_COUNT - 1 && particle->size <= 0.5)
			{
				remove_particle_group(i);
			}
		}
	}
}

void draw_particles(void)
{
	particle_t particle = {0};

	for (int i = 0; i < current_particle_index; i++)
	{
		for (int j = 0; j < PARTICLE_COUNT; j++)
		{
			particle = particle_groups[i].particles[j];

			DrawCircleV(particle.pos, particle.size, WHITE);
		}
	}
}

void create_random_asteroid(Vector2 target_pos)
{
	double angle = (rand() % 360) * DEG2RAD;

	float size = ASTEROID_BASE_RADIUS * (((100 + ((rand() % 150) - 50))) / (float)100);

	float speed_thing = ASTEROID_BASE_RADIUS * ASTEROID_BASE_SPEED;

	float point_thing = ASTEROID_BASE_RADIUS * 100;

	asteroid_t asteroid = {
		.pos = {WIDTH / 2 + cos(angle) * WIDTH, HEIGHT / 2 + sin(angle) * HEIGHT},
		.is_on_screen = false,
		.size = size,
		.speed = speed_thing / size,
		.point_value = point_thing / size,
		.powerup = rand() % 30 == 3 ? PW_BULLET_TIME : PW_NONE,
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

void update_asteroids(player_t *player)
{
	for (int i = 0; i < current_asteroid_index; i++)
	{
		asteroids[i].pos.x += asteroids[i].dir.x * asteroids[i].speed * global_speed_multiplier;
		asteroids[i].pos.y += asteroids[i].dir.y * asteroids[i].speed * global_speed_multiplier;

		if (CheckCollisionCircleRec(asteroids[i].pos, asteroids[i].size, (Rectangle){.x = player->pos.x, .y = player->pos.y, .width = player->w, .height = player->h}))
		{
			gamestate = GS_GAMEOVER;
		}

		for (int j = 0; j < current_bullet_index; j++)
		{
			if (CheckCollisionCircles(asteroids[i].pos, asteroids[i].size, bullets[j].pos, BULLET_RADIUS))
			{
				points += asteroids[i].point_value;
				kill_timer = kill_cooldown;
				if (player->active_powerup != PW_BULLET_TIME)
				{
					kill_cooldown *= 0.95;
				}
				if (asteroids[i].powerup == PW_BULLET_TIME)
				{
					add_powerup(player, PW_BULLET_TIME);
				}
				create_particle_group(asteroids[i].pos);
				remove_asteroid(i);
				remove_bullet(j);
			}
		}

		// IF OUTSIDE OF SCREEN
		if (asteroids[i].pos.x + asteroids[i].size < 0 || asteroids[i].pos.x - asteroids[i].size > WIDTH || asteroids[i].pos.y + asteroids[i].size < 0 || asteroids[i].pos.y - asteroids[i].size > HEIGHT)
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

void draw_asteroids(void)
{
	for (int i = 0; i < current_asteroid_index; i++)
	{
		DrawCircleV(asteroids[i].pos, asteroids[i].size, asteroids[i].powerup == PW_BULLET_TIME ? YELLOW : GREEN);
	}
}

void update_mouse_controls(player_t *player)
{
	Vector2 mouse_pos = GetMousePosition();

	Vector2 pc = get_center_vector(player->pos, player->h, player->w);

	Vector2 dir = get_inbetween_dir_vector(pc, mouse_pos);

	player->angle = atan2(dir.y, dir.x);
}

int main(void)
{
	srand(time(NULL));

	player_t player = (player_t){
		.pos = {300, 300},
		.w = 20,
		.h = 20,
		.angle = 0,
		.active_powerup = PW_NONE,
		.powerup_timer = 0,
	};

	float asteroid_spawn_timer = 0;
	float blink_timer = 0;
	kill_timer = kill_cooldown;

	int kill_bar_max_length = 300;
	int kill_bar_half_length;

	bool gameover_first_frame = true;

	InitWindow(WIDTH, HEIGHT, "Asteroids");
	SetTargetFPS(60);

	while (!WindowShouldClose())
	{
		switch (gamestate)
		{
		case GS_RUNNING:
		{
			// UPDATE

			// TIMERS
			if (player.active_powerup != PW_NONE)
			{
				if (player.powerup_timer <= 0)
				{
					player.active_powerup = PW_NONE;
					global_speed_multiplier = 1;
				}
				{
					player.powerup_timer -= GetFrameTime();
				}
			}

			if (kill_timer <= 0)
			{
				gamestate = GS_GAMEOVER;
			}
			else if (player.active_powerup != PW_BULLET_TIME)
			{
				kill_timer -= GetFrameTime();
			}

			if (asteroid_spawn_timer > 0.5 * (1 / global_speed_multiplier))
			{
				create_random_asteroid(player.pos);
				asteroid_spawn_timer = 0;
			}
			else
			{
				asteroid_spawn_timer += GetFrameTime();
			}

			if (blink_timer > 0)
			{
				blink_timer -= GetFrameTime();
			}

			// Reset player velocity
			player.vel = (Vector2){0, 0};

			// Player Movement
			if (IsKeyDown(KEY_A))
			{
				if (mouse_controls)
				{
					player.vel.x -= 1;
				}
				else
				{
					rotate_player(&player, -1);
				}
			}
			if (IsKeyDown(KEY_D))
			{
				if (mouse_controls)
				{
					player.vel.x += 1;
				}
				else
				{
					rotate_player(&player, 1);
				}
			}
			if (IsKeyDown(KEY_W))
			{
				if (mouse_controls)
				{
					player.vel.y -= 1;
				}
				else
				{
					move_player_forward(&player, 1);
				}
			}
			if (IsKeyDown(KEY_S))
			{
				if (mouse_controls)
				{
					player.vel.y += 1;
				}
				else
				{
					move_player_forward(&player, -1);
				}
			}

			if (mouse_controls)
			{
				update_mouse_controls(&player);
				move_player_strafe(&player);
			}

			// Shoot
			if (IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(0))
			{
				create_bullet(player);
			}

			// Blink
			if (IsKeyPressed(KEY_E) || IsMouseButtonPressed(1))
			{
				if (blink_timer <= 0)
				{
					blink_player_dir(&player);
					// blink_player_vel(&player);

					blink_timer = BLINK_COOLDOWN;
				}
			}

			// Update Positions and collision
			update_bullets();

			update_asteroids(&player);

			update_particles();

			// DRAW
			BeginDrawing();
			ClearBackground(BLACK);

			draw_player(player);
			draw_particles();
			draw_asteroids();
			draw_bullets();

			DrawText(TextFormat("%d", points), 10, 10, 64, WHITE);
			DrawText("Blink:", 10, 70, 32, RED);
			DrawRectangle(10, 110, blink_timer <= 0 ? 200 : 200 - 200 * (blink_timer / BLINK_COOLDOWN), 20, RED);
			DrawRectangleLinesEx((Rectangle){10, 110, 200, 20}, 1, WHITE);

			kill_bar_half_length = (kill_bar_max_length / 2) * (kill_timer / kill_cooldown);
			DrawRectangle((WIDTH / 2) - kill_bar_half_length, 0, kill_bar_half_length, 15, GREEN);
			DrawRectangle(WIDTH / 2, 0, kill_bar_half_length, 15, GREEN);

			// if (current_particle_index > 0)
			// {
			// 	DrawText(TextFormat("%f, %f", particle_groups[current_particle_index - 1].particles[0].pos.x, particle_groups[current_particle_index - 1].particles[0].pos.y), 10, 700, 32, RED);
			// }

			// DrawText(TextFormat("%d", current_particle_index), 700, 0, 32, WHITE);

			EndDrawing();
		}
		break;

		case GS_GAMEOVER:
		{
			if (gameover_first_frame)
			{
				// CLEAR VISUAL BUFFER
				// NOTE(@albin): Hack solution to avoid flickering.
				BeginDrawing();
				ClearBackground(BLACK);
				draw_player(player);
				draw_asteroids();
				draw_bullets();
				EndDrawing();

				BeginDrawing();
				ClearBackground(BLACK);
				draw_player(player);
				draw_asteroids();
				draw_bullets();
				EndDrawing();

				gameover_first_frame = false;
			}

			BeginDrawing();

			DrawText(TextFormat("%d", points), WIDTH / 2 - (MeasureText(TextFormat("%d", points), 128) / 2), HEIGHT / 2 - 80, 128, BLUE);

			EndDrawing();
		}
		break;
		case GS_NONE:
			break;
		}
	}

	// FREE MEMORY
	for (int i = 0; i < current_particle_index; i++)
	{
		free(particle_groups[i].particles);
		particle_groups[i].particles = NULL;
	}

	CloseWindow();
	return 0;
}