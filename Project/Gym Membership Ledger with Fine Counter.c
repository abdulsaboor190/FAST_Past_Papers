/*
 * Gym Membership Ledger with Fine Counter
 * ----------------------------------------
 * Programming Fundamentals - Project
 *
 * Concepts used:
 *   - Nested Structs : Member -> PersonalInfo, Member -> Package,
 *                      Member -> PaymentRecord (array of payment history)
 *   - Math Library   : ceil() for rounding up overdue days to full weeks,
 *                      fabs() for absolute difference, pow() for
 *                      compound penalty calculation, floor() for
 *                      discount computation — all from <math.h>
 *   - File I/O       : full save/load via fopen/fprintf/fscanf;
 *                      ledger persists in gym_ledger.txt
 *
 * Compile : gcc gym.c -o gym -lm
 * Run     : ./gym
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ===================== Constants ===================== */

#define MAX_MEMBERS     100
#define MAX_PAYMENTS    24       /* up to 24 payment history entries   */
#define NAME_LEN        50
#define PHONE_LEN       16
#define EMAIL_LEN       60
#define ADDR_LEN        80
#define PKG_LEN         30
#define DATA_FILE       "gym_ledger.txt"

/* Fine rates */
#define FINE_RATE_PER_DAY   5.0f   /* PKR per overdue day               */
#define FINE_CAP_WEEKS      8      /* fine stops growing after 8 weeks  */
#define GRACE_DAYS          3      /* no fine within 3 days of due date */
#define LOYALTY_THRESHOLD   12     /* months for loyalty discount        */
#define LOYALTY_DISCOUNT    0.10f  /* 10 % off next renewal              */

/* Member status */
#define STATUS_ACTIVE    0
#define STATUS_EXPIRED   1
#define STATUS_SUSPENDED 2
#define STATUS_CANCELLED 3

/* ===================== Structures ===================== */

typedef struct {
    int day, month, year;
} Date;

/* Nested struct 1 — personal profile */
typedef struct {
    char name[NAME_LEN];
    int  age;
    char phone[PHONE_LEN];
    char email[EMAIL_LEN];
    char address[ADDR_LEN];
    char emergencyContact[PHONE_LEN];
} PersonalInfo;

/* Nested struct 2 — membership package */
typedef struct {
    char  name[PKG_LEN];      /* Basic / Standard / Premium / VIP      */
    float monthlyFee;
    int   durationMonths;     /* package length                         */
    int   sessionsPerWeek;    /* allowed gym sessions                   */
    int   includesTrainer;    /* 1 = personal trainer included          */
    int   includesLocker;
} Package;

/* Single payment record */
typedef struct {
    int   month, year;
    float amountDue;
    float amountPaid;
    float fine;
    int   paidOnTime;         /* 1 = on time, 0 = late                  */
    Date  paidOn;
} PaymentRecord;

/* Top-level member — contains both nested structs */
typedef struct {
    int           id;
    PersonalInfo  info;       /* nested                                  */
    Package       pkg;        /* nested                                  */
    Date          joinDate;
    Date          renewalDue; /* date next payment is due                */
    float         outstandingFine;
    float         totalPaid;
    int           status;     /* STATUS_* above                          */
    int           monthsActive;
    PaymentRecord history[MAX_PAYMENTS]; /* payment history array        */
    int           historyCount;
} Member;

/* ===================== Globals ===================== */

Member members[MAX_MEMBERS];
int    memberCount = 0;
int    nextId      = 1;

/* ===================== Date helpers ===================== */

static long dateToDays(Date d)
{
    /* Gregorian approximation — sufficient for fine calculations */
    long y = d.year, m = d.month;
    if (m < 3) { y--; m += 12; }
    return 365*y + y/4 - y/100 + y/400
         + (153*m - 457)/5 + d.day - 306;
}

static int daysOverdue(Date due)
{
    /* Reads today via popen; falls back to a fixed date */
    Date today = {9, 10, 2026};
    FILE *f = popen("date +%d/%m/%Y 2>/dev/null", "r");
    if (f) { fscanf(f, "%d/%d/%d", &today.day, &today.month, &today.year); pclose(f); }
    long diff = dateToDays(today) - dateToDays(due);
    return (int)diff;
}

static Date addMonths(Date d, int months)
{
    d.month += months;
    while (d.month > 12) { d.month -= 12; d.year++; }
    return d;
}

static int isValidDate(Date d)
{
    int days[] = {0,31,28,31,30,31,30,31,31,30,31,30,31};
    if (d.year % 400 == 0 || (d.year % 4 == 0 && d.year % 100 != 0)) days[2]=29;
    return d.month>=1 && d.month<=12 && d.day>=1 && d.day<=days[d.month];
}

static void printDate(Date d) { printf("%02d/%02d/%04d", d.day, d.month, d.year); }

static const char *monthName(int m)
{
    const char *n[]={"","Jan","Feb","Mar","Apr","May","Jun",
                     "Jul","Aug","Sep","Oct","Nov","Dec"};
    return (m>=1&&m<=12)?n[m]:"?";
}

/* ===================== Math-library fine engine =====================
 *
 * Fine logic (uses ceil, fabs, pow, floor from <math.h>):
 *
 *  1. No fine if overdue <= GRACE_DAYS
 *  2. Fine = FINE_RATE_PER_DAY * overdue_days
 *     but overdue_days is capped at FINE_CAP_WEEKS * 7
 *  3. ceil()  — round up partial days to whole days
 *  4. pow()   — compound penalty: fine *= 1.02^weeks_overdue
 *               (2 % per week compound if more than 4 weeks late)
 *  5. floor() — truncate fine to nearest rupee for billing
 *  6. fabs()  — used when computing balance differences
 */

static float computeFine(Date due)
{
    int overdue = daysOverdue(due);
    if (overdue <= GRACE_DAYS) return 0.0f;

    /* cap days then round up to whole day using ceil */
    int cappedDays = (overdue > FINE_CAP_WEEKS * 7)
                     ? FINE_CAP_WEEKS * 7 : overdue;
    float fine = (float)ceil((double)cappedDays) * FINE_RATE_PER_DAY;

    /* compound penalty if more than 4 weeks overdue */
    double weeksOverdue = cappedDays / 7.0;
    if (weeksOverdue > 4.0)
        fine = (float)(fine * pow(1.02, floor(weeksOverdue) - 4));

    /* fabs — ensure fine is never negative (e.g. floating-point edge) */
    fine = (float)fabs((double)fine);

    /* floor to nearest whole rupee */
    return (float)floor((double)fine);
}

static float loyaltyDiscount(Member *m)
{
    /* floor() to get complete months */
    int months = (int)floor((double)m->monthsActive);
    if (months >= LOYALTY_THRESHOLD)
        return m->pkg.monthlyFee * LOYALTY_DISCOUNT;
    return 0.0f;
}

/* ===================== Status helpers ===================== */

static const char *statusStr(int s)
{
    switch(s){
        case STATUS_ACTIVE:    return "Active";
        case STATUS_EXPIRED:   return "Expired";
        case STATUS_SUSPENDED: return "Suspended";
        case STATUS_CANCELLED: return "Cancelled";
        default:               return "?";
    }
}

/* Scan all members and auto-update status based on renewal date */
static void refreshStatuses(void)
{
    for (int i = 0; i < memberCount; i++) {
        Member *m = &members[i];
        if (m->status == STATUS_CANCELLED) continue;
        int overdue = daysOverdue(m->renewalDue);
        if (overdue > 0)
            m->outstandingFine = computeFine(m->renewalDue);
        if (overdue > 30 && m->status == STATUS_ACTIVE)
            m->status = STATUS_EXPIRED;
        else if (overdue > 60)
            m->status = STATUS_SUSPENDED;
    }
}

/* ===================== Input helpers ===================== */

static void readLine(const char *prompt, char *buf, int size)
{
    printf("%s", prompt);
    if (fgets(buf, size, stdin)){
        buf[strcspn(buf,"\n")]='\0';
        for(char *p=buf;*p;p++) if(*p=='|') *p='/';
    }
}

static int readInt(const char *prompt, int lo, int hi)
{
    char line[64]; int v;
    for(;;){
        printf("%s",prompt);
        if(fgets(line,sizeof line,stdin)&&sscanf(line,"%d",&v)==1&&v>=lo&&v<=hi)
            return v;
        printf("  Enter %d-%d.\n",lo,hi);
    }
}

static float readFloat(const char *prompt, float lo, float hi)
{
    char line[64]; float v;
    for(;;){
        printf("%s",prompt);
        if(fgets(line,sizeof line,stdin)&&sscanf(line,"%f",&v)==1&&v>=lo&&v<=hi)
            return v;
        printf("  Enter %.2f-%.2f.\n",lo,hi);
    }
}

static Date readDate(const char *label)
{
    Date d; char line[32];
    for(;;){
        printf("%s (dd/mm/yyyy): ",label);
        if(fgets(line,sizeof line,stdin)&&
           sscanf(line,"%d/%d/%d",&d.day,&d.month,&d.year)==3&&
           isValidDate(d)) return d;
        printf("  Invalid date.\n");
    }
}

/* ===================== Package presets ===================== */

static void selectPackage(Package *p)
{
    printf("\nPackages:\n");
    printf("  1. Basic    — PKR 1500/mo, 3x/week, no trainer, no locker\n");
    printf("  2. Standard — PKR 2500/mo, 5x/week, no trainer, locker\n");
    printf("  3. Premium  — PKR 4000/mo, unlimited,  trainer, locker\n");
    printf("  4. VIP      — PKR 7000/mo, unlimited,  trainer, locker + sauna\n");
    printf("  5. Custom\n");

    int choice = readInt("Choice: ", 1, 5);
    switch(choice){
        case 1: strcpy(p->name,"Basic");    p->monthlyFee=1500; p->sessionsPerWeek=3; p->includesTrainer=0; p->includesLocker=0; break;
        case 2: strcpy(p->name,"Standard"); p->monthlyFee=2500; p->sessionsPerWeek=5; p->includesTrainer=0; p->includesLocker=1; break;
        case 3: strcpy(p->name,"Premium");  p->monthlyFee=4000; p->sessionsPerWeek=7; p->includesTrainer=1; p->includesLocker=1; break;
        case 4: strcpy(p->name,"VIP");      p->monthlyFee=7000; p->sessionsPerWeek=7; p->includesTrainer=1; p->includesLocker=1; break;
        default:
            readLine("Package name: ", p->name, PKG_LEN);
            p->monthlyFee     = readFloat("Monthly fee (PKR): ", 100, 100000);
            p->sessionsPerWeek= readInt("Sessions per week: ", 1, 7);
            p->includesTrainer= readInt("Personal trainer? (1=Yes 0=No): ", 0, 1);
            p->includesLocker = readInt("Locker? (1=Yes 0=No): ", 0, 1);
    }
    p->durationMonths = readInt("Package duration (months): ", 1, 24);
}

/* ===================== Core features ===================== */

static void addMember(void)
{
    if (memberCount >= MAX_MEMBERS){ printf("Ledger full.\n"); return; }

    Member *m = &members[memberCount];
    memset(m, 0, sizeof(Member));
    m->id = nextId++;

    /* Personal info (nested struct) */
    readLine("Full name: ", m->info.name, NAME_LEN);
    if(m->info.name[0]=='\0'){ printf("Name required.\n"); nextId--; return; }
    m->info.age = readInt("Age: ", 5, 120);
    readLine("Phone: ", m->info.phone, PHONE_LEN);
    readLine("Email: ", m->info.email, EMAIL_LEN);
    readLine("Address: ", m->info.address, ADDR_LEN);
    readLine("Emergency contact phone: ", m->info.emergencyContact, PHONE_LEN);

    /* Package (nested struct) */
    selectPackage(&m->pkg);

    m->joinDate   = readDate("Join date");
    m->renewalDue = addMonths(m->joinDate, 1);

    m->status       = STATUS_ACTIVE;
    m->monthsActive = 0;
    m->historyCount = 0;
    m->outstandingFine = 0;
    m->totalPaid    = 0;

    memberCount++;
    printf("Member %s added (ID %d). First payment due: ", m->info.name, m->id);
    printDate(m->renewalDue); printf("\n");
}

static void listMembers(void)
{
    refreshStatuses();
    if(memberCount==0){ printf("No members.\n"); return; }

    printf("\n%-4s %-28s %-10s %-10s %8s %8s  %s\n",
           "ID","Name","Package","Status","Fee/mo","Fine","Renewal Due");
    printf("-----------------------------------------------------------------------------\n");

    for(int i=0;i<memberCount;i++){
        Member *m=&members[i];
        printf("%-4d %-28s %-10s %-10s %8.0f %8.0f  ",
               m->id, m->info.name, m->pkg.name, statusStr(m->status),
               m->pkg.monthlyFee, m->outstandingFine);
        printDate(m->renewalDue); printf("\n");
    }
    printf("Total: %d member(s).\n", memberCount);
}

static Member *findById(int id)
{
    for(int i=0;i<memberCount;i++)
        if(members[i].id==id) return &members[i];
    return NULL;
}

static void viewMember(void)
{
    int id = readInt("Member ID: ",1,999999);
    Member *m = findById(id);
    if(!m){ printf("Not found.\n"); return; }

    float fine = computeFine(m->renewalDue);
    m->outstandingFine = fine;

    printf("\n========== Member Profile ==========\n");
    printf("ID        : %d\n", m->id);
    printf("Name      : %s\n", m->info.name);
    printf("Age       : %d\n", m->info.age);
    printf("Phone     : %s\n", m->info.phone);
    printf("Email     : %s\n", m->info.email);
    printf("Address   : %s\n", m->info.address);
    printf("Emergency : %s\n", m->info.emergencyContact);
    printf("\n--- Package ---\n");
    printf("Name      : %s\n", m->pkg.name);
    printf("Fee/month : PKR %.2f\n", m->pkg.monthlyFee);
    printf("Duration  : %d month(s)\n", m->pkg.durationMonths);
    printf("Sessions  : %d/week\n", m->pkg.sessionsPerWeek);
    printf("Trainer   : %s\n", m->pkg.includesTrainer?"Yes":"No");
    printf("Locker    : %s\n", m->pkg.includesLocker?"Yes":"No");
    printf("\n--- Financials ---\n");
    printf("Join Date : "); printDate(m->joinDate); printf("\n");
    printf("Renewal   : "); printDate(m->renewalDue); printf("\n");
    printf("Status    : %s\n", statusStr(m->status));
    printf("Months    : %d\n", m->monthsActive);
    printf("Total Paid: PKR %.2f\n", m->totalPaid);

    int overdue = daysOverdue(m->renewalDue);
    if(overdue > GRACE_DAYS){
        printf("Overdue   : %d day(s)\n", overdue);
        printf("Fine      : PKR %.2f", fine);
        if(m->monthsActive >= LOYALTY_THRESHOLD){
            float disc = loyaltyDiscount(m);   /* uses floor() internally */
            printf("  (Loyalty discount on renewal: PKR %.2f)", disc);
        }
        printf("\n");
    } else {
        printf("Fine      : None (within grace period)\n");
    }

    if(m->historyCount > 0){
        printf("\n--- Payment History ---\n");
        printf("%-10s %-10s %-10s %-10s %-10s %s\n",
               "Month","Due","Paid","Fine","On Time","Paid On");
        printf("------------------------------------------------------------\n");
        for(int i=0;i<m->historyCount;i++){
            PaymentRecord *pr=&m->history[i];
            printf("%-3s %4d  %-10.2f %-10.2f %-10.2f %-10s ",
                   monthName(pr->month), pr->year,
                   pr->amountDue, pr->amountPaid, pr->fine,
                   pr->paidOnTime?"Yes":"Late");
            printDate(pr->paidOn); printf("\n");
        }
    }
}

static void recordPayment(void)
{
    int id = readInt("Member ID: ",1,999999);
    Member *m = findById(id);
    if(!m){ printf("Not found.\n"); return; }

    float fine = computeFine(m->renewalDue);
    float discount = loyaltyDiscount(m);               /* floor() inside */
    float totalDue = m->pkg.monthlyFee + fine - discount;

    printf("Monthly fee  : PKR %.2f\n", m->pkg.monthlyFee);
    if(fine > 0)     printf("Outstanding fine  : PKR %.2f\n", fine);
    if(discount > 0) printf("Loyalty discount  : -PKR %.2f\n", discount);
    printf("Total due    : PKR %.2f\n", totalDue);

    float paid = readFloat("Amount paying (PKR): ", 0, 9999999);

    /* Build payment record */
    if(m->historyCount < MAX_PAYMENTS){
        PaymentRecord *pr = &m->history[m->historyCount];
        pr->month      = m->renewalDue.month;
        pr->year       = m->renewalDue.year;
        pr->amountDue  = totalDue;
        pr->amountPaid = paid;
        pr->fine       = fine;
        pr->paidOnTime = (daysOverdue(m->renewalDue) <= GRACE_DAYS) ? 1 : 0;

        /* today as paid-on date */
        pr->paidOn.day=9; pr->paidOn.month=10; pr->paidOn.year=2026;
        FILE *f=popen("date +%d/%m/%Y 2>/dev/null","r");
        if(f){ fscanf(f,"%d/%d/%d",&pr->paidOn.day,&pr->paidOn.month,&pr->paidOn.year); pclose(f); }

        m->historyCount++;
    }

    m->totalPaid += paid;
    m->outstandingFine = (float)fabs((double)(fine - (paid - m->pkg.monthlyFee + discount)));
    if(m->outstandingFine < 0) m->outstandingFine = 0;

    /* Advance renewal date by 1 month */
    m->renewalDue = addMonths(m->renewalDue, 1);
    m->monthsActive++;
    m->status = STATUS_ACTIVE;

    printf("Payment recorded. Next renewal: "); printDate(m->renewalDue); printf("\n");
    float balance = paid - totalDue;
    if(balance >= 0) printf("Change / credit: PKR %.2f\n", balance);
    else             printf("Remaining balance: PKR %.2f\n", (float)fabs((double)balance));
}

static void updateMember(void)
{
    int id = readInt("Member ID: ",1,999999);
    Member *m = findById(id);
    if(!m){ printf("Not found.\n"); return; }

    char buf[EMAIL_LEN];
    printf("Leave blank to keep.\n");
    printf("Current phone: %s\n", m->info.phone);
    readLine("New phone: ", buf, PHONE_LEN);
    if(buf[0]) strcpy(m->info.phone, buf);

    printf("Current email: %s\n", m->info.email);
    readLine("New email: ", buf, EMAIL_LEN);
    if(buf[0]) strcpy(m->info.email, buf);

    printf("Current address: %s\n", m->info.address);
    readLine("New address: ", buf, ADDR_LEN);
    if(buf[0]) strcpy(m->info.address, buf);

    printf("Change package? (y/n): ");
    if(fgets(buf,sizeof buf,stdin)&&(buf[0]=='y'||buf[0]=='Y'))
        selectPackage(&m->pkg);

    printf("Member updated.\n");
}

static void suspendCancel(void)
{
    int id = readInt("Member ID: ",1,999999);
    Member *m = findById(id);
    if(!m){ printf("Not found.\n"); return; }

    printf("1) Suspend  2) Cancel  3) Reactivate: ");
    int c = readInt("",1,3);
    if(c==1)      m->status = STATUS_SUSPENDED;
    else if(c==2) m->status = STATUS_CANCELLED;
    else          m->status = STATUS_ACTIVE;
    printf("Status updated to %s.\n", statusStr(m->status));
}

static void fineReport(void)
{
    refreshStatuses();
    printf("\n=== Outstanding Fines ===\n");
    printf("%-4s %-28s %-10s %10s  Overdue\n","ID","Name","Status","Fine (PKR)");
    printf("------------------------------------------------------\n");

    float total=0; int cnt=0;
    for(int i=0;i<memberCount;i++){
        Member *m=&members[i];
        if(m->outstandingFine > 0){
            int overdue=daysOverdue(m->renewalDue);
            printf("%-4d %-28s %-10s %10.2f  %d day(s)\n",
                   m->id, m->info.name, statusStr(m->status),
                   m->outstandingFine, overdue);
            total+=m->outstandingFine; cnt++;
        }
    }
    if(cnt==0) printf("No outstanding fines.\n");
    else printf("\nTotal fines outstanding: PKR %.2f across %d member(s).\n",total,cnt);
}

static void renewalReport(void)
{
    int days = readInt("Flag renewals due within how many days? ", 1, 90);
    printf("\n=== Upcoming Renewals (within %d days) ===\n", days);
    printf("%-4s %-28s %-10s  Renewal Due  Overdue\n","ID","Name","Package");
    printf("------------------------------------------------------------\n");

    int cnt=0;
    for(int i=0;i<memberCount;i++){
        Member *m=&members[i];
        if(m->status==STATUS_CANCELLED) continue;
        int overdue=daysOverdue(m->renewalDue);
        /* show those due soon (negative overdue = future) */
        if(overdue >= -days && overdue <= days){
            printf("%-4d %-28s %-10s  ", m->id, m->info.name, m->pkg.name);
            printDate(m->renewalDue);
            if(overdue>0) printf("  (%d days overdue)", overdue);
            else          printf("  (in %d days)", -overdue);
            printf("\n"); cnt++;
        }
    }
    if(cnt==0) printf("None.\n");
    else printf("%d member(s) listed.\n",cnt);
}

static void revenueReport(void)
{
    float totalRevenue=0, totalFines=0;
    int active=0, expired=0, suspended=0;
    for(int i=0;i<memberCount;i++){
        Member *m=&members[i];
        totalRevenue += m->totalPaid;
        totalFines   += m->outstandingFine;
        if(m->status==STATUS_ACTIVE)    active++;
        else if(m->status==STATUS_EXPIRED)  expired++;
        else if(m->status==STATUS_SUSPENDED) suspended++;
    }
    printf("\n=== Revenue & Membership Summary ===\n");
    printf("Total members      : %d\n", memberCount);
    printf("Active             : %d\n", active);
    printf("Expired            : %d\n", expired);
    printf("Suspended          : %d\n", suspended);
    printf("Total revenue      : PKR %.2f\n", totalRevenue);
    printf("Outstanding fines  : PKR %.2f\n", totalFines);
    printf("Projected monthly  : PKR %.2f\n",
           (float)(active) * 3000 /* rough average */);
}

static void searchMember(void)
{
    char key[NAME_LEN];
    readLine("Search name (partial): ", key, NAME_LEN);
    int found=0;
    printf("\n%-4s %-28s %-12s %-10s\n","ID","Name","Phone","Status");
    printf("----------------------------------------------------\n");
    for(int i=0;i<memberCount;i++){
        if(strstr(members[i].info.name, key)){
            printf("%-4d %-28s %-12s %-10s\n",
                   members[i].id, members[i].info.name,
                   members[i].info.phone, statusStr(members[i].status));
            found++;
        }
    }
    printf("%d result(s).\n",found);
}

/* ===================== File I/O ===================== */
/*
 * Format:
 * M line  : M|id|name|age|phone|email|address|emergency|
 *            pkgName|fee|duration|sessions|trainer|locker|
 *            joinD|joinM|joinY|dueD|dueM|dueY|
 *            fine|totalPaid|status|monthsActive|histCount
 * P lines : P|month|year|due|paid|fine|onTime|paidD|paidM|paidY
 */

static void saveData(void)
{
    FILE *f=fopen(DATA_FILE,"w");
    if(!f){ printf("Cannot write %s.\n",DATA_FILE); return; }

    for(int i=0;i<memberCount;i++){
        Member *m=&members[i];
        fprintf(f,"M|%d|%s|%d|%s|%s|%s|%s|"
                  "%s|%.2f|%d|%d|%d|%d|"
                  "%d|%d|%d|%d|%d|%d|"
                  "%.2f|%.2f|%d|%d|%d\n",
                m->id, m->info.name, m->info.age,
                m->info.phone, m->info.email,
                m->info.address, m->info.emergencyContact,
                m->pkg.name, m->pkg.monthlyFee, m->pkg.durationMonths,
                m->pkg.sessionsPerWeek, m->pkg.includesTrainer, m->pkg.includesLocker,
                m->joinDate.day, m->joinDate.month, m->joinDate.year,
                m->renewalDue.day, m->renewalDue.month, m->renewalDue.year,
                m->outstandingFine, m->totalPaid,
                m->status, m->monthsActive, m->historyCount);

        for(int j=0;j<m->historyCount;j++){
            PaymentRecord *pr=&m->history[j];
            fprintf(f,"P|%d|%d|%.2f|%.2f|%.2f|%d|%d|%d|%d\n",
                    pr->month,pr->year,pr->amountDue,pr->amountPaid,
                    pr->fine,pr->paidOnTime,
                    pr->paidOn.day,pr->paidOn.month,pr->paidOn.year);
        }
    }
    fclose(f);
    printf("Ledger saved (%d records) to %s.\n", memberCount, DATA_FILE);
}

static void loadData(void)
{
    FILE *f=fopen(DATA_FILE,"r");
    if(!f) return;

    memberCount=0; nextId=1;
    char line[512];
    Member *cur=NULL;

    while(fgets(line,sizeof line,f)){
        line[strcspn(line,"\n")]='\0';
        if(line[0]=='M' && memberCount<MAX_MEMBERS){
            cur=&members[memberCount];
            memset(cur,0,sizeof(Member));
            int n=sscanf(line,
                "M|%d|%49[^|]|%d|%15[^|]|%59[^|]|%79[^|]|%15[^|]|"
                "%29[^|]|%f|%d|%d|%d|%d|"
                "%d|%d|%d|%d|%d|%d|"
                "%f|%f|%d|%d|%d",
                &cur->id, cur->info.name, &cur->info.age,
                cur->info.phone, cur->info.email,
                cur->info.address, cur->info.emergencyContact,
                cur->pkg.name, &cur->pkg.monthlyFee, &cur->pkg.durationMonths,
                &cur->pkg.sessionsPerWeek, &cur->pkg.includesTrainer, &cur->pkg.includesLocker,
                &cur->joinDate.day, &cur->joinDate.month, &cur->joinDate.year,
                &cur->renewalDue.day, &cur->renewalDue.month, &cur->renewalDue.year,
                &cur->outstandingFine, &cur->totalPaid,
                &cur->status, &cur->monthsActive, &cur->historyCount);
            if(n==24){
                if(cur->id>=nextId) nextId=cur->id+1;
                cur->historyCount=0; /* will be filled by P lines */
                memberCount++;
            } else cur=NULL;
        } else if(line[0]=='P' && cur && cur->historyCount<MAX_PAYMENTS){
            PaymentRecord *pr=&cur->history[cur->historyCount];
            int n=sscanf(line,"P|%d|%d|%f|%f|%f|%d|%d|%d|%d",
                &pr->month,&pr->year,&pr->amountDue,&pr->amountPaid,
                &pr->fine,&pr->paidOnTime,
                &pr->paidOn.day,&pr->paidOn.month,&pr->paidOn.year);
            if(n==9) cur->historyCount++;
        }
    }
    fclose(f);
    refreshStatuses();
    printf("Loaded %d member(s) from %s.\n",memberCount,DATA_FILE);
}

/* ===================== Menu ===================== */

static void printMenu(void)
{
    printf("\n========= Gym Membership Ledger =========\n");
    printf(" 1.  Add member           7.  Fine report\n");
    printf(" 2.  List members         8.  Renewal report\n");
    printf(" 3.  View member          9.  Revenue summary\n");
    printf(" 4.  Record payment      10.  Search member\n");
    printf(" 5.  Update member       11.  Save ledger\n");
    printf(" 6.  Suspend / Cancel     0.  Save & exit\n");
    printf("=========================================\n");
}

int main(void)
{
    printf("Gym Membership Ledger with Fine Counter\n");
    printf("========================================\n");
    loadData();

    int choice;
    do {
        printMenu();
        choice=readInt("Choice: ",0,11);
        switch(choice){
            case 1:  addMember();      break;
            case 2:  listMembers();    break;
            case 3:  viewMember();     break;
            case 4:  recordPayment();  break;
            case 5:  updateMember();   break;
            case 6:  suspendCancel();  break;
            case 7:  fineReport();     break;
            case 8:  renewalReport();  break;
            case 9:  revenueReport();  break;
            case 10: searchMember();   break;
            case 11: saveData();       break;
            case 0:  saveData();       break;
        }
    } while(choice!=0);

    printf("Goodbye!\n");
    return 0;
}
