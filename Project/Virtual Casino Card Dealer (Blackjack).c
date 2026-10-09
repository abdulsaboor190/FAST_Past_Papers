#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <conio.h>
#include <ctype.h>
#include <time.h>

#define DECK_SIZE 52
#define HAND_MAX 12
#define START_CHIPS 1000
#define RESHUFFLE_AT 15
#define MAX_SCORES 5

struct Card
{
	int rank;			//0 = Ace, 1-8 = 2 to 9, 9 = 10, 10 = Jack, 11 = Queen, 12 = King
	int suit;			//0 = Spades, 1 = Hearts, 2 = Diamonds, 3 = Clubs
	int value;			//Blackjack value (Ace counts as 11 here)
};

struct Score
{
	char name[20];
	int chips;
	int rounds;
};

int word_select = 0;
int rnd_index(int i);

char rank_names[13][3] = {"A", "2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K"};
char suit_letters[5] = "SHDC";

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

//Pointer swapping routine used by the shuffle
void swap_cards(struct Card *a, struct Card *b)
{
	struct Card temp;
	temp = *a;
	*a = *b;
	*b = temp;
}

void swap_scores(struct Score *a, struct Score *b)
{
	struct Score temp;
	temp = *a;
	*a = *b;
	*b = temp;
}

void init_deck(struct Card *deck)
{
	int s, r;
	int i = 0;

	for ( s = 0; s < 4; s++)
	{
		for ( r = 0; r < 13; r++)
		{
			(deck + i)->rank = r;
			(deck + i)->suit = s;

			if(r == 0)
			{
				(deck + i)->value = 11;
			}
			else if(r >= 9)
			{
				(deck + i)->value = 10;
			}
			else
			{
				(deck + i)->value = r + 1;
			}
			i++;
		}
	}
}

//Fisher-Yates shuffle: every card is swapped with a random card before it
void shuffle_deck(struct Card *deck)
{
	int i, j;

	for ( i = DECK_SIZE - 1; i > 0; i--)
	{
		j = rnd_index(i + 1);
		swap_cards(deck + i, deck + j);
	}
}

struct Card deal_card(struct Card *deck, int *next)
{
	if(*next >= DECK_SIZE)
	{
		shuffle_deck(deck);
		*next = 0;
		printf("\n\t[The dealer reshuffles the deck]\n");
	}

	struct Card c = *(deck + *next);
	(*next)++;
	return c;
}

//Aces count as 11 and drop to 1 while the hand would bust
int hand_total(struct Card *hand, int count)
{
	int i;
	int total = 0;
	int aces = 0;

	for ( i = 0; i < count; i++)
	{
		total = total + (hand + i)->value;
		if((hand + i)->rank == 0)
		{
			aces++;
		}
	}

	while(total > 21 && aces > 0)
	{
		total = total - 10;
		aces--;
	}
	return total;
}

void print_hand(struct Card *hand, int count, int hide_second)
{
	int i;
	struct Card *c;

	for ( i = 0; i < count; i++)
	{
		c = hand + i;
		if(hide_second == 1 && i == 1)
		{
			printf("[??] ");
		}
		else
		{
			printf("[%s-%c] ", rank_names[c->rank], suit_letters[c->suit]);
		}
	}
}

void show_table(struct Card *player, int pc, struct Card *dealer, int dc, int hide, int bet, int chips)
{
	printf("\n\t======== BLACKJACK ========\n");
	printf("\tS = Spades  H = Hearts  D = Diamonds  C = Clubs\n\n");
	printf("\tChips: %d     Bet: %d\n\n", chips, bet);

	printf("\tDealer : ");
	print_hand(dealer, dc, hide);
	if(hide == 1)
	{
		printf("  (total: ?)\n");
	}
	else
	{
		printf("  (total: %d)\n", hand_total(dealer, dc));
	}

	printf("\tYou    : ");
	print_hand(player, pc, 0);
	printf("  (total: %d)\n", hand_total(player, pc));
}

//Plays one hand. Returns 0 = lost, 1 = won, 2 = push, 3 = won with a blackjack
int play_round(struct Card *deck, int *next, int *chips)
{
	struct Card player[HAND_MAX];
	struct Card dealer[HAND_MAX];
	int pc = 0, dc = 0;
	int bet, ptotal, dtotal;
	int standing = 0;
	char action;

	system("cls");

	if(*next > DECK_SIZE - RESHUFFLE_AT)
	{
		shuffle_deck(deck);
		*next = 0;
		printf("\n\t[The dealer reshuffles the deck]\n");
	}

	printf("\n\tChips: %d\n", *chips);
	bet = read_int("\tPlace your bet: ", 1, *chips);

	player[pc] = deal_card(deck, next);
	pc++;
	dealer[dc] = deal_card(deck, next);
	dc++;
	player[pc] = deal_card(deck, next);
	pc++;
	dealer[dc] = deal_card(deck, next);
	dc++;

	ptotal = hand_total(player, pc);
	dtotal = hand_total(dealer, dc);

	system("cls");
	show_table(player, pc, dealer, dc, 1, bet, *chips);

	//Natural blackjack check
	if(ptotal == 21 || dtotal == 21)
	{
		system("cls");
		show_table(player, pc, dealer, dc, 0, bet, *chips);

		if(ptotal == 21 && dtotal == 21)
		{
			printf("\n\tBoth have blackjack. PUSH, your bet is returned.\n");
			return 2;
		}
		else if(ptotal == 21)
		{
			printf("\n\tBLACKJACK! You win %d chips (pays 3 to 2).\n", bet * 3 / 2);
			*chips = *chips + bet * 3 / 2;
			return 3;
		}
		printf("\n\tDealer has blackjack. You lose %d chips.\n", bet);
		*chips = *chips - bet;
		return 0;
	}

	//Player's turn
	while(standing == 0 && ptotal < 21)
	{
		printf("\n\t(H)it, (S)tand");
		if(pc == 2 && *chips >= bet * 2)
		{
			printf(", (D)ouble down");
		}
		printf(": ");

		action = toupper(getch());
		printf("%c\n", action);

		if(action == 'H')
		{
			player[pc] = deal_card(deck, next);
			pc++;
			ptotal = hand_total(player, pc);
			system("cls");
			show_table(player, pc, dealer, dc, 1, bet, *chips);
		}
		else if(action == 'S')
		{
			standing = 1;
		}
		else if(action == 'D' && pc == 2 && *chips >= bet * 2)
		{
			bet = bet * 2;
			player[pc] = deal_card(deck, next);
			pc++;
			ptotal = hand_total(player, pc);
			standing = 1;
			system("cls");
			show_table(player, pc, dealer, dc, 1, bet, *chips);
			printf("\n\tDoubled down! One card only.\n");
		}
		else
		{
			printf("\tInvalid choice!\n");
		}
	}

	if(ptotal > 21)
	{
		system("cls");
		show_table(player, pc, dealer, dc, 0, bet, *chips);
		printf("\n\tBUST! You lose %d chips.\n", bet);
		*chips = *chips - bet;
		return 0;
	}

	//Dealer's turn: draws until 17 or more
	while(dtotal < 17)
	{
		dealer[dc] = deal_card(deck, next);
		dc++;
		dtotal = hand_total(dealer, dc);
	}

	system("cls");
	show_table(player, pc, dealer, dc, 0, bet, *chips);

	if(dtotal > 21)
	{
		printf("\n\tDealer busts! You win %d chips.\n", bet);
		*chips = *chips + bet;
		return 1;
	}
	else if(ptotal > dtotal)
	{
		printf("\n\tYou win %d chips!\n", bet);
		*chips = *chips + bet;
		return 1;
	}
	else if(ptotal < dtotal)
	{
		printf("\n\tDealer wins. You lose %d chips.\n", bet);
		*chips = *chips - bet;
		return 0;
	}

	printf("\n\tPUSH, your bet is returned.\n");
	return 2;
}

void sort_scores(struct Score *scores, int count)
{
	int i, j;

	for ( i = 0; i < count - 1; i++)
	{
		for ( j = 0; j < count - i - 1; j++)
		{
			if((scores + j)->chips < (scores + j + 1)->chips)
			{
				swap_scores(scores + j, scores + j + 1);
			}
		}
	}
}

int load_scores(struct Score *scores)
{
	FILE *fp = fopen("blackjack_scores.txt", "r");
	int count = 0;

	if(fp == NULL)
	{
		return 0;
	}

	while(count < MAX_SCORES && fscanf(fp, "%19s %d %d", (scores + count)->name, &(scores + count)->chips, &(scores + count)->rounds) == 3)
	{
		count++;
	}

	fclose(fp);
	return count;
}

void save_scores(struct Score *scores, int count)
{
	FILE *fp = fopen("blackjack_scores.txt", "w");
	int i;

	if(fp == NULL)
	{
		printf("\n\tCould not save high scores.\n");
		return;
	}

	for ( i = 0; i < count; i++)
	{
		fprintf(fp, "%s %d %d\n", (scores + i)->name, (scores + i)->chips, (scores + i)->rounds);
	}
	fclose(fp);
}

//Inserts a new score, keeps the best MAX_SCORES and returns 1 if it made the list
int add_score(struct Score *scores, int *count, char *name, int chips, int rounds)
{
	int i;

	strcpy((scores + *count)->name, name);
	(scores + *count)->chips = chips;
	(scores + *count)->rounds = rounds;
	(*count)++;

	sort_scores(scores, *count);
	if(*count > MAX_SCORES)
	{
		*count = MAX_SCORES;
	}

	for ( i = 0; i < *count; i++)
	{
		if(strcmp((scores + i)->name, name) == 0 && (scores + i)->chips == chips && (scores + i)->rounds == rounds)
		{
			return 1;
		}
	}
	return 0;
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

	printf("\n\t%-5s%-20s%-10s%s\n", "No.", "Player", "Chips", "Rounds");
	printf("\t----------------------------------------\n");
	for ( i = 0; i < count; i++)
	{
		printf("\t%-5d%-20s%-10d%d\n", i + 1, (scores + i)->name, (scores + i)->chips, (scores + i)->rounds);
	}
	printf("\n");
}

void rules()
{
	printf("\n\t Rules : ");
	printf("\n\t - You start with %d chips. Place a bet before every hand.", START_CHIPS);
	printf("\n\t - Get closer to 21 than the dealer without going over.");
	printf("\n\t - Number cards count as their number, J Q K as 10, Ace as 11 or 1.");
	printf("\n\t - Hit takes a card, Stand keeps your hand, Double down doubles your bet for one card.");
	printf("\n\t - A blackjack (Ace + 10 value card) pays 3 to 2.");
	printf("\n\t - The dealer must draw until reaching 17 or more.");
	printf("\n\t - Your final chips are logged in the top %d high scores.\n\n", MAX_SCORES);
}

int main()
{
	struct Card deck[DECK_SIZE];
	struct Score scores[MAX_SCORES + 1];
	char name[20];
	char again;
	int next = 0;
	int score_count;
	int choice, chips, before, result;
	int rounds, wins, losses, pushes, blackjacks, best_win;

	init_deck(deck);
	shuffle_deck(deck);
	score_count = load_scores(scores);

	do
	{
		printf("\n******Welcome to the Virtual Casino Blackjack Dealer*****\n");
		printf("\n1. PLAY\n2. RULES\n3. HIGH SCORES\n4. EXIT\n\n");
		choice = read_int("", 1, 4);

		switch(choice)
		{
			case 1:				//Play
			{
				printf("\n\tEnter your name (no spaces): ");
				scanf("%19s", name);
				while(getchar() != '\n');

				chips = START_CHIPS;
				rounds = 0;
				wins = 0;
				losses = 0;
				pushes = 0;
				blackjacks = 0;
				best_win = 0;
				again = 'Y';

				while(again == 'Y' && chips > 0)
				{
					before = chips;
					result = play_round(deck, &next, &chips);
					rounds++;

					if(result == 1)
					{
						wins++;
					}
					else if(result == 3)
					{
						wins++;
						blackjacks++;
					}
					else if(result == 2)
					{
						pushes++;
					}
					else
					{
						losses++;
					}

					if(chips - before > best_win)
					{
						best_win = chips - before;
					}

					if(chips <= 0)
					{
						printf("\n\tYou are out of chips!\n");
					}
					else
					{
						printf("\n\tChips: %d. Play another hand? (Y/N): ", chips);
						again = toupper(getch());
						printf("%c\n", again);
					}
				}

				printf("\n\t===== SESSION SUMMARY =====");
				printf("\n\tPlayer       : %s", name);
				printf("\n\tFinal chips  : %d", chips);
				printf("\n\tHands played : %d", rounds);
				printf("\n\tWins / Losses / Pushes : %d / %d / %d", wins, losses, pushes);
				printf("\n\tBlackjacks   : %d", blackjacks);
				printf("\n\tBiggest win  : %d chips\n", best_win);

				if(add_score(scores, &score_count, name, chips, rounds) == 1)
				{
					printf("\n\tCongratulations! You made the high score list!\n");
				}
				save_scores(scores, score_count);
				break;
			}
			case 2:				//Rules
			{
				rules();
				break;
			}
			case 3:				//High scores
			{
				show_scores(scores, score_count);
				break;
			}
			case 4:				//Exit
			{
				printf("\n\tThanks for playing. The house always remembers!\n");
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
