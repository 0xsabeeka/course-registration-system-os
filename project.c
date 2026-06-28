#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

// CONSTANTS
#define maxCourses     10
#define maxStudents    50
#define highPriority   1
#define normalPriority 0

// Course structure (contains course details)
typedef struct {
    char courseID[10];
    char courseName[50];
    int totalSeats;
    int availableSeats;
    pthread_mutex_t lock;   // every course has its own mutex lock
} Course;

// Student structure (conatains student details)
typedef struct {
    int studentId;
    int courseIndex;
    int priority;
    int registrationResult;
} Student;

// Global variables
Course courses[maxCourses];
Student students[maxStudents];

int totalSuccess = 0;
int totalFailed = 0;

FILE* logFile;

// mutex to protect totalSuccess and totalFailed counters
pthread_mutex_t counterLock = PTHREAD_MUTEX_INITIALIZER;


// MODULE 1: DATA STRUCTURES AND INITIALIZATION

/*
    Function: initializeCourses
    Purpose:
    Initializes course data with course ID, course name,
    total seats, available seats, and mutex lock for each course.
    It also checks that course count does not exceed the maximum limit.
*/
void initializeCourses(int courseCount) {
    if (courseCount > maxCourses) {
        printf("Course count cannot be more than %d\n", maxCourses);
        exit(1);
    }

    char* ids[] = {
    "CS101", "CS102", "CS103", "CS104", "CS105",
    "CS106", "CS107", "CS108", "CS109", "CS110"
};

    char* names[] = {
    "Operating Systems",
    "Data Structures",
    "Algorithms",
    "Computer Networks",
    "Database Systems",
    "Software Engineering",
    "Artificial Intelligence",
    "Machine Learning",
    "Cyber Security",
    "Cloud Computing"
};

    int seats[] = {2, 1, 3, 6, 4, 5, 3, 2, 4, 5};

    for (int i = 0; i < courseCount; i++) {
        strcpy(courses[i].courseID, ids[i]);
        strcpy(courses[i].courseName, names[i]);

        courses[i].totalSeats = seats[i];
        courses[i].availableSeats = seats[i];

        // Initialize mutex lock for this course
        // This lock will protect availableSeats during registration
        pthread_mutex_init(&courses[i].lock, NULL);
    }
}


/*
    Function: initializeStudents
    Purpose:
    Initializes student registration requests.
    Each student is given an ID, random course choice,
    priority level, and initial registration result.
    High-priority students are assigned first according to highPriorityCount.
*/
void initializeStudents(int studentCount, int courseCount, int highPriorityCount) {
    if (studentCount > maxStudents) {
        printf("Student count cannot be more than %d\n", maxStudents);
        exit(1);
    }

    // Seed random number generator so course assignment changes in every run
    srand(time(NULL));

    for (int i = 0; i < studentCount; i++) {
        students[i].studentId = i + 1;

        // Each student randomly chooses one course
        students[i].courseIndex = rand() % courseCount;

        // 0 means not registered yet / failed by default
        students[i].registrationResult = 0;

        // First few students are high priority, rest are normal priority
        if (i < highPriorityCount)
            students[i].priority = highPriority;
        else
            students[i].priority = normalPriority;
    }
}


// MODULE 6: LOGGING

/*
    Function: logEntry
    Purpose:
    Prints one registration attempt on the terminal and writes
    the same entry into the log file.
    The log contains timestamp, student ID, course ID,
    course name, priority level, and registration result.
*/
void logEntry(int studentId, char courseID[], char courseName[], int priority, int result) {
    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    char timeStr[10];

    // Convert current time into HH:MM:SS format
    strftime(timeStr, sizeof(timeStr), "%H:%M:%S", t);

    char priorityText[10];

    if (priority == highPriority)
        strcpy(priorityText, "HIGH");
    else
        strcpy(priorityText, "NORMAL");

    char resultText[20];

    if (result == 1)
        strcpy(resultText, "SUCCESS");
    else
        strcpy(resultText, "FAILED - No Seats");

    // Print registration attempt on terminal
    printf("[%s] Student %-3d | Course: %-6s | %-22s | Priority: %-6s | %s\n",
           timeStr, studentId, courseID, courseName, priorityText, resultText);

    // save same registration attempt in log file
    if (logFile != NULL) {
        fprintf(logFile, "[%s] Student %-3d | Course: %-6s | %-22s | Priority: %-6s | %s\n",
                timeStr, studentId, courseID, courseName, priorityText, resultText);

        // fflush saves log immediately instead of waiting until program ends
        fflush(logFile);
    }

    fflush(stdout);
}


// MODULE 2, 3, 4: THREAD FUNCTION, SYNCHRONIZATION, PRIORITY HANDLING

/*
    Function: studentThreadFunction
    Purpose:
    This function is executed by every student thread.
    It applies priority handling, locks the requested course,
    checks available seats, updates the seat count safely,
    logs the result, and updates success or failure counters.
*/
void* studentThreadFunction(void* arg) {

    Student* student = (Student*) arg;

    // Course index tells which course this student wants
    int index = student->courseIndex;

    // PRIORITY HANDLING:
    // Normal-priority students wait a little.
    // High-priority students continue immediately.
    // This gives preference to final-year/high-priority students.
    if (student->priority == normalPriority)
        usleep(100000);

    // SYNCHRONIZATION:
    // Lock the requested course before checking or updating seats.
    // This prevents multiple threads from changing same course seats together.
    pthread_mutex_lock(&courses[index].lock);

    // CRITICAL SECTION STARTS HERE
    // Only one thread can enter this section for the same course at a time.
    if (courses[index].availableSeats > 0) {
        courses[index].availableSeats--;      // allocate one seat
        student->registrationResult = 1;      // 1 means registration successful

        logEntry(student->studentId, courses[index].courseID, courses[index].courseName, student->priority, 1);

        // Counter lock protects totalSuccess from race condition
        pthread_mutex_lock(&counterLock);
        totalSuccess++;
        pthread_mutex_unlock(&counterLock);

    } else {
        student->registrationResult = 0;      // registration failed

        logEntry(student->studentId, courses[index].courseID, courses[index].courseName, student->priority, 0);

        pthread_mutex_lock(&counterLock);
        totalFailed++;
        pthread_mutex_unlock(&counterLock);
    }
    // CRITICAL SECTION ENDS HERE

    // Unlock course so other waiting students can try registration
    pthread_mutex_unlock(&courses[index].lock);

    return NULL;
}


// MODULE 6: FINAL SUMMARY

/*
    Function: printFinalSummary
    Purpose:
    Displays the final seat allocation of all courses.
    It also prints total students, successful registrations,
    failed registrations, and success rate.
    The same summary is also written into the log file.
*/
void printFinalSummary(int studentCount, int courseCount) {
    printf("\n");
    printf("================================================\n");
    printf("              FINAL SUMMARY                     \n");
    printf("================================================\n");
    printf("%-6s | %-22s | Total Seats | Remaining | Registered\n",
           "ID", "Course Name");
    printf("------------------------------------------------\n");

    for (int i = 0; i < courseCount; i++) {
        int registered = courses[i].totalSeats - courses[i].availableSeats;

        printf("%-6s | %-22s | %-11d | %-9d | %d\n",
               courses[i].courseID,
               courses[i].courseName,
               courses[i].totalSeats,
               courses[i].availableSeats,
               registered);
    }

    printf("------------------------------------------------\n");
    printf("Total Students    : %d\n", studentCount);
    printf("Total Successful  : %d\n", totalSuccess);
    printf("Total Failed      : %d\n", totalFailed);

    float successRate = 0.0;

    if ((totalSuccess + totalFailed) > 0)
        successRate = (totalSuccess * 100.0) / (totalSuccess + totalFailed);

    printf("Success Rate      : %.1f%%\n", successRate);

    // Write final summary to log file also
    if (logFile != NULL) {
        fprintf(logFile, "\n================================================\n");
        fprintf(logFile, "              FINAL SUMMARY                     \n");
        fprintf(logFile, "================================================\n");
        fprintf(logFile, "%-6s | %-22s | Total Seats | Remaining | Registered\n",
                "ID", "Course Name");
        fprintf(logFile, "------------------------------------------------\n");

        for (int i = 0; i < courseCount; i++) {
            int registered = courses[i].totalSeats - courses[i].availableSeats;

            fprintf(logFile, "%-6s | %-22s | %-11d | %-9d | %d\n",
                    courses[i].courseID,
                    courses[i].courseName,
                    courses[i].totalSeats,
                    courses[i].availableSeats,
                    registered);
        }

        fprintf(logFile, "------------------------------------------------\n");
        fprintf(logFile, "Total Students    : %d\n", studentCount);
        fprintf(logFile, "Total Successful  : %d\n", totalSuccess);
        fprintf(logFile, "Total Failed      : %d\n", totalFailed);
        fprintf(logFile, "Success Rate      : %.1f%%\n", successRate);
    }
}


/*
    Function: runTest
    Purpose:
    Creates one thread for each student request.
    It starts all student threads, waits for all threads to finish
    using pthread_join, and then prints the final summary.
*/
void runTest(int studentCount, int courseCount) {
    pthread_t threads[maxStudents];

    totalSuccess = 0;
    totalFailed = 0;

    // THREAD CREATION:
    // One thread is created for each student request.
    // All threads run studentThreadFunction concurrently.
    for (int i = 0; i < studentCount; i++) {
        pthread_create(&threads[i], NULL, studentThreadFunction, &students[i]);
    }

    // THREAD JOINING:
    // Main program waits until every student thread finishes.
    // This ensures all threads terminate cleanly before final summary.
    for (int i = 0; i < studentCount; i++) {
        pthread_join(threads[i], NULL);
    }

    printFinalSummary(studentCount, courseCount);
}


/*
    Function: destroyCourseLocks
    Purpose:
    Destroys the mutex locks of all initialized courses after a test ends.
    This keeps the program clean before another test initializes courses again.
*/
void destroyCourseLocks(int courseCount) {
    for (int i = 0; i < courseCount; i++) {
        pthread_mutex_destroy(&courses[i].lock);
    }
}


/*
    Function: runMandatoryTest
    Purpose:
    Runs the required mandatory test scenario.
    It uses 3 courses, 10 student threads,
    and 3 high-priority students as required in the project statement.
*/
void runMandatoryTest() {
    printf("\n================================================\n");
    printf("         MANDATORY TEST SCENARIO                \n");
    printf("        (10 Students, 3 Courses)                \n");
    printf("================================================\n");

    fprintf(logFile, "\n================================================\n");
    fprintf(logFile, "         MANDATORY TEST SCENARIO                \n");
    fprintf(logFile, "================================================\n");

    int courseCount = 3;
    int studentCount = 10;
    int highPriorityCount = 3;

    initializeCourses(courseCount);
    initializeStudents(studentCount, courseCount, highPriorityCount);

    runTest(studentCount, courseCount);

    destroyCourseLocks(courseCount);
}


/*
    Function: runStressTest
    Purpose:
    Runs a larger stress test scenario.
    It uses 6 courses and 60 student threads to test
    synchronization under higher concurrency.
*/
void runStressTest() {
    printf("\n================================================\n");
    printf("           STRESS TEST SCENARIO                 \n");
    printf("        (50 Students, 10 Courses)                \n");
    printf("================================================\n");

    fprintf(logFile, "\n================================================\n");
    fprintf(logFile, "           STRESS TEST SCENARIO                 \n");
    fprintf(logFile, "================================================\n");

    int courseCount = 10;
    int studentCount = 50;
    int highPriorityCount = 15;

    initializeCourses(courseCount);
    initializeStudents(studentCount, courseCount, highPriorityCount);

    runTest(studentCount, courseCount);

    destroyCourseLocks(courseCount);
}


/*
    Function: main
    Purpose:
    Starts the university course registration simulation.
    It opens the log file, runs mandatory and stress tests,
    destroys remaining mutex resources, closes the log file,
    and ends the program.
*/
int main() {
    // Open log file in write mode.
    // If file already exists, old content is replaced.
    logFile = fopen("log.txt", "w");

    if (logFile == NULL) {
        printf("Could not open log file. Exiting.\n");
        return 1;
    }

    printf("================================================\n");
    printf("   UNIVERSITY COURSE REGISTRATION SYSTEM        \n");
    printf("================================================\n");

    fprintf(logFile, "================================================\n");
    fprintf(logFile, "   UNIVERSITY COURSE REGISTRATION SYSTEM        \n");
    fprintf(logFile, "================================================\n");

    runMandatoryTest();
    runStressTest();

    // Destroy counter mutex after all thread work is completed
    pthread_mutex_destroy(&counterLock);

    fclose(logFile);

    printf("Log saved to log.txt\n");

    return 0;
}