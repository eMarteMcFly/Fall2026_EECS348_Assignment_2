/********************************************************************************
 * Name of Program: EECS 348 Assignment 2 - CEO Email Priority Queue
 * Brief Description: Implements a priority queue using a list-based Max Heap 
 *                    to organize and prioritize emails for a company CEO. 
 *                    Emails are prioritized by sender category (Boss > Subordinate 
 *                    > Peer > Important Person > Other Person) and then by date 
 *                    (newest emails first). Handles commands: EMAIL, NEXT, READ, 
 *                    and COUNT while robustly handling invalid or malformed inputs.
 * Inputs: Standard input (stdin) containing commands (EMAIL, NEXT, READ, COUNT) 
 *         and comma-separated email details.
 * Output: Standard output (stdout) detailing email counts and peak email details.
 * All Collaborators: None
 * Other Sources: Standard C library documentation
 * Author's Full Name: [Your Full Name]
 * Creation Date: September 17, 2026
 * Revision Date: September 17, 2026
 * Revisions: Initial C implementation with error handling and heap management.
 ********************************************************************************/

#include <stdio.h>   /* Standard I/O operations */
#include <stdlib.h>  /* Dynamic memory management routines */
#include <string.h>  /* String manipulation functions */
#include <ctype.h>   /* Character handling functions */

#define INITIAL_CAPACITY 10 /* Initial heap array capacity */
#define MAX_LINE_LEN 512    /* Maximum length of an input line */

/* Structure representing an email record */
typedef struct {
    char sender[64];        /* Category of sender */
    char subject[256];      /* Subject line of the email */
    char date[16];          /* String representation of date (MM-DD-YYYY) */
    int year;               /* Parsed year for comparison */
    int month;              /* Parsed month for comparison */
    int day;                /* Parsed day for comparison */
    int category_priority;  /* Priority level calculated from sender category */
    int sequence_id;        /* Order of arrival for tie-breaking */
} Email;

/* Structure representing the dynamic array-based Max Heap */
typedef struct {
    Email *data;            /* Dynamic array holding email elements */
    int size;               /* Current number of emails in heap */
    int capacity;           /* Max capacity before memory reallocation */
} MaxHeap;

/* 
 * Function Prototypes
 * Author: Authored by Student
 */
MaxHeap* create_heap(void);
void destroy_heap(MaxHeap *heap);
void heap_insert(MaxHeap *heap, Email email);
Email heap_extract_max(MaxHeap *heap);
Email heap_peek_max(const MaxHeap *heap);
int compare_emails(const Email *e1, const Email *e2);
int get_category_priority(const char *category);
void trim_whitespace(char *str);
int parse_date(const char *date_str, int *m, int *d, int *y);
void handle_email_command(MaxHeap *heap, char *args, int sequence_counter);

/*
 * Block: Utility Helper Functions
 * Author: Authored by Student
 */

/* Trim leading and trailing whitespace characters in-place */
void trim_whitespace(char *str) {
    if (str == NULL) return; /* Handle null pointer guard */
    char *start = str; /* Pointer to start of trimmed string */
    while (isspace((unsigned char)*start)) start++; /* Skip leading spaces */
    if (*start == '\0') { /* Check if string is all spaces */
        *str = '\0'; /* Set output to empty string */
        return; /* Exit function */
    }
    char *end = start + strlen(start) - 1; /* Pointer to end of string */
    while (end > start && isspace((unsigned char)*end)) end--; /* Trim trailing spaces */
    *(end + 1) = '\0'; /* Null terminate trimmed string */
    if (start != str) { /* Shift content if leading spaces were removed */
        memmove(str, start, strlen(start) + 1); /* Copy string to beginning */
    }
}

/* Convert sender category string into numerical priority value */
int get_category_priority(const char *category) {
    if (category == NULL) return 0; /* Guard against NULL input */
    if (strcasecmp(category, "Boss") == 0) return 5; /* Highest priority */
    if (strcasecmp(category, "Subordinate") == 0) return 4; /* Second priority */
    if (strcasecmp(category, "Peer") == 0) return 3; /* Third priority */
    if (strcasecmp(category, "Important Person") == 0 || strcasecmp(category, "ImportantPerson") == 0) return 2; /* Fourth priority */
    if (strcasecmp(category, "Other Person") == 0 || strcasecmp(category, "OtherPerson") == 0) return 1; /* Lowest priority */
    return 0; /* Return 0 for invalid or unrecognized categories */
}

/* Parse date string MM-DD-YYYY into integer components */
int parse_date(const char *date_str, int *m, int *d, int *y) {
    if (date_str == NULL || m == NULL || d == NULL || y == NULL) return 0; /* Guard against null pointers */
    if (sscanf(date_str, "%d-%d-%d", m, d, y) != 3) { /* Extract month, day, year */
        return 0; /* Return failure if format does not match */
    }
    if (*m < 1 || *m > 12 || *d < 1 || *d > 31 || *y < 1900) { /* Basic sanity validation */
        return 0; /* Return failure if out of valid ranges */
    }
    return 1; /* Return success */
}

/* Compare priority of two emails (Returns positive if e1 > e2, negative if e1 < e2) */
int compare_emails(const Email *e1, const Email *e2) {
    /* Step 1: Compare category priority */
    if (e1->category_priority != e2->category_priority) {
        return e1->category_priority - e2->category_priority; /* Higher numerical value wins */
    }
    /* Step 2: Compare year (newer date wins) */
    if (e1->year != e2->year) {
        return e1->year - e2->year; /* Larger year is newer */
    }
    /* Step 3: Compare month (newer date wins) */
    if (e1->month != e2->month) {
        return e1->month - e2->month; /* Larger month is newer */
    }
    /* Step 4: Compare day (newer date wins) */
    if (e1->day != e2->day) {
        return e1->day - e2->day; /* Larger day is newer */
    }
    /* Step 5: Tie-breaker using arrival sequence order (newer arrival wins) */
    return e1->sequence_id - e2->sequence_id;
}

/*
 * Block: Max Heap Construction & Operations
 * Author: Authored by Student
 */

/* Allocate and initialize dynamic heap */
MaxHeap* create_heap(void) {
    MaxHeap *heap = (MaxHeap*) malloc(sizeof(MaxHeap)); /* Allocate heap control structure */
    if (!heap) { /* Verify memory allocation succeeded */
        fprintf(stderr, "Error: Memory allocation failed for MaxHeap.\n"); /* Output memory error */
        exit(EXIT_FAILURE); /* Terminate execution */
    }
    heap->capacity = INITIAL_CAPACITY; /* Set default capacity */
    heap->size = 0; /* Set initial element count to zero */
    heap->data = (Email*) malloc(sizeof(Email) * heap->capacity); /* Allocate underlying data array */
    if (!heap->data) { /* Verify dynamic array allocation succeeded */
        fprintf(stderr, "Error: Memory allocation failed for Heap Data.\n"); /* Output memory error */
        free(heap); /* Free allocated control struct */
        exit(EXIT_FAILURE); /* Terminate execution */
    }
    return heap; /* Return created heap pointer */
}

/* Free allocated heap memory */
void destroy_heap(MaxHeap *heap) {
    if (heap) { /* Guard against NULL pointer */
        if (heap->data) { /* Check if dynamic data array exists */
            free(heap->data); /* Release data array */
        }
        free(heap); /* Release control structure */
    }
}

/* Insert email into Max Heap */
void heap_insert(MaxHeap *heap, Email email) {
    if (heap->size >= heap->capacity) { /* Check if expansion is needed */
        heap->capacity *= 2; /* Double dynamic array capacity */
        Email *temp = (Email*) realloc(heap->data, sizeof(Email) * heap->capacity); /* Reallocate memory */
        if (!temp) { /* Validate reallocation success */
            fprintf(stderr, "Error: Reallocation failed during heap insert.\n"); /* Print error */
            return; /* Abort insertion safely */
        }
        heap->data = temp; /* Update data pointer to new buffer */
    }

    int current = heap->size; /* Index where new element is added */
    heap->data[current] = email; /* Insert element at end of heap array */
    heap->size++; /* Increment size counter */

    /* Heapify Up to maintain Max Heap invariant */
    while (current > 0) {
        int parent = (current - 1) / 2; /* Calculate parent index */
        if (compare_emails(&heap->data[current], &heap->data[parent]) > 0) { /* If child > parent */
            Email temp_email = heap->data[current]; /* Swap element with parent */
            heap->data[current] = heap->data[parent]; /* Move parent down */
            heap->data[parent] = temp_email; /* Move child up */
            current = parent; /* Move pointer to parent index */
        } else {
            break; /* Heap property satisfied, stop percolating up */
        }
    }
}

/* Extract max priority element from heap */
Email heap_extract_max(MaxHeap *heap) {
    Email max_item = heap->data[0]; /* Store root element */
    heap->data[0] = heap->data[heap->size - 1]; /* Move last element to root */
    heap->size--; /* Decrement size counter */

    int current = 0; /* Start down-heap percolation from root */
    while (1) {
        int left = 2 * current + 1; /* Left child index */
        int right = 2 * current + 2; /* Right child index */
        int largest = current; /* Assume current node is largest */

        if (left < heap->size && compare_emails(&heap->data[left], &heap->data[largest]) > 0) {
            largest = left; /* Left child is larger */
        }
        if (right < heap->size && compare_emails(&heap->data[right], &heap->data[largest]) > 0) {
            largest = right; /* Right child is larger */
        }

        if (largest != current) { /* Swap if child is larger */
            Email temp = heap->data[current]; /* Temporary hold for swap */
            heap->data[current] = heap->data[largest]; /* Assign child to current */
            heap->data[largest] = temp; /* Assign current to child */
            current = largest; /* Move down tree to largest child position */
        } else {
            break; /* Heap order restored, break loop */
        }
    }
    return max_item; /* Return extracted highest priority email */
}

/* Peek top priority email without removing */
Email heap_peek_max(const MaxHeap *heap) {
    return heap->data[0]; /* Return root element */
}

/*
 * Block: Command Parsing and Logic Processing
 * Author: Authored by Student
 */

/* Parse and validate EMAIL command arguments */
void handle_email_command(MaxHeap *heap, char *args, int sequence_counter) {
    if (args == NULL || strlen(args) == 0) { /* Check for empty arguments */
        fprintf(stderr, "Error: Malformed EMAIL line.\n"); /* Output warning */
        return; /* Skip line processing */
    }

    char *cat_token = strtok(args, ","); /* Tokenize category string */
    char *subj_token = strtok(NULL, ","); /* Tokenize subject string */
    char *date_token = strtok(NULL, ","); /* Tokenize date string */

    if (!cat_token || !subj_token || !date_token) { /* Verify three comma-separated fields exist */
        fprintf(stderr, "Error: Incorrect EMAIL format. Expected: EMAIL <category>, <subject>, <date>\n");
        return; /* Skip malformed command */
    }

    trim_whitespace(cat_token); /* Clean sender category string */
    trim_whitespace(subj_token); /* Clean subject line string */
    trim_whitespace(date_token); /* Clean date string */

    int priority = get_category_priority(cat_token); /* Map category string to priority level */
    if (priority == 0) { /* Guard against invalid category */
        fprintf(stderr, "Error: Invalid sender category '%s'.\n", cat_token); /* Log category error */
        return; /* Ignore invalid email entry */
    }

    int m, d, y; /* Variables for date parsing */
    if (!parse_date(date_token, &m, &d, &y)) { /* Validate date format */
        fprintf(stderr, "Error: Invalid date format '%s'. Must be MM-DD-YYYY.\n", date_token); /* Log date error */
        return; /* Ignore invalid date entry */
    }

    Email email; /* Construct new Email record */
    strncpy(email.sender, cat_token, sizeof(email.sender) - 1); /* Copy sender category */
    email.sender[sizeof(email.sender) - 1] = '\0'; /* Ensure string termination */
    strncpy(email.subject, subj_token, sizeof(email.subject) - 1); /* Copy subject line */
    email.subject[sizeof(email.subject) - 1] = '\0'; /* Ensure string termination */
    strncpy(email.date, date_token, sizeof(email.date) - 1); /* Copy date string */
    email.date[sizeof(email.date) - 1] = '\0'; /* Ensure string termination */
    
    email.month = m; /* Assign parsed month */
    email.day = d; /* Assign parsed day */
    email.year = y; /* Assign parsed year */
    email.category_priority = priority; /* Assign numerical priority */
    email.sequence_id = sequence_counter; /* Track insertion sequence for stability */

    heap_insert(heap, email); /* Insert newly created email into max heap */
}

/*
 * Block: Program Entry Point
 * Author: Authored by Student
 */
int main(void) {
    MaxHeap *heap = create_heap(); /* Instantiate new max heap priority queue */
    char line[MAX_LINE_LEN]; /* Buffer to hold standard input line */
    int sequence_counter = 0; /* Global insertion sequence counter */

    /* Read lines from standard input until End-Of-File (EOF) */
    while (fgets(line, sizeof(line), stdin) != NULL) {
        trim_whitespace(line); /* Trim leading and trailing whitespace */
        if (strlen(line) == 0) continue; /* Skip blank empty lines */

        if (strncmp(line, "EMAIL", 5) == 0) { /* Check if line starts with EMAIL */
            sequence_counter++; /* Increment order counter for new email */
            handle_email_command(heap, line + 5, sequence_counter); /* Process EMAIL arguments */
        } 
        else if (strcmp(line, "COUNT") == 0) { /* Check for COUNT command */
            printf("There are %d emails to read.\n", heap->size); /* Display current count */
        } 
        else if (strcmp(line, "NEXT") == 0) { /* Check for NEXT command */
            if (heap->size > 0) { /* Check if queue has emails */
                Email top = heap_peek_max(heap); /* Look at top element */
                printf("Next email:\n"); /* Header output */
                printf("Sender: %s\n", top.sender); /* Output sender category */
                printf("Subject: %s\n", top.subject); /* Output subject text */
                printf("Date: %s\n", top.date); /* Output email date string */
            } else {
                printf("No emails to read.\n"); /* Gracefully handle empty queue peek */
            }
        } 
        else if (strcmp(line, "READ") == 0) { /* Check for READ command */
            if (heap->size > 0) { /* Check if queue has emails */
                heap_extract_max(heap); /* Remove highest priority email without printing */
            } else {
                /* Handled silently or safely when no emails exist */
            }
        } 
        else {
            fprintf(stderr, "Error: Unrecognized command line: '%s'\n", line); /* Handle unknown command */
        }
    }

    destroy_heap(heap); /* Deallocate heap buffer before exiting */
    return 0; /* Return exit code success */
}