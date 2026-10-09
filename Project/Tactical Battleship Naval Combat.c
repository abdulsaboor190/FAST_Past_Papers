#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <conio.h>
#include <ctype.h>
#include <time.h>

#define SIZE 10
#define SHIPS 5
#define RADARS 2

int word_select = 0;
int rnd_index(int i);

char ship_symbols[SHIPS+1] = "CBRSD";
char ship_names[SHIPS][15] = {"Carrier", "Battleship", "Cruiser", "Submarine", "Destroyer"};
int ship_sizes[SHIPS] = {5, 4, 3, 3, 2};

//Stack of cells the AI wants to try next after it lands a hit
int target_row[SIZE*SIZE];
int target_col[SIZE*SIZE];
int target_count = 0;

void init_grid(char grid[][SIZE])
{
	int i, j;
	for ( i = 0; i < SIZE; i++)
	{
		for ( j = 0; j < SIZE; j++)
		{
			grid[i][j] = '~';
		}
	}
}

//dir 0 = horizontal, dir 1 = vertical
int can_place(char fleet[][SIZE], int row, int col, int dir, int size)
{
	int i, r, c;
	for ( i = 0; i < size; i++)
	{
		if(dir == 0)
		{
			r = row;
			c = col + i;
		}
		else
		{
			r = row + i;
			c = col;
		}

		if(r >= SIZE || c >= SIZE)
		{
			return 0;
		}
		if(fleet[r][c] != '~')
		{
			return 0;
		}
	}
	return 1;
}

void put_ship(char fleet[][SIZE], int s, int row, int col, int dir)
{
	int i;
	for ( i = 0; i < ship_sizes[s]; i++)
	{
		if(dir == 0)
		{
			fleet[row][col + i] = ship_symbols[s];
		}
		else
		{
			fleet[row + i][col] = ship_symbols[s];
		}
	}
}

//Randomly drops every ship on the grid without overlapping
void place_ships(char fleet[][SIZE])
{
	int s;
	int row, col, dir;

	for ( s = 0; s < SHIPS; s++)
	{
		do
		{
			row = rnd_index(SIZE);
			col = rnd_index(SIZE);
			dir = rnd_index(2);
		} while(can_place(fleet, row, col, dir, ship_sizes[s]) == 0);

		put_ship(fleet, s, row, col, dir);
	}
}

void print_fleet(char fleet[][SIZE])
{
	int i, j;

	printf("\n\t   ");
	for ( j = 0; j < SIZE; j++)
	{
		printf("%-2d", j + 1);
	}
	printf("\n");

	for ( i = 0; i < SIZE; i++)
	{
		printf("\t%c  ", 'A' + i);
		for ( j = 0; j < SIZE; j++)
		{
			printf("%c ", fleet[i][j]);
		}
		printf("\n");
	}
}

//Converts text like B7 into row and column, returns 1 if it is inside the grid
int parse_coordinate(char* text, int* row, int* col)
{
	char rowch;

	if(sscanf(text, "%c%d", &rowch, col) != 2)
	{
		return 0;
	}

	*row = toupper(rowch) - 'A';
	*col = *col - 1;

	if(*row < 0 || *row >= SIZE || *col < 0 || *col >= SIZE)
	{
		return 0;
	}
	return 1;
}

//Returns 0 for invalid input, 1 for a valid coordinate and 2 when the player typed R
int read_coordinate(int* row, int* col)
{
	char input[20];

	scanf("%19s", input);
	while(getchar() != '\n');

	if(strlen(input) == 1 && toupper(input[0]) == 'R')
	{
		return 2;
	}
	return parse_coordinate(input, row, col);
}

//Lets the player position every ship by hand
void manual_place(char fleet[][SIZE])
{
	char input[20];
	char dirch;
	int s, row, col, dir;
	int placed;

	for ( s = 0; s < SHIPS; s++)
	{
		placed = 0;
		while(placed == 0)
		{
			system("cls");
			print_fleet(fleet);
			printf("\n\tPlace your %s (%d cells).\n", ship_names[s], ship_sizes[s]);
			printf("\tEnter start cell and direction H or V (e.g. A1 H): ");
			scanf("%19s %c", input, &dirch);
			while(getchar() != '\n');
			dirch = toupper(dirch);

			if(parse_coordinate(input, &row, &col) == 0 || (dirch != 'H' && dirch != 'V'))
			{
				printf("\n\tInvalid input! Press any key to try again...");
				getch();
			}
			else
			{
				if(dirch == 'H')
				{
					dir = 0;
				}
				else
				{
					dir = 1;
				}

				if(can_place(fleet, row, col, dir, ship_sizes[s]) == 0)
				{
					printf("\n\tThe ship does not fit there! Press any key to try again...");
					getch();
				}
				else
				{
					put_ship(fleet, s, row, col, dir);
					placed = 1;
				}
			}
		}
	}
}

//Returns 0 for a miss, 1 for a hit and 2 when the hit sinks a ship (ship index goes in *ship)
int fire(char fleet[][SIZE], char view[][SIZE], int row, int col, int hp[], int* ship)
{
	int idx;

	if(fleet[row][col] == '~')
	{
		view[row][col] = 'O';
		return 0;
	}

	view[row][col] = 'X';
	idx = strchr(ship_symbols, fleet[row][col]) - ship_symbols;
	hp[idx]--;
	*ship = idx;

	if(hp[idx] == 0)
	{
		return 2;
	}
	return 1;
}

//Counts the ship sections that are still unhit in the 3x3 area around a cell
int radar_scan(char fleet[][SIZE], char view[][SIZE], int row, int col)
{
	int i, j;
	int count = 0;

	for ( i = row - 1; i <= row + 1; i++)
	{
		for ( j = col - 1; j <= col + 1; j++)
		{
			if(i >= 0 && i < SIZE && j >= 0 && j < SIZE)
			{
				if(fleet[i][j] != '~' && view[i][j] != 'X')
				{
					count++;
				}
			}
		}
	}
	return count;
}

int count_afloat(int hp[])
{
	int i, count = 0;
	for ( i = 0; i < SHIPS; i++)
	{
		if(hp[i] > 0)
		{
			count++;
		}
	}
	return count;
}

void display_boards(char player_fleet[][SIZE], char enemy_view[][SIZE], char player_view[][SIZE], int player_hp[], int enemy_hp[], int radar_left)
{
	int i, j;
	char cell;

	printf("\n\t======== TACTICAL BATTLESHIP ========\n");
	printf("\tX = hit   O = miss   ~ = water\n\n");
	printf("\t      YOUR FLEET                    ENEMY WATERS\n");

	printf("\t   ");
	for ( j = 0; j < SIZE; j++)
	{
		printf("%-2d", j + 1);
	}
	printf("        ");
	for ( j = 0; j < SIZE; j++)
	{
		printf("%-2d", j + 1);
	}
	printf("\n");

	for ( i = 0; i < SIZE; i++)
	{
		printf("\t%c  ", 'A' + i);
		for ( j = 0; j < SIZE; j++)
		{
			cell = player_fleet[i][j];
			if(enemy_view[i][j] != '~')
			{
				cell = enemy_view[i][j];
			}
			printf("%c ", cell);
		}

		printf("      %c  ", 'A' + i);
		for ( j = 0; j < SIZE; j++)
		{
			printf("%c ", player_view[i][j]);
		}
		printf("\n");
	}

	printf("\n\tYour ships afloat : %d\tEnemy ships afloat : %d\tRadar charges : %d\n\n", count_afloat(player_hp), count_afloat(enemy_hp), radar_left);
}

//One player turn: either fire at a coordinate or spend a radar charge
void player_turn(char enemy_fleet[][SIZE], char player_view[][SIZE], int enemy_hp[], int* shots, int* hits, int* radar_left)
{
	int row, col, result, ship, status;
	int done = 0;

	while(done == 0)
	{
		printf("\tEnter target (e.g. B7)");
		if(*radar_left > 0)
		{
			printf(" or R for radar (%d left)", *radar_left);
		}
		printf(": ");
		status = read_coordinate(&row, &col);

		if(status == 0)
		{
			printf("\tInvalid coordinate!\n");
		}
		else if(status == 2)
		{
			if(*radar_left == 0)
			{
				printf("\tNo radar charges left!\n");
			}
			else
			{
				do
				{
					printf("\tEnter radar center (e.g. B7): ");
				} while(read_coordinate(&row, &col) != 1);

				result = radar_scan(enemy_fleet, player_view, row, col);
				(*radar_left)--;
				printf("\n\tRadar sweep around %c%d: %d enemy ship section(s) in the 3x3 area.\n", 'A' + row, col + 1, result);
				done = 1;
			}
		}
		else if(player_view[row][col] != '~')
		{
			printf("\tYou already fired there!\n");
		}
		else
		{
			result = fire(enemy_fleet, player_view, row, col, enemy_hp, &ship);
			(*shots)++;

			printf("\n\tYou fired at %c%d: ", 'A' + row, col + 1);
			if(result == 0)
			{
				printf("MISS.\n");
			}
			else if(result == 1)
			{
				printf("HIT!\n");
				(*hits)++;
			}
			else
			{
				printf("HIT! You sank the enemy %s!\n", ship_names[ship]);
				(*hits)++;
			}
			done = 1;
		}
	}
}

void add_target(char enemy_view[][SIZE], int row, int col)
{
	if(row >= 0 && row < SIZE && col >= 0 && col < SIZE && enemy_view[row][col] == '~')
	{
		target_row[target_count] = row;
		target_col[target_count] = col;
		target_count++;
	}
}

//level 1 shoots randomly, level 2 hunts around its hits
void ai_turn(char player_fleet[][SIZE], char enemy_view[][SIZE], int player_hp[], int level)
{
	int row = 0, col = 0;
	int result, ship;
	int valid = 0;

	while(level == 2 && target_count > 0 && valid == 0)
	{
		target_count--;
		row = target_row[target_count];
		col = target_col[target_count];
		if(enemy_view[row][col] == '~')
		{
			valid = 1;
		}
	}

	if(valid == 0)
	{
		do
		{
			row = rnd_index(SIZE);
			col = rnd_index(SIZE);
		} while(enemy_view[row][col] != '~');
	}

	result = fire(player_fleet, enemy_view, row, col, player_hp, &ship);

	printf("\tEnemy fired at %c%d: ", 'A' + row, col + 1);
	if(result == 0)
	{
		printf("MISS.\n");
	}
	else if(result == 1)
	{
		printf("HIT on your ship!\n");
		if(level == 2)
		{
			add_target(enemy_view, row - 1, col);
			add_target(enemy_view, row + 1, col);
			add_target(enemy_view, row, col - 1);
			add_target(enemy_view, row, col + 1);
		}
	}
	else
	{
		printf("HIT! Your %s has been sunk!\n", ship_names[ship]);
	}
}

int load_highscore()
{
	FILE *fp = fopen("battleship_score.txt", "r");
	int best = 0;

	if(fp == NULL)
	{
		return 0;
	}
	fscanf(fp, "%d", &best);
	fclose(fp);
	return best;
}

void save_highscore(int best)
{
	FILE *fp = fopen("battleship_score.txt", "w");

	if(fp == NULL)
	{
		return;
	}
	fprintf(fp, "%d", best);
	fclose(fp);
}

//Plays one full battle and returns the points earned (0 means the battle was lost)
int play_game(int level, int manual)
{
	char player_fleet[SIZE][SIZE];
	char enemy_fleet[SIZE][SIZE];
	char player_view[SIZE][SIZE];
	char enemy_view[SIZE][SIZE];
	int player_hp[SHIPS];
	int enemy_hp[SHIPS];
	int shots = 0, hits = 0;
	int radar_left = RADARS;
	int i, j;
	int points = 0;

	init_grid(player_fleet);
	init_grid(enemy_fleet);
	init_grid(player_view);
	init_grid(enemy_view);
	target_count = 0;

	for ( i = 0; i < SHIPS; i++)
	{
		player_hp[i] = ship_sizes[i];
		enemy_hp[i] = ship_sizes[i];
	}

	place_ships(enemy_fleet);
	if(manual == 1)
	{
		manual_place(player_fleet);
	}
	else
	{
		place_ships(player_fleet);
	}

	while(count_afloat(player_hp) > 0 && count_afloat(enemy_hp) > 0)
	{
		system("cls");
		display_boards(player_fleet, enemy_view, player_view, player_hp, enemy_hp, radar_left);

		player_turn(enemy_fleet, player_view, enemy_hp, &shots, &hits, &radar_left);

		if(count_afloat(enemy_hp) > 0)
		{
			ai_turn(player_fleet, enemy_view, player_hp, level);
		}

		printf("\n\tPress any key to continue...");
		getch();
	}

	system("cls");
	display_boards(player_fleet, enemy_view, player_view, player_hp, enemy_hp, radar_left);

	if(count_afloat(enemy_hp) == 0)
	{
		points = 500 - shots * 3 + count_afloat(player_hp) * 50;
		if(level == 2)
		{
			points = points * 2;
		}
		printf("\tVICTORY! You destroyed the enemy fleet!\n");
		printf("\tPoints earned : %d\n", points);
	}
	else
	{
		printf("\tDEFEAT! Your fleet has been destroyed.\n\n\tEnemy fleet positions:\n");
		print_fleet(enemy_fleet);
	}

	printf("\n\tShots fired : %d\n\tHits        : %d\n", shots, hits);
	if(shots > 0)
	{
		printf("\tAccuracy    : %.1f%%\n", (float)hits / shots * 100);
	}
	return points;
}

void rules()
{
	printf("\n\t Rules : ");
	printf("\n\t - Both fleets are hidden on a %dx%d grid. The enemy fleet is placed randomly.", SIZE, SIZE);
	printf("\n\t - Your fleet can be placed randomly or by hand (cell and H/V direction, e.g. A1 H).");
	printf("\n\t - Fleet: Carrier (5), Battleship (4), Cruiser (3), Submarine (3), Destroyer (2).");
	printf("\n\t - Each turn you fire at one coordinate, then the enemy fires at yours.");
	printf("\n\t - X = hit, O = miss, ~ = unknown water.");
	printf("\n\t - Type R instead of a target to use radar (%d charges per battle). It scans a 3x3", RADARS);
	printf("\n\t   area and tells you how many unhit ship sections are inside. It uses up your turn.");
	printf("\n\t - Easy enemy fires randomly, Hard enemy hunts around its hits.");
	printf("\n\t - Win points = 500 - 3 per shot + 50 per surviving ship, doubled on Hard.\n\n");
}

int main()
{
	int choice, level, manual;
	int games = 0, wins = 0, losses = 0;
	int points, session_score = 0;
	int high_score = load_highscore();

	do
	{
		printf("\n******Welcome to Tactical Battleship Naval Combat*****\n");
		printf("\n1. PLAY\n2. RULES\n3. STATS\n4. EXIT\n\n");
		scanf("%d", &choice);

		switch(choice)
		{
			case 1:				//Play
			{
				printf("\n\tSelect Enemy AI level:\n");
				printf("\n1.Easy (random shots)\t2.Hard (hunts hits)\n\n");
				scanf("%d", &level);

				if(level != 1 && level != 2)
				{
					printf("\n\tInvalid level! Easy selected.\n");
					level = 1;
				}

				printf("\n\tSelect fleet placement:\n");
				printf("\n1.Automatic\t2.Manual\n\n");
				scanf("%d", &manual);

				if(manual == 2)
				{
					manual = 1;
				}
				else
				{
					manual = 0;
				}

				games++;
				points = play_game(level, manual);

				if(points > 0)
				{
					wins++;
					session_score = session_score + points;
					if(points > high_score)
					{
						high_score = points;
						save_highscore(high_score);
						printf("\tNEW HIGH SCORE: %d\n", high_score);
					}
				}
				else
				{
					losses++;
				}
				break;
			}
			case 2:				//Rules
			{
				rules();
				break;
			}
			case 3:				//Stats
			{
				printf("\n\t===== STATS =====");
				printf("\n\tBattles fought : %d", games);
				printf("\n\tVictories      : %d", wins);
				printf("\n\tDefeats        : %d", losses);
				printf("\n\tSession score  : %d", session_score);
				printf("\n\tBest battle    : %d\n\n", high_score);
				break;
			}
			case 4:				//Exit
			{
				printf("\n\tGoodbye, Admiral!\n");
				break;
			}
			default:
			{
				printf("\n\tInvalid choice!\n");
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
