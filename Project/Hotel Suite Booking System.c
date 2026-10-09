#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <conio.h>
#include <ctype.h>
#include <time.h>

#define ROOMS 20
#define DELUXE_RATE 12000
#define ECONOMY_RATE 5000

#define VACANT 0
#define RESERVED 1
#define OCCUPIED 2
#define CLEANING 3

struct Room
{
	int number;
	char type[10];
	int price;			//per night in Rs
	int state;
	char guest[30];
	char phone[12];
	int nights;
};

char* state_name(int s)
{
	switch(s)
	{
		case RESERVED: return "Reserved";
		case OCCUPIED: return "Occupied";
		case CLEANING: return "Cleaning";
		default: return "Vacant";
	}
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

//Reads a whole line (spaces allowed) and removes the newline
void read_line(char *buf, int size)
{
	int len;

	fgets(buf, size, stdin);
	len = strlen(buf);
	if(len > 0 && buf[len - 1] == '\n')
	{
		buf[len - 1] = '\0';
	}
	else
	{
		while(getchar() != '\n');
	}
}

int valid_name(char *name)
{
	int i;
	int letters = 0;
	int len = strlen(name);

	if(len < 2 || len > 29)
	{
		return 0;
	}
	for ( i = 0; i < len; i++)
	{
		if(isalpha(name[i]))
		{
			letters++;
		}
		else if(name[i] != ' ')
		{
			return 0;
		}
	}
	return letters >= 2;
}

int valid_phone(char *phone)
{
	int i;
	if(strlen(phone) != 11)
	{
		return 0;
	}
	for ( i = 0; i < 11; i++)
	{
		if(!isdigit(phone[i]))
		{
			return 0;
		}
	}
	return 1;
}

//ali raza khan -> Ali Raza Khan
void capitalize_name(char *name)
{
	int new_word = 1;
	while(*name != '\0')
	{
		if(*name == ' ')
		{
			new_word = 1;
		}
		else if(new_word == 1)
		{
			*name = toupper(*name);
			new_word = 0;
		}
		else
		{
			*name = tolower(*name);
		}
		name++;
	}
}

void lower_copy(char *dest, char *src, int size)
{
	int i;
	for ( i = 0; i < size - 1 && src[i] != '\0'; i++)
	{
		dest[i] = tolower(src[i]);
	}
	dest[i] = '\0';
}

int same_text(char *a, char *b)
{
	while(*a != '\0' && *b != '\0')
	{
		if(tolower(*a) != tolower(*b))
		{
			return 0;
		}
		a++;
		b++;
	}
	return *a == *b;
}

//Case insensitive check that part appears inside text
int contains_text(char *text, char *part)
{
	char t[50], p[50];

	lower_copy(t, text, 50);
	lower_copy(p, part, 50);
	return strstr(t, p) != NULL;
}

//Economy rooms 101-112 on floor 1, Deluxe rooms 201-208 on floor 2 (numbers stay sorted)
void init_rooms(struct Room rooms[])
{
	int i;
	for ( i = 0; i < ROOMS; i++)
	{
		if(i < 12)
		{
			rooms[i].number = 101 + i;
			strcpy(rooms[i].type, "Economy");
			rooms[i].price = ECONOMY_RATE;
		}
		else
		{
			rooms[i].number = 201 + (i - 12);
			strcpy(rooms[i].type, "Deluxe");
			rooms[i].price = DELUXE_RATE;
		}
		rooms[i].state = VACANT;
		strcpy(rooms[i].guest, "");
		strcpy(rooms[i].phone, "");
		rooms[i].nights = 0;
	}
}

//Binary search on the sorted room numbers, returns the index or -1
int find_room(struct Room rooms[], int number)
{
	int low = 0, high = ROOMS - 1, mid;

	while(low <= high)
	{
		mid = (low + high) / 2;
		if(rooms[mid].number == number)
		{
			return mid;
		}
		else if(rooms[mid].number < number)
		{
			low = mid + 1;
		}
		else
		{
			high = mid - 1;
		}
	}
	return -1;
}

//10% discount for stays of 5 nights or more
int calc_total(int price, int nights)
{
	int total = price * nights;
	if(nights >= 5)
	{
		total = total * 90 / 100;
	}
	return total;
}

void print_header()
{
	printf("\n\t%-7s%-10s%-9s%-10s%-24s%s\n", "Room", "Type", "Rate", "Status", "Guest", "Nights");
	printf("\t------------------------------------------------------------------\n");
}

void print_room(struct Room *r)
{
	printf("\t%-7d%-10s%-9d%-10s", r->number, r->type, r->price, state_name(r->state));
	if(r->state == RESERVED || r->state == OCCUPIED)
	{
		printf("%-24s%d\n", r->guest, r->nights);
	}
	else
	{
		printf("%-24s%s\n", "-", "-");
	}
}

void print_summary(struct Room rooms[], int revenue)
{
	int counts[2][4] = {{0}};
	int i, t;
	int occupied = 0;

	for ( i = 0; i < ROOMS; i++)
	{
		if(strcmp(rooms[i].type, "Deluxe") == 0)
		{
			t = 0;
		}
		else
		{
			t = 1;
		}
		counts[t][rooms[i].state]++;
		if(rooms[i].state == OCCUPIED)
		{
			occupied++;
		}
	}

	printf("\n\t===== OCCUPANCY SUMMARY =====\n");
	printf("\t%-10s%-9s%-10s%-10s%s\n", "Type", "Vacant", "Reserved", "Occupied", "Cleaning");
	printf("\t%-10s%-9d%-10d%-10d%d\n", "Deluxe", counts[0][VACANT], counts[0][RESERVED], counts[0][OCCUPIED], counts[0][CLEANING]);
	printf("\t%-10s%-9d%-10d%-10d%d\n", "Economy", counts[1][VACANT], counts[1][RESERVED], counts[1][OCCUPIED], counts[1][CLEANING]);
	printf("\n\tOccupancy rate : %.1f%%", (float)occupied / ROOMS * 100);
	printf("\n\tTotal revenue  : Rs %d\n", revenue);
}

void view_rooms(struct Room rooms[], int revenue)
{
	int i;

	printf("\n\t===== ALL ROOMS =====\n");
	print_header();
	for ( i = 0; i < ROOMS; i++)
	{
		print_room(&rooms[i]);
	}
	print_summary(rooms, revenue);
}

//Vacant -> Reserved
void book_room(struct Room rooms[])
{
	char name[50], phone[50];
	char type_name[10];
	int type, nights, i;
	int found = -1;

	printf("\n\t===== NEW BOOKING =====\n");
	printf("\t1. Deluxe  (Rs %d per night)\n\t2. Economy (Rs %d per night)\n", DELUXE_RATE, ECONOMY_RATE);
	type = read_int("\tSelect room type: ", 1, 2);

	if(type == 1)
	{
		strcpy(type_name, "Deluxe");
	}
	else
	{
		strcpy(type_name, "Economy");
	}

	for ( i = 0; i < ROOMS && found == -1; i++)
	{
		if(strcmp(rooms[i].type, type_name) == 0 && rooms[i].state == VACANT)
		{
			found = i;
		}
	}

	if(found == -1)
	{
		printf("\n\tSorry, no %s room is vacant right now. Try the other type or check back later.\n", type_name);
		return;
	}

	nights = read_int("\tNumber of nights (1-30): ", 1, 30);

	do
	{
		printf("\tGuest name: ");
		read_line(name, 50);
		if(valid_name(name) == 0)
		{
			printf("\tName must be 2-29 characters, alphabets and spaces only!\n");
		}
	} while(valid_name(name) == 0);
	capitalize_name(name);

	do
	{
		printf("\tPhone number (11 digits): ");
		read_line(phone, 50);
		if(valid_phone(phone) == 0)
		{
			printf("\tPhone number must be exactly 11 digits!\n");
		}
	} while(valid_phone(phone) == 0);

	rooms[found].state = RESERVED;
	strcpy(rooms[found].guest, name);
	strcpy(rooms[found].phone, phone);
	rooms[found].nights = nights;

	printf("\n\tBooking confirmed!");
	printf("\n\tGuest      : %s", rooms[found].guest);
	printf("\n\tRoom       : %d (%s)", rooms[found].number, rooms[found].type);
	printf("\n\tNights     : %d", nights);
	printf("\n\tEstimated  : Rs %d\n", calc_total(rooms[found].price, nights));
}

//Reserved -> Occupied
void check_in(struct Room rooms[])
{
	char name[50];
	int number, idx;

	number = read_int("\n\tEnter room number: ", 101, 208);
	idx = find_room(rooms, number);

	if(idx == -1)
	{
		printf("\n\tRoom %d does not exist.\n", number);
		return;
	}
	if(rooms[idx].state != RESERVED)
	{
		printf("\n\tRoom %d is %s, so no check-in is possible.\n", number, state_name(rooms[idx].state));
		return;
	}

	printf("\tConfirm guest name: ");
	read_line(name, 50);

	if(same_text(name, rooms[idx].guest) == 0)
	{
		printf("\n\tName does not match the reservation for room %d.\n", number);
		return;
	}

	rooms[idx].state = OCCUPIED;
	printf("\n\tWelcome, %s! Room %d is ready for you.\n", rooms[idx].guest, number);
}

//Occupied -> Cleaning, prints the bill
int check_out(struct Room rooms[])
{
	int number, idx;
	int subtotal, total;

	number = read_int("\n\tEnter room number: ", 101, 208);
	idx = find_room(rooms, number);

	if(idx == -1)
	{
		printf("\n\tRoom %d does not exist.\n", number);
		return 0;
	}
	if(rooms[idx].state != OCCUPIED)
	{
		printf("\n\tRoom %d is %s, so no check-out is possible.\n", number, state_name(rooms[idx].state));
		return 0;
	}

	subtotal = rooms[idx].price * rooms[idx].nights;
	total = calc_total(rooms[idx].price, rooms[idx].nights);

	printf("\n\t========== BILL ==========");
	printf("\n\tGuest    : %s", rooms[idx].guest);
	printf("\n\tRoom     : %d (%s)", rooms[idx].number, rooms[idx].type);
	printf("\n\tNights   : %d x Rs %d = Rs %d", rooms[idx].nights, rooms[idx].price, subtotal);
	if(rooms[idx].nights >= 5)
	{
		printf("\n\tDiscount : 10%% (Rs %d)", subtotal - total);
	}
	printf("\n\tTOTAL    : Rs %d", total);
	printf("\n\t==========================\n");

	rooms[idx].state = CLEANING;
	strcpy(rooms[idx].guest, "");
	strcpy(rooms[idx].phone, "");
	rooms[idx].nights = 0;
	return total;
}

//Cleaning -> Vacant
void clean_room(struct Room rooms[])
{
	int number, idx;

	number = read_int("\n\tEnter room number: ", 101, 208);
	idx = find_room(rooms, number);

	if(idx == -1)
	{
		printf("\n\tRoom %d does not exist.\n", number);
	}
	else if(rooms[idx].state != CLEANING)
	{
		printf("\n\tRoom %d is %s and does not need cleaning.\n", number, state_name(rooms[idx].state));
	}
	else
	{
		rooms[idx].state = VACANT;
		printf("\n\tRoom %d is cleaned and vacant again.\n", number);
	}
}

//Reserved -> Vacant
void cancel_booking(struct Room rooms[])
{
	int number, idx;

	number = read_int("\n\tEnter room number: ", 101, 208);
	idx = find_room(rooms, number);

	if(idx == -1)
	{
		printf("\n\tRoom %d does not exist.\n", number);
	}
	else if(rooms[idx].state != RESERVED)
	{
		printf("\n\tRoom %d has no reservation to cancel.\n", number);
	}
	else
	{
		printf("\n\tReservation of %s for room %d cancelled.\n", rooms[idx].guest, number);
		rooms[idx].state = VACANT;
		strcpy(rooms[idx].guest, "");
		strcpy(rooms[idx].phone, "");
		rooms[idx].nights = 0;
	}
}

void search_rooms(struct Room rooms[])
{
	char name[50];
	int option, number, idx, i, type;
	int matches = 0;

	printf("\n\t===== SEARCH =====\n");
	printf("\t1. By room number\n\t2. By guest name\n\t3. Vacant rooms by type\n");
	option = read_int("\tSelect option: ", 1, 3);

	switch(option)
	{
		case 1:
		{
			number = read_int("\tEnter room number: ", 1, 999);
			idx = find_room(rooms, number);
			if(idx == -1)
			{
				printf("\n\tRoom %d does not exist.\n", number);
			}
			else
			{
				print_header();
				print_room(&rooms[idx]);
				if(rooms[idx].state == RESERVED || rooms[idx].state == OCCUPIED)
				{
					printf("\n\tContact : %s\n", rooms[idx].phone);
				}
			}
			break;
		}
		case 2:
		{
			printf("\tEnter full or partial guest name: ");
			read_line(name, 50);
			if(strlen(name) == 0)
			{
				printf("\n\tNothing entered!\n");
				break;
			}
			for ( i = 0; i < ROOMS; i++)
			{
				if((rooms[i].state == RESERVED || rooms[i].state == OCCUPIED) && contains_text(rooms[i].guest, name))
				{
					if(matches == 0)
					{
						print_header();
					}
					print_room(&rooms[i]);
					matches++;
				}
			}
			if(matches == 0)
			{
				printf("\n\tNo guest matches \"%s\".\n", name);
			}
			break;
		}
		case 3:
		{
			printf("\t1. Deluxe\n\t2. Economy\n");
			type = read_int("\tSelect room type: ", 1, 2);
			for ( i = 0; i < ROOMS; i++)
			{
				if(rooms[i].state == VACANT && ((type == 1 && strcmp(rooms[i].type, "Deluxe") == 0) || (type == 2 && strcmp(rooms[i].type, "Economy") == 0)))
				{
					if(matches == 0)
					{
						print_header();
					}
					print_room(&rooms[i]);
					matches++;
				}
			}
			if(matches == 0)
			{
				printf("\n\tNo vacant room of that type.\n");
			}
			break;
		}
	}
}

//Binary data stream: revenue first, then the whole room array
void save_data(struct Room rooms[], int revenue)
{
	FILE *fp = fopen("hotel.dat", "wb");

	if(fp == NULL)
	{
		printf("\n\tWarning: could not save data to hotel.dat\n");
		return;
	}
	fwrite(&revenue, sizeof(int), 1, fp);
	fwrite(rooms, sizeof(struct Room), ROOMS, fp);
	fclose(fp);
}

int load_data(struct Room rooms[], int *revenue)
{
	FILE *fp = fopen("hotel.dat", "rb");

	if(fp == NULL)
	{
		return 0;
	}
	if(fread(revenue, sizeof(int), 1, fp) != 1 || fread(rooms, sizeof(struct Room), ROOMS, fp) != ROOMS)
	{
		fclose(fp);
		return 0;
	}
	fclose(fp);
	return 1;
}

//Text report of every room
void save_report(struct Room rooms[], int revenue)
{
	FILE *fp = fopen("hotel_report.txt", "w");
	time_t now = time(NULL);
	int i, occupied = 0, reserved = 0;

	if(fp == NULL)
	{
		printf("\n\tCould not open hotel_report.txt for writing.\n");
		return;
	}

	fprintf(fp, "HOTEL SUITE BOOKING SYSTEM - STATUS REPORT\n");
	fprintf(fp, "Generated: %s\n", ctime(&now));
	fprintf(fp, "%-7s%-10s%-9s%-10s%-24s%s\n", "Room", "Type", "Rate", "Status", "Guest", "Nights");
	fprintf(fp, "------------------------------------------------------------------\n");

	for ( i = 0; i < ROOMS; i++)
	{
		fprintf(fp, "%-7d%-10s%-9d%-10s", rooms[i].number, rooms[i].type, rooms[i].price, state_name(rooms[i].state));
		if(rooms[i].state == RESERVED || rooms[i].state == OCCUPIED)
		{
			fprintf(fp, "%-24s%d\n", rooms[i].guest, rooms[i].nights);
		}
		else
		{
			fprintf(fp, "%-24s%s\n", "-", "-");
		}

		if(rooms[i].state == OCCUPIED)
		{
			occupied++;
		}
		else if(rooms[i].state == RESERVED)
		{
			reserved++;
		}
	}

	fprintf(fp, "\nOccupied rooms : %d\nReserved rooms : %d\nOccupancy rate : %.1f%%\nTotal revenue  : Rs %d\n",
			occupied, reserved, (float)occupied / ROOMS * 100, revenue);
	fclose(fp);
	printf("\n\tReport saved to hotel_report.txt\n");
}

int main()
{
	struct Room rooms[ROOMS];
	int revenue = 0;
	int choice, bill;

	init_rooms(rooms);
	if(load_data(rooms, &revenue) == 0)
	{
		init_rooms(rooms);
		revenue = 0;
		printf("\nNo saved data found. Starting with an empty hotel.\n");
	}
	else
	{
		printf("\nSaved hotel data loaded.\n");
	}

	do
	{
		printf("\n******Welcome to the Hotel Suite Booking System*****\n");
		printf("\n1. BOOK A ROOM\n2. CHECK IN\n3. CHECK OUT\n4. MARK ROOM CLEANED\n5. CANCEL RESERVATION\n6. VIEW ALL ROOMS\n7. SEARCH\n8. SAVE REPORT TO FILE\n9. EXIT\n\n");
		choice = read_int("", 1, 9);

		switch(choice)
		{
			case 1:				//Book
			{
				book_room(rooms);
				save_data(rooms, revenue);
				break;
			}
			case 2:				//Check in
			{
				check_in(rooms);
				save_data(rooms, revenue);
				break;
			}
			case 3:				//Check out
			{
				bill = check_out(rooms);
				revenue = revenue + bill;
				save_data(rooms, revenue);
				break;
			}
			case 4:				//Clean
			{
				clean_room(rooms);
				save_data(rooms, revenue);
				break;
			}
			case 5:				//Cancel
			{
				cancel_booking(rooms);
				save_data(rooms, revenue);
				break;
			}
			case 6:				//View
			{
				view_rooms(rooms, revenue);
				break;
			}
			case 7:				//Search
			{
				search_rooms(rooms);
				break;
			}
			case 8:				//Report
			{
				save_report(rooms, revenue);
				break;
			}
			case 9:				//Exit
			{
				save_data(rooms, revenue);
				printf("\n\tData saved. Goodbye!\n");
				break;
			}
		}
	} while(choice != 9);

	return 0;
}
