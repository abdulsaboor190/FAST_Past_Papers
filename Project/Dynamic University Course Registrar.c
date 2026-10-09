/*
 * Dynamic University Course Registrar
 * -----------------------------------
 * Programming Fundamentals - Semester Project
 *
 * Concepts used:
 *   - Dynamic Memory Allocation: malloc / realloc / free
 *       * students array grows automatically (capacity doubles when full)
 *       * each student's enrolled-courses array grows the same way
 *       * the course catalog is a dynamic array too
 *   - Nested structures:
 *       Student -> Date (dob), Student -> Enrollment[] -> Course
 *   - File processing: data is saved to / loaded from text files
 *       (students.txt and courses.txt)
 *
 * Compile:  gcc registrar.c -o registrar
 * Run:      ./registrar
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_CAPACITY 2      /* small on purpose so growth is easy to see */
#define NAME_LEN   50
#define CODE_LEN   12
#define TITLE_LEN  60
#define PROG_LEN   30
#define NOT_GRADED -1.0f

#define STUDENT_FILE "students.txt"
#define COURSE_FILE  "courses.txt"

/* ===================== Structures ===================== */

typedef struct {
    int day, month, year;
} Date;

typedef struct {
    char code[CODE_LEN];
    char title[TITLE_LEN];
    int  creditHours;
} Course;

/* Nested: an Enrollment contains a Course */
typedef struct {
    Course course;
    float  marks;               /* NOT_GRADED (-1) until entered */
} Enrollment;

/* Nested: a Student contains a Date and a dynamic array of Enrollments */
typedef struct {
    int         id;
    char        name[NAME_LEN];
    Date        dob;
    char        program[PROG_LEN];
    Enrollment *enrollments;    /* malloc / realloc */
    int         enrollCount;
    int         enrollCap;
} Student;

typedef struct {
    Student *students;          /* malloc / realloc */
    int      count;
    int      capacity;
    Course  *catalog;           /* malloc / realloc */
    int      catCount;
    int      catCap;
} Registrar;

/* ===================== Input helpers ===================== */

static void readLine(const char *prompt, char *buf, int size)
{
    printf("%s", prompt);
    if (fgets(buf, size, stdin) == NULL) {
        buf[0] = '\0';
        return;
    }
    buf[strcspn(buf, "\n")] = '\0';
    /* '|' is our file delimiter, so never allow it inside text */
    for (char *p = buf; *p; p++)
        if (*p == '|') *p = '/';
}

static int readInt(const char *prompt, int min, int max)
{
    char line[64];
    int value;
    while (1) {
        printf("%s", prompt);
        if (fgets(line, sizeof line, stdin) == NULL) return min;
        if (sscanf(line, "%d", &value) == 1 && value >= min && value <= max)
            return value;
        printf("  Invalid input. Enter a number between %d and %d.\n", min, max);
    }
}

static float readMarks(const char *prompt)
{
    char line[64];
    float value;
    while (1) {
        printf("%s", prompt);
        if (fgets(line, sizeof line, stdin) == NULL) return NOT_GRADED;
        if (sscanf(line, "%f", &value) == 1 && value >= 0 && value <= 100)
            return value;
        printf("  Invalid. Marks must be between 0 and 100.\n");
    }
}

/* ===================== Grading ===================== */

static void gradeInfo(float marks, char *letter, float *points)
{
    if      (marks >= 85) { strcpy(letter, "A");  *points = 4.00f; }
    else if (marks >= 80) { strcpy(letter, "A-"); *points = 3.67f; }
    else if (marks >= 75) { strcpy(letter, "B+"); *points = 3.33f; }
    else if (marks >= 70) { strcpy(letter, "B");  *points = 3.00f; }
    else if (marks >= 65) { strcpy(letter, "B-"); *points = 2.67f; }
    else if (marks >= 60) { strcpy(letter, "C+"); *points = 2.33f; }
    else if (marks >= 55) { strcpy(letter, "C");  *points = 2.00f; }
    else if (marks >= 50) { strcpy(letter, "D");  *points = 1.00f; }
    else                  { strcpy(letter, "F");  *points = 0.00f; }
}

/* ===================== Memory management ===================== */

static void initRegistrar(Registrar *r)
{
    r->capacity = INITIAL_CAPACITY;
    r->count = 0;
    r->students = malloc(r->capacity * sizeof(Student));

    r->catCap = INITIAL_CAPACITY;
    r->catCount = 0;
    r->catalog = malloc(r->catCap * sizeof(Course));

    if (r->students == NULL || r->catalog == NULL) {
        printf("Fatal: out of memory.\n");
        exit(1);
    }
}

static void freeRegistrar(Registrar *r)
{
    for (int i = 0; i < r->count; i++)
        free(r->students[i].enrollments);
    free(r->students);
    free(r->catalog);
    r->students = NULL;
    r->catalog = NULL;
    r->count = r->capacity = r->catCount = r->catCap = 0;
}

/* Grow the students array when it is full. Returns 1 on success. */
static int ensureStudentCapacity(Registrar *r)
{
    if (r->count < r->capacity) return 1;

    int newCap = r->capacity * 2;
    Student *tmp = realloc(r->students, newCap * sizeof(Student));
    if (tmp == NULL) {
        printf("Error: could not expand student capacity.\n");
        return 0;
    }
    r->students = tmp;
    r->capacity = newCap;
    printf("[Memory] Student capacity expanded to %d.\n", newCap);
    return 1;
}

static int ensureCatalogCapacity(Registrar *r)
{
    if (r->catCount < r->catCap) return 1;

    int newCap = r->catCap * 2;
    Course *tmp = realloc(r->catalog, newCap * sizeof(Course));
    if (tmp == NULL) {
        printf("Error: could not expand course catalog.\n");
        return 0;
    }
    r->catalog = tmp;
    r->catCap = newCap;
    return 1;
}

static int ensureEnrollCapacity(Student *s)
{
    if (s->enrollments == NULL) {
        s->enrollCap = INITIAL_CAPACITY;
        s->enrollCount = 0;
        s->enrollments = malloc(s->enrollCap * sizeof(Enrollment));
        return s->enrollments != NULL;
    }
    if (s->enrollCount < s->enrollCap) return 1;

    int newCap = s->enrollCap * 2;
    Enrollment *tmp = realloc(s->enrollments, newCap * sizeof(Enrollment));
    if (tmp == NULL) {
        printf("Error: could not expand enrollment list.\n");
        return 0;
    }
    s->enrollments = tmp;
    s->enrollCap = newCap;
    return 1;
}

/* ===================== Lookup ===================== */

static Student *findStudent(Registrar *r, int id)
{
    for (int i = 0; i < r->count; i++)
        if (r->students[i].id == id) return &r->students[i];
    return NULL;
}

static Course *findCourse(Registrar *r, const char *code)
{
    for (int i = 0; i < r->catCount; i++)
        if (strcmp(r->catalog[i].code, code) == 0) return &r->catalog[i];
    return NULL;
}

static Enrollment *findEnrollment(Student *s, const char *code)
{
    for (int i = 0; i < s->enrollCount; i++)
        if (strcmp(s->enrollments[i].course.code, code) == 0)
            return &s->enrollments[i];
    return NULL;
}

/* ===================== Course catalog ===================== */

static void addCourse(Registrar *r)
{
    Course c;
    readLine("Course code (e.g. CS101): ", c.code, CODE_LEN);
    if (c.code[0] == '\0') { printf("Code cannot be empty.\n"); return; }
    if (findCourse(r, c.code)) { printf("That course already exists.\n"); return; }
    readLine("Course title: ", c.title, TITLE_LEN);
    c.creditHours = readInt("Credit hours (1-6): ", 1, 6);

    if (!ensureCatalogCapacity(r)) return;
    r->catalog[r->catCount++] = c;
    printf("Course %s added.\n", c.code);
}

static void listCourses(Registrar *r)
{
    if (r->catCount == 0) { printf("No courses in the catalog.\n"); return; }
    printf("\n%-10s %-40s %s\n", "Code", "Title", "Credits");
    printf("-------------------------------------------------------------\n");
    for (int i = 0; i < r->catCount; i++)
        printf("%-10s %-40s %d\n", r->catalog[i].code,
               r->catalog[i].title, r->catalog[i].creditHours);
}

/* ===================== Students ===================== */

static void addStudent(Registrar *r)
{
    int id = readInt("Student ID (positive number): ", 1, 999999999);
    if (findStudent(r, id)) { printf("A student with that ID already exists.\n"); return; }

    if (!ensureStudentCapacity(r)) return;

    Student s;
    s.id = id;
    readLine("Name: ", s.name, NAME_LEN);
    s.dob.day   = readInt("Date of birth - day (1-31): ", 1, 31);
    s.dob.month = readInt("Date of birth - month (1-12): ", 1, 12);
    s.dob.year  = readInt("Date of birth - year: ", 1950, 2020);
    readLine("Program (e.g. BSCS): ", s.program, PROG_LEN);
    s.enrollments = NULL;
    s.enrollCount = 0;
    s.enrollCap = 0;

    r->students[r->count++] = s;
    printf("Student %s added (total: %d).\n", s.name, r->count);
}

static void listStudents(Registrar *r)
{
    if (r->count == 0) { printf("No students registered.\n"); return; }
    printf("\n%-10s %-25s %-12s %-10s %s\n",
           "ID", "Name", "DOB", "Program", "Courses");
    printf("-----------------------------------------------------------------------\n");
    for (int i = 0; i < r->count; i++) {
        Student *s = &r->students[i];
        printf("%-10d %-25s %02d/%02d/%04d   %-10s %d\n", s->id, s->name,
               s->dob.day, s->dob.month, s->dob.year, s->program, s->enrollCount);
    }
    printf("(Used %d of %d student slots)\n", r->count, r->capacity);
}

static void updateStudent(Registrar *r)
{
    Student *s = findStudent(r, readInt("Student ID to update: ", 1, 999999999));
    if (!s) { printf("Student not found.\n"); return; }

    char buf[NAME_LEN];
    readLine("New name (blank = keep): ", buf, NAME_LEN);
    if (buf[0]) strcpy(s->name, buf);
    char prog[PROG_LEN];
    readLine("New program (blank = keep): ", prog, PROG_LEN);
    if (prog[0]) strcpy(s->program, prog);
    printf("Student updated.\n");
}

static void deleteStudent(Registrar *r)
{
    int id = readInt("Student ID to delete: ", 1, 999999999);
    for (int i = 0; i < r->count; i++) {
        if (r->students[i].id == id) {
            free(r->students[i].enrollments);
            for (int j = i; j < r->count - 1; j++)
                r->students[j] = r->students[j + 1];
            r->count--;
            printf("Student deleted.\n");
            return;
        }
    }
    printf("Student not found.\n");
}

static void searchStudent(Registrar *r)
{
    char key[NAME_LEN];
    readLine("Search name (partial match, case-sensitive): ", key, NAME_LEN);
    int found = 0;
    for (int i = 0; i < r->count; i++) {
        if (strstr(r->students[i].name, key)) {
            printf("  %d  %s  (%s)\n", r->students[i].id,
                   r->students[i].name, r->students[i].program);
            found++;
        }
    }
    if (!found) printf("No matching students.\n");
}

/* ===================== Enrollment ===================== */

static void enrollStudent(Registrar *r)
{
    Student *s = findStudent(r, readInt("Student ID: ", 1, 999999999));
    if (!s) { printf("Student not found.\n"); return; }

    listCourses(r);
    char code[CODE_LEN];
    readLine("Course code to enroll in: ", code, CODE_LEN);
    Course *c = findCourse(r, code);
    if (!c) { printf("Course not in catalog.\n"); return; }
    if (findEnrollment(s, code)) { printf("Already enrolled in %s.\n", code); return; }

    if (!ensureEnrollCapacity(s)) return;
    s->enrollments[s->enrollCount].course = *c;          /* nested struct copy */
    s->enrollments[s->enrollCount].marks = NOT_GRADED;
    s->enrollCount++;
    printf("%s enrolled in %s.\n", s->name, code);
}

static void dropCourse(Registrar *r)
{
    Student *s = findStudent(r, readInt("Student ID: ", 1, 999999999));
    if (!s) { printf("Student not found.\n"); return; }

    char code[CODE_LEN];
    readLine("Course code to drop: ", code, CODE_LEN);
    for (int i = 0; i < s->enrollCount; i++) {
        if (strcmp(s->enrollments[i].course.code, code) == 0) {
            for (int j = i; j < s->enrollCount - 1; j++)
                s->enrollments[j] = s->enrollments[j + 1];
            s->enrollCount--;
            printf("Course dropped.\n");
            return;
        }
    }
    printf("Student is not enrolled in that course.\n");
}

static void enterMarks(Registrar *r)
{
    Student *s = findStudent(r, readInt("Student ID: ", 1, 999999999));
    if (!s) { printf("Student not found.\n"); return; }

    char code[CODE_LEN];
    readLine("Course code: ", code, CODE_LEN);
    Enrollment *e = findEnrollment(s, code);
    if (!e) { printf("Student is not enrolled in that course.\n"); return; }

    e->marks = readMarks("Marks (0-100): ");
    printf("Marks saved.\n");
}

static void viewTranscript(Registrar *r)
{
    Student *s = findStudent(r, readInt("Student ID: ", 1, 999999999));
    if (!s) { printf("Student not found.\n"); return; }

    printf("\n===== Student Record =====\n");
    printf("ID      : %d\n", s->id);
    printf("Name    : %s\n", s->name);
    printf("DOB     : %02d/%02d/%04d\n", s->dob.day, s->dob.month, s->dob.year);
    printf("Program : %s\n", s->program);

    if (s->enrollCount == 0) { printf("No enrolled courses.\n"); return; }

    printf("\n%-10s %-32s %-7s %-7s %s\n", "Code", "Title", "Credits", "Marks", "Grade");
    printf("---------------------------------------------------------------------\n");

    float totalPoints = 0;
    int totalCredits = 0;
    for (int i = 0; i < s->enrollCount; i++) {
        Enrollment *e = &s->enrollments[i];
        if (e->marks < 0) {
            printf("%-10s %-32s %-7d %-7s %s\n", e->course.code, e->course.title,
                   e->course.creditHours, "-", "-");
        } else {
            char letter[3];
            float gp;
            gradeInfo(e->marks, letter, &gp);
            printf("%-10s %-32s %-7d %-7.1f %s\n", e->course.code, e->course.title,
                   e->course.creditHours, e->marks, letter);
            totalPoints += gp * e->course.creditHours;
            totalCredits += e->course.creditHours;
        }
    }
    if (totalCredits > 0)
        printf("\nGPA (graded courses): %.2f\n", totalPoints / totalCredits);
    else
        printf("\nNo graded courses yet.\n");
}

/* ===================== File processing ===================== */
/*
 * courses.txt  : code|title|creditHours
 * students.txt : id|name|day|month|year|program|enrollCount
 *                (followed by enrollCount lines: code|title|credits|marks)
 */

static int saveData(Registrar *r)
{
    FILE *fc = fopen(COURSE_FILE, "w");
    FILE *fs = fopen(STUDENT_FILE, "w");
    if (!fc || !fs) {
        printf("Error: could not open data files for writing.\n");
        if (fc) fclose(fc);
        if (fs) fclose(fs);
        return 0;
    }

    for (int i = 0; i < r->catCount; i++)
        fprintf(fc, "%s|%s|%d\n", r->catalog[i].code,
                r->catalog[i].title, r->catalog[i].creditHours);

    for (int i = 0; i < r->count; i++) {
        Student *s = &r->students[i];
        fprintf(fs, "%d|%s|%d|%d|%d|%s|%d\n", s->id, s->name, s->dob.day,
                s->dob.month, s->dob.year, s->program, s->enrollCount);
        for (int j = 0; j < s->enrollCount; j++) {
            Enrollment *e = &s->enrollments[j];
            fprintf(fs, "%s|%s|%d|%.2f\n", e->course.code, e->course.title,
                    e->course.creditHours, e->marks);
        }
    }

    fclose(fc);
    fclose(fs);
    printf("Data saved to %s and %s.\n", COURSE_FILE, STUDENT_FILE);
    return 1;
}

static void loadData(Registrar *r)
{
    char line[256];

    FILE *fc = fopen(COURSE_FILE, "r");
    if (fc) {
        while (fgets(line, sizeof line, fc)) {
            Course c;
            line[strcspn(line, "\n")] = '\0';
            if (sscanf(line, "%11[^|]|%59[^|]|%d", c.code, c.title, &c.creditHours) == 3) {
                if (ensureCatalogCapacity(r))
                    r->catalog[r->catCount++] = c;
            }
        }
        fclose(fc);
    }

    FILE *fs = fopen(STUDENT_FILE, "r");
    if (!fs) return;

    while (fgets(line, sizeof line, fs)) {
        Student s;
        int n = 0;
        line[strcspn(line, "\n")] = '\0';
        if (sscanf(line, "%d|%49[^|]|%d|%d|%d|%29[^|]|%d", &s.id, s.name,
                   &s.dob.day, &s.dob.month, &s.dob.year, s.program, &n) != 7)
            continue;

        s.enrollments = NULL;
        s.enrollCount = 0;
        s.enrollCap = 0;

        for (int j = 0; j < n; j++) {
            Enrollment e;
            if (!fgets(line, sizeof line, fs)) break;
            line[strcspn(line, "\n")] = '\0';
            if (sscanf(line, "%11[^|]|%59[^|]|%d|%f", e.course.code,
                       e.course.title, &e.course.creditHours, &e.marks) == 4) {
                if (ensureEnrollCapacity(&s))
                    s.enrollments[s.enrollCount++] = e;
            }
        }

        if (!ensureStudentCapacity(r)) { free(s.enrollments); break; }
        r->students[r->count++] = s;
    }
    fclose(fs);

    printf("Loaded %d student(s) and %d course(s) from file.\n", r->count, r->catCount);
}

/* ===================== Menu ===================== */

static void printMenu(void)
{
    printf("\n========== University Course Registrar ==========\n");
    printf(" 1. Add student            7. Add course to catalog\n");
    printf(" 2. List students          8. List catalog\n");
    printf(" 3. Search student         9. Enrol student in course\n");
    printf(" 4. Update student        10. Drop course\n");
    printf(" 5. Delete student        11. Enter marks\n");
    printf(" 6. View student record   12. Save data\n");
    printf(" 0. Save & exit\n");
    printf("=================================================\n");
}

int main(void)
{
    Registrar reg;
    initRegistrar(&reg);
    loadData(&reg);

    int choice;
    do {
        printMenu();
        choice = readInt("Choice: ", 0, 12);
        switch (choice) {
            case 1:  addStudent(&reg);     break;
            case 2:  listStudents(&reg);   break;
            case 3:  searchStudent(&reg);  break;
            case 4:  updateStudent(&reg);  break;
            case 5:  deleteStudent(&reg);  break;
            case 6:  viewTranscript(&reg); break;
            case 7:  addCourse(&reg);      break;
            case 8:  listCourses(&reg);    break;
            case 9:  enrollStudent(&reg);  break;
            case 10: dropCourse(&reg);     break;
            case 11: enterMarks(&reg);     break;
            case 12: saveData(&reg);       break;
            case 0:  saveData(&reg);       break;
        }
    } while (choice != 0);

    freeRegistrar(&reg);
    printf("Goodbye!\n");
    return 0;
}
