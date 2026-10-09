#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <conio.h>
#include <ctype.h>
#include <time.h>
#include <windows.h>

#define WIDTH 30			//playfield cells (each cell is 2 console columns wide)
#define HEIGHT 20
#define MAX_LEN (WIDTH * HEIGHT)
#define MAX_SCORES 5
#define BONUS_TICKS 40

//Where the playfield sits on the screen
#define OFFX 2
#define OFFY 3
#define MSG_ROW (OFFY + HEIGHT + 3)

//Board cell contents
#define B_EMPTY 0
#define B_BODY 1
#define B_FOOD 2
#define B_BONUS 3

//Console colors
#define C_DKGREEN 2
#define C_GREEN 10
#define C_CYAN 11
#define C_RED 12
#define C_YELLOW 14
#define C_WHITE 15
#define C_GRAY 8

struct Point
{
	int x;
	int y;
};

struct Snake
{
	struct Point body[MAX_LEN];		//body[0] is the head
	int length;
	int dir;						//0 = up, 1 = right, 2 = down, 3 = left
};

struct Food
{
	struct Point pos;
	int active;
	int ticks;						//time left (used by the bonus food)
};

struct Score
{
	char name[20];
	int score;
	int level;
};

int word_select = 0;
int rnd_index(int i);

char level_names[3][10] = {"Easy", "Medium", "Hard"};

//---------------------------------------- console helpers ----------------------------------------

void move_to(int x, int y)
{
	COORD pos;
	pos.X = x;
	pos.Y = y;
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
}

void paint(int attribute)
{
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), attribute);
}

void show_cursor(int visible)
{
	CONSOLE_CURSOR_INFO info;
	info.dwSize = 20;
	info.bVisible = visible;
	SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);
}

//Keeps asking until the user enters a whole number between min and max
int read_int(char *prompt, int min, int max)
{
	int value;

	while(1)
	{
		printf("%s", prompt);
		if(scanf("%d", &value) != 1)
		{
			while(getchar() != '\n');
			printf("\tInvalid input!\n");
		}
		else
		{
			while(getchar() != '\n');
			if(value < min || value > max)
			{
				printf("\tEnter a value from %d to %d!\n", min, max);
			}
			else
			{
				return value;
			}
		}
	}
}

//---------------------------------------- drawing ----------------------------------------

//Every playfield cell is drawn as two characters so the game looks square
void draw_cell(int x, int y, char *text, int color)
{
	move_to(OFFX + 1 + x * 2, OFFY + 1 + y);
	paint(color);
	printf("%s", text);
}

void draw_border()
{
	int i;

	paint(C_CYAN);
	move_to(OFFX, OFFY);
	for ( i = 0; i < WIDTH * 2 + 2; i++)
	{
		printf("#");
	}
	move_to(OFFX, OFFY + HEIGHT + 1);
	for ( i = 0; i < WIDTH * 2 + 2; i++)
	{
		printf("#");
	}
	for ( i = 1; i <= HEIGHT; i++)
	{
		move_to(OFFX, OFFY + i);
		printf("#");
		move_to(OFFX + WIDTH * 2 + 1, OFFY + i);
		printf("#");
	}
	paint(C_WHITE);
}

void show_status(int score, int length, int speed, int high_score, int level, int wrap)
{
	char line[100];

	move_to(OFFX, 0);
	paint(C_YELLOW);
	sprintf(line, "Score: %-6d Length: %-4d Speed: %-3d High score: %-6d", score, length, speed, high_score);
	printf("%-70s", line);

	move_to(OFFX, 1);
	paint(C_GRAY);
	sprintf(line, "%s | %s | Arrows/WASD move, P pause, Q quit", level_names[level - 1], wrap == 1 ? "Wrap-around" : "Classic walls");
	printf("%-70s", line);
	paint(C_WHITE);
}

void show_message(char *text, int color)
{
	move_to(OFFX, MSG_ROW);
	paint(color);
	printf("%-70s", text);
	paint(C_WHITE);
}

//---------------------------------------- game setup ----------------------------------------

//Clears the board and puts a 3 cell snake in the middle heading right
void init_game(struct Snake *s, char board[][WIDTH])
{
	int x, y, i;

	for ( y = 0; y < HEIGHT; y++)
	{
		for ( x = 0; x < WIDTH; x++)
		{
			board[y][x] = B_EMPTY;
		}
	}

	s->length = 3;
	s->dir = 1;
	for ( i = 0; i < s->length; i++)
	{
		(s->body + i)->x = WIDTH / 2 - i;
		(s->body + i)->y = HEIGHT / 2;
		board[(s->body + i)->y][(s->body + i)->x] = B_BODY;
	}
}

//Drops food on a random empty cell (callers make sure an empty cell exists)
void place_food(char board[][WIDTH], struct Food *f, int type)
{
	int x, y;

	do
	{
		x = rnd_index(WIDTH);
		y = rnd_index(HEIGHT);
	} while(board[y][x] != B_EMPTY);

	f->pos.x = x;
	f->pos.y = y;
	f->active = 1;
	f->ticks = BONUS_TICKS;
	board[y][x] = type;

	if(type == B_FOOD)
	{
		draw_cell(x, y, "()", C_RED);
	}
	else
	{
		draw_cell(x, y, "$$", C_YELLOW);
	}
}

//---------------------------------------- the game ----------------------------------------

//Plays one game and returns the final score
int play_game(int level, int wrap, int high_score)
{
	struct Snake snake;
	struct Food food, bonus;
	struct Point tail;
	char board[HEIGHT][WIDTH];
	char reason[80] = "";
	char line[100];
	int dx[4] = {0, 1, 0, -1};
	int dy[4] = {-1, 0, 1, 0};
	int start_delay[3] = {150, 100, 60};
	int delay = start_delay[level - 1];
	int score = 0, eaten = 0;
	int alive = 1, eating, paused;
	int pending, cand, key, nx, ny, i, cell;

	init_game(&snake, board);
	food.active = 0;
	bonus.active = 0;
	bonus.ticks = 0;
	pending = snake.dir;

	system("cls");
	show_cursor(0);
	draw_border();

	for ( i = 0; i < snake.length; i++)
	{
		if(i == 0)
		{
			draw_cell((snake.body + i)->x, (snake.body + i)->y, "\xDB\xDB", C_GREEN);
		}
		else
		{
			draw_cell((snake.body + i)->x, (snake.body + i)->y, "\xDB\xDB", C_DKGREEN);
		}
	}
	place_food(board, &food, B_FOOD);

	while(alive == 1)
	{
		show_status(score, snake.length, 1 + eaten / 5, high_score, level, wrap);
		Sleep(delay);

		//Read every key pressed during the delay
		while(kbhit())
		{
			key = getch();
			cand = -1;

			if(key == 0 || key == 224)
			{
				key = getch();
				if(key == 72)
				{
					cand = 0;
				}
				else if(key == 77)
				{
					cand = 1;
				}
				else if(key == 80)
				{
					cand = 2;
				}
				else if(key == 75)
				{
					cand = 3;
				}
			}
			else
			{
				key = toupper(key);
				if(key == 'W')
				{
					cand = 0;
				}
				else if(key == 'D')
				{
					cand = 1;
				}
				else if(key == 'S')
				{
					cand = 2;
				}
				else if(key == 'A')
				{
					cand = 3;
				}
				else if(key == 'P')
				{
					paused = 1;
					show_message("PAUSED - press P to continue", C_YELLOW);
					while(paused == 1)
					{
						if(kbhit() && toupper(getch()) == 'P')
						{
							paused = 0;
						}
						Sleep(50);
					}
					show_message("", C_WHITE);
				}
				else if(key == 'Q' || key == 27)
				{
					alive = 0;
					strcpy(reason, "You ended the game.");
				}
			}

			//The snake can never turn straight back into itself
			if(cand != -1 && cand != (snake.dir + 2) % 4)
			{
				pending = cand;
			}
		}

		if(alive == 0)
		{
			break;
		}

		snake.dir = pending;
		nx = snake.body[0].x + dx[snake.dir];
		ny = snake.body[0].y + dy[snake.dir];

		if(wrap == 1)
		{
			nx = (nx + WIDTH) % WIDTH;
			ny = (ny + HEIGHT) % HEIGHT;
		}
		else if(nx < 0 || nx >= WIDTH || ny < 0 || ny >= HEIGHT)
		{
			alive = 0;
			strcpy(reason, "You crashed into the wall!");
		}

		if(alive == 1)
		{
			tail = snake.body[snake.length - 1];
			cell = board[ny][nx];
			eating = (cell == B_FOOD || cell == B_BONUS);

			//The tail moves away unless the snake is growing
			if(eating == 0)
			{
				board[tail.y][tail.x] = B_EMPTY;
				draw_cell(tail.x, tail.y, "  ", C_WHITE);
			}

			if(board[ny][nx] == B_BODY)
			{
				alive = 0;
				strcpy(reason, "You bit yourself!");
			}
		}

		if(alive == 1)
		{
			//Old head becomes body, then every segment follows the one before it
			draw_cell(snake.body[0].x, snake.body[0].y, "\xDB\xDB", C_DKGREEN);
			for ( i = snake.length - 1; i > 0; i--)
			{
				snake.body[i] = snake.body[i - 1];
			}
			snake.body[0].x = nx;
			snake.body[0].y = ny;

			if(eating == 1)
			{
				snake.body[snake.length] = tail;
				snake.length++;
			}

			board[ny][nx] = B_BODY;
			draw_cell(nx, ny, "\xDB\xDB", C_GREEN);

			if(cell == B_FOOD)
			{
				score = score + 10 * level;
				eaten++;
				food.active = 0;

				//Every 5th food: the snake speeds up and a timed bonus appears
				if(eaten % 5 == 0)
				{
					delay = delay - 8;
					if(delay < 40)
					{
						delay = 40;
					}
					if(bonus.active == 0 && snake.length + 4 < MAX_LEN)
					{
						place_food(board, &bonus, B_BONUS);
					}
				}

				if(snake.length + 3 < MAX_LEN)
				{
					place_food(board, &food, B_FOOD);
				}
			}
			else if(cell == B_BONUS)
			{
				score = score + 50 * level;
				bonus.active = 0;
			}

			if(snake.length + 3 >= MAX_LEN)
			{
				alive = 0;
				strcpy(reason, "You filled the board! PERFECT GAME!");
			}
		}

		//Bonus food disappears when its timer runs out
		if(alive == 1 && bonus.active == 1)
		{
			bonus.ticks--;
			if(bonus.ticks <= 0)
			{
				bonus.active = 0;
				board[bonus.pos.y][bonus.pos.x] = B_EMPTY;
				draw_cell(bonus.pos.x, bonus.pos.y, "  ", C_WHITE);
			}
		}
	}

	show_status(score, snake.length, 1 + eaten / 5, high_score, level, wrap);
	sprintf(line, "GAME OVER - %s", reason);
	show_message(line, C_RED);
	move_to(OFFX, MSG_ROW + 1);
	paint(C_YELLOW);
	printf("Final score: %d    Final length: %d    Food eaten: %d", score, snake.length, eaten);
	move_to(OFFX, MSG_ROW + 2);
	paint(C_WHITE);
	printf("Press any key to continue...");

	Sleep(500);
	while(kbhit())
	{
		getch();
	}
	getch();

	paint(C_WHITE);
	show_cursor(1);
	return score;
}

//---------------------------------------- high scores ----------------------------------------

void swap_scores(struct Score *a, struct Score *b)
{
	struct Score temp;
	temp = *a;
	*a = *b;
	*b = temp;
}

void sort_scores(struct Score *scores, int count)
{
	int i, j;

	for ( i = 0; i < count - 1; i++)
	{
		for ( j = 0; j < count - i - 1; j++)
		{
			if((scores + j)->score < (scores + j + 1)->score)
			{
				swap_scores(scores + j, scores + j + 1);
			}
		}
	}
}

int load_scores(struct Score *scores)
{
	FILE *fp = fopen("snake_scores.txt", "r");
	int count = 0;

	if(fp == NULL)
	{
		return 0;
	}

	while(count < MAX_SCORES && fscanf(fp, "%19s %d %d", (scores + count)->name, &(scores + count)->score, &(scores + count)->level) == 3)
	{
		count++;
	}

	fclose(fp);
	return count;
}

void save_scores(struct Score *scores, int count)
{
	FILE *fp = fopen("snake_scores.txt", "w");
	int i;

	if(fp == NULL)
	{
		printf("\n\tCould not save high scores.\n");
		return;
	}

	for ( i = 0; i < count; i++)
	{
		fprintf(fp, "%s %d %d\n", (scores + i)->name, (scores + i)->score, (scores + i)->level);
	}
	fclose(fp);
}

//A score qualifies if the table has room or it beats the lowest entry
int qualifies(struct Score *scores, int count, int score)
{
	if(score <= 0)
	{
		return 0;
	}
	if(count < MAX_SCORES)
	{
		return 1;
	}
	return score > (scores + count - 1)->score;
}

//Inserts a score and keeps only the best MAX_SCORES
void add_score(struct Score *scores, int *count, char *name, int score, int level)
{
	strcpy((scores + *count)->name, name);
	(scores + *count)->score = score;
	(scores + *count)->level = level;
	(*count)++;

	sort_scores(scores, *count);
	if(*count > MAX_SCORES)
	{
		*count = MAX_SCORES;
	}
}

void show_scores(struct Score *scores, int count)
{
	int i;

	printf("\n\t===== HIGH SCORES =====\n");
	if(count == 0)
	{
		printf("\n\tNo scores yet. Be the first!\n\n");
		return;
	}

	printf("\n\t%-5s%-20s%-10s%s\n", "No.", "Player", "Score", "Level");
	printf("\t----------------------------------------------\n");
	for ( i = 0; i < count; i++)
	{
		printf("\t%-5d%-20s%-10d%s\n", i + 1, (scores + i)->name, (scores + i)->score, level_names[(scores + i)->level - 1]);
	}
	printf("\n");
}

void how_to_play()
{
	printf("\n\t How to play : ");
	printf("\n\t - Steer the snake with the arrow keys or W A S D. It keeps moving on its own.");
	printf("\n\t - Eat the red food () to grow and score points.");
	printf("\n\t - Every 5th food the snake speeds up and a golden $$ bonus appears for a short time.");
	printf("\n\t - Bonus food is worth 5 times a normal food but vanishes quickly.");
	printf("\n\t - Classic mode: hitting a wall ends the game. Wrap-around mode: walls are open.");
	printf("\n\t - Biting your own body always ends the game.");
	printf("\n\t - Points are multiplied by the difficulty level (1 to 3).");
	printf("\n\t - P pauses the game and Q quits the round.\n\n");
}

int main()
{
	struct Score scores[MAX_SCORES + 1];
	char name[20];
	int score_count;
	int choice, level, mode, score, high_score;

	score_count = load_scores(scores);

	do
	{
		system("cls");
		show_cursor(1);
		paint(C_GREEN);
		printf("\n******Welcome to the Console Snake Game*****\n");
		paint(C_WHITE);
		printf("\n1. PLAY\n2. HIGH SCORES\n3. HOW TO PLAY\n4. EXIT\n\n");
		choice = read_int("", 1, 4);

		switch(choice)
		{
			case 1:				//Play
			{
				printf("\n\tSelect difficulty:\n");
				printf("\n1.Easy\t2.Medium\t3.Hard\n\n");
				level = read_int("", 1, 3);

				printf("\n\tSelect mode:\n");
				printf("\n1.Classic walls\t2.Wrap-around\n\n");
				mode = read_int("", 1, 2);

				high_score = 0;
				if(score_count > 0)
				{
					high_score = scores[0].score;
				}

				score = play_game(level, mode == 2, high_score);

				system("cls");
				printf("\n\tYour score: %d\n", score);

				if(qualifies(scores, score_count, score) == 1)
				{
					printf("\n\tNew high score! Enter your name (no spaces): ");
					scanf("%19s", name);
					while(getchar() != '\n');

					add_score(scores, &score_count, name, score, level);
					save_scores(scores, score_count);
					show_scores(scores, score_count);
				}
				printf("\n\tPress any key to return to the menu...");
				getch();
				break;
			}
			case 2:				//High scores
			{
				show_scores(scores, score_count);
				printf("\tPress any key to return to the menu...");
				getch();
				break;
			}
			case 3:				//How to play
			{
				how_to_play();
				printf("\tPress any key to return to the menu...");
				getch();
				break;
			}
			case 4:				//Exit
			{
				printf("\n\tThanks for playing. Goodbye!\n");
				break;
			}
		}
	} while(choice != 4);

	return 0;
}

int rnd_index(int i)
{
	if (word_select == 0)
	{
		srand(time(NULL) << 5);
		word_select = 1;
	}

	return rand() % i;
}
