#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <conio.h>

#define PEGS 3				//A, B and C
#define MAX_DISKS 8			//Biggest tower the game allows

int peg[PEGS][MAX_DISKS];		//Each peg is a stack of disk sizes (1 = smallest)
int height[PEGS];			//How many disks are on each peg
int disks = 3;				//Disks in the current game
int move_count = 0;			//Moves made in the current game
int stepping = 0;			//1 = show every solver move on the screen
FILE* logfile = NULL;			//Move log (hanoi_log.txt)

//------------------------------------------------------------------------------------------------------
//												STACK FUNCTIONS
//------------------------------------------------------------------------------------------------------

void push(int p, int d)
{
	peg[p][height[p]] = d;
	height[p]++;
}

int pop(int p)
{
	height[p]--;
	return peg[p][height[p]];
}

//Size of the top disk on a peg, 0 if the peg is empty
int top_disk(int p)
{
	if (height[p] == 0) return 0;
	return peg[p][height[p] - 1];
}

void reset_game(int n)
{
	int p, d;

	disks = n;
	move_count = 0;

	for (p = 0; p < PEGS; p++) height[p] = 0;
	for (d = n; d >= 1; d--) push(0, d);		//Biggest disk goes in first, so it ends up at the bottom
}

//------------------------------------------------------------------------------------------------------
//													DISPLAY
//------------------------------------------------------------------------------------------------------

//Draws one slice of a peg: a disk of size d, or just the rod if d is 0
void draw_disk(int d)
{
	int i, pad = disks - d;

	for (i = 0; i < pad; i++) putchar(' ');

	if (d == 0) putchar('|');
	else
	{
		for (i = 0; i < 2 * d + 1; i++) putchar('#');
	}

	for (i = 0; i < pad; i++) putchar(' ');
}

void print_pegs()
{
	int level, p, d, i;

	printf("\n");
	for (level = disks - 1; level >= 0; level--)
	{
		printf("\t");
		for (p = 0; p < PEGS; p++)
		{
			d = 0;
			if (level < height[p]) d = peg[p][level];

			draw_disk(d);
			printf("   ");
		}
		printf("\n");
	}

	printf("\t");
	for (p = 0; p < PEGS; p++)
	{
		for (i = 0; i < 2 * disks + 1; i++) putchar('=');
		printf("   ");
	}

	printf("\n\t");
	for (p = 0; p < PEGS; p++)
	{
		for (i = 0; i < disks; i++) putchar(' ');
		printf("%c", 'A' + p);
		for (i = 0; i < disks; i++) putchar(' ');
		printf("   ");
	}
	printf("\n\n");
}

//------------------------------------------------------------------------------------------------------
//												MOVES AND RULES
//------------------------------------------------------------------------------------------------------

//A move is legal if the source has a disk and the target is empty or has a bigger disk on top
int is_valid_move(int from, int to)
{
	if (from == to) return 0;
	if (height[from] == 0) return 0;
	if (height[to] > 0 && top_disk(to) < top_disk(from)) return 0;
	return 1;
}

void move_disk(int from, int to)
{
	int d = pop(from);
	push(to, d);
	move_count++;

	if (logfile != NULL) fprintf(logfile, "Move %d: disk %d from %c to %c\n", move_count, d, 'A' + from, 'A' + to);
}

void check_result()
{
	int minimum = (1 << disks) - 1;			//2^n - 1

	printf("\tDisks             : %d\n", disks);
	printf("\tMoves made        : %d\n", move_count);
	printf("\tMinimum (2^%d - 1) : %d\n", disks, minimum);

	if (move_count == minimum) printf("\n\tPERFECT! That is the minimum number of moves.\n");
	else if (move_count > minimum) printf("\n\tSolved, but with %d extra moves.\n", move_count - minimum);
	else printf("\n\tFewer moves than the minimum? Something is wrong!\n");
}

//------------------------------------------------------------------------------------------------------
//									RECURSIVE SOLVER
//------------------------------------------------------------------------------------------------------

void show_step(int d, int from, int to)
{
	system("cls");
	printf("\n\t======== TOWER OF HANOI SOLVER ========\n");
	printf("\tMove %d: disk %d from %c to %c\n", move_count, d, 'A' + from, 'A' + to);
	print_pegs();
	printf("\tPress any key for the next move (Q = finish instantly): ");

	if (tolower(getch()) == 'q') stepping = 0;
}

/*
	Moves n disks from peg 'from' to peg 'to' using peg 'via' as the helper.
	Base case : n == 0 -> nothing to move
	Step 1    : move the top n-1 disks out of the way (from -> via)
	Step 2    : move the biggest disk (from -> to)
	Step 3    : move the n-1 disks on top of it (via -> to)
	Pegs are numbered 0 = A, 1 = B, 2 = C.
*/
void hanoi(int n, int from, int to, int via)
{
	if (n == 0) return;

	hanoi(n - 1, from, via, to);

	int d = top_disk(from);
	move_disk(from, to);
	if (stepping) show_step(d, from, to);

	hanoi(n - 1, via, to, from);
}

//------------------------------------------------------------------------------------------------------
//													GAMEPLAY
//------------------------------------------------------------------------------------------------------

int ask_disks()
{
	int n;

	printf("\n\tHow many disks (1-%d): ", MAX_DISKS);
	if (scanf("%d", &n) != 1)
	{
		while (getchar() != '\n' && !feof(stdin));
		printf("\n\tInvalid input.\n");
		return 0;
	}

	if (n < 1 || n > MAX_DISKS)
	{
		printf("\n\tThe number of disks must be from 1 to %d.\n", MAX_DISKS);
		return 0;
	}
	return n;
}

void watch_solver()
{
	int n = ask_disks();
	if (n == 0) return;

	reset_game(n);

	logfile = fopen("hanoi_log.txt", "w");
	if (logfile == NULL) printf("\n\tWarning: could not create hanoi_log.txt, moves will not be saved.\n");
	else fprintf(logfile, "Tower of Hanoi - %d disks - solved by the computer\n", n);

	system("cls");
	printf("\n\t======== TOWER OF HANOI SOLVER ========\n");
	printf("\tStarting position:\n");
	print_pegs();
	printf("\tPress any key to start the solver: ");
	getch();

	stepping = 1;
	hanoi(n, 0, 2, 1);
	stepping = 0;

	if (logfile != NULL)
	{
		fprintf(logfile, "Total moves: %d\n", move_count);
		fclose(logfile);
		logfile = NULL;
	}

	system("cls");
	printf("\n\t======== SOLVED ========\n");
	print_pegs();
	check_result();
	printf("\n\tThe move log was saved to hanoi_log.txt\n");
	printf("\tPress any key to return to the menu: ");
	getch();
}

void play_manual()
{
	int n = ask_disks();
	if (n == 0) return;

	char from_ch, to_ch;
	int from, to;
	char message[80] = "Move all the disks to peg C!";

	reset_game(n);

	logfile = fopen("hanoi_log.txt", "w");
	if (logfile == NULL) printf("\n\tWarning: could not create hanoi_log.txt, moves will not be saved.\n");
	else fprintf(logfile, "Tower of Hanoi - %d disks - played manually\n", n);

	while (height[2] != disks)
	{
		system("cls");
		printf("\n\t======== TOWER OF HANOI ========\n");
		printf("\tMoves: %d\n", move_count);
		printf("\t%s\n", message);
		print_pegs();

		printf("\tMove FROM which peg (A/B/C, Q = quit): ");
		from_ch = toupper(getch());
		printf("%c\n", from_ch);

		if (from_ch == 'Q')
		{
			if (logfile != NULL)
			{
				fprintf(logfile, "Game abandoned after %d moves\n", move_count);
				fclose(logfile);
				logfile = NULL;
			}
			return;
		}

		printf("\tMove TO which peg (A/B/C): ");
		to_ch = toupper(getch());
		printf("%c\n", to_ch);

		if (from_ch < 'A' || from_ch > 'C' || to_ch < 'A' || to_ch > 'C')
		{
			strcpy(message, "Use only the letters A, B or C.");
			continue;
		}

		from = from_ch - 'A';
		to = to_ch - 'A';

		if (from == to) strcpy(message, "Pick two different pegs.");
		else if (height[from] == 0) strcpy(message, "That peg is empty!");
		else if (!is_valid_move(from, to)) strcpy(message, "Illegal! A bigger disk cannot go on a smaller one.");
		else
		{
			move_disk(from, to);
			sprintf(message, "Moved disk %d from %c to %c.", top_disk(to), from_ch, to_ch);
		}
	}

	if (logfile != NULL)
	{
		fprintf(logfile, "Solved in %d moves\n", move_count);
		fclose(logfile);
		logfile = NULL;
	}

	system("cls");
	printf("\n\t======== YOU SOLVED IT! ========\n");
	print_pegs();
	check_result();
	printf("\n\tThe move log was saved to hanoi_log.txt\n");
	printf("\tPress any key to return to the menu: ");
	getch();
}

void view_log()
{
	FILE* fp = fopen("hanoi_log.txt", "r");
	char line[100];

	if (fp == NULL)
	{
		printf("\n\tNo move log yet. Play a game or watch the solver first.\n");
		return;
	}

	printf("\n\t------------ LAST MOVE LOG ------------\n");
	while (fgets(line, sizeof(line), fp) != NULL)
	{
		printf("\t%s", line);
	}
	fclose(fp);
}

void rules()
{
	printf("\n\t Rules : ");
	printf("\n\t - There are three pegs: A, B and C. All the disks start on peg A.");
	printf("\n\t - Goal: move the whole tower to peg C.");
	printf("\n\t - Move only one disk at a time, and only the top disk of a peg.");
	printf("\n\t - A bigger disk can never be placed on top of a smaller one.");
	printf("\n\t - The minimum number of moves for n disks is 2^n - 1.");
	printf("\n\t - Every move is written to hanoi_log.txt.\n");
}

int main()
{
	int choice;

	do
	{
		printf("\n******Welcome to Tower of Hanoi Solver & Move Logger*****\n");
		printf("\n\nMain Menu:\n");
		printf("1. Watch the Solver\n2. Play Manually\n3. View Last Move Log\n4. Rules\n5. Exit\n\n");

		if (scanf("%d", &choice) != 1)
		{
			while (getchar() != '\n' && !feof(stdin));
			choice = 0;
		}

		switch (choice)
		{
			case 1: watch_solver(); break;
			case 2: play_manual(); break;
			case 3: view_log(); break;
			case 4: rules(); break;
			case 5: printf("\n\tGoodbye!\n"); break;
			default: printf("\n\tPlease enter a number from 1 to 5.\n"); break;
		}
	}
	while (choice != 5 && !feof(stdin));

	return 0;
}
