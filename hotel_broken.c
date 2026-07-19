/* hotel_broken.c
 * INTENTIONALLY BROKEN VERSION — for demonstration only.
 * Compile: gcc -O0 -pthread hotel_broken.c -o broken
 *
 * THREE deliberate bugs — same as before, now with room categories
 * and date-range bookings to match the full hotel server design.
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>

#define NUM_ROOMS      10
#define NUM_CLIENTS    200

typedef enum { STANDARD, DELUXE, SUITE } RoomCategory;

typedef struct {
    int          id;
    RoomCategory category;
    bool         available;
    int          booked_by;   /* client id currently occupying */
} Room;

/* Shared state — accessed by all threads with NO protection. */
Room rooms[NUM_ROOMS];
int  total_bookings = 0;
long total_revenue  = 0;

static const char* cat_name(RoomCategory c) {
    return c == STANDARD ? "Standard" : c == DELUXE ? "Deluxe" : "Suite";
}

/*
 * BUG 1: Check-then-act on rooms[i].available is not atomic.
 *        Two threads can both see available==true and double-book the room.
 *
 * BUG 2: total_bookings read-modify-write is split across usleep(1),
 *        causing lost updates.
 *
 * BUG 3: while(1) busy-wait wastes CPU and widens the race window.
 */
void* client(void* arg) {
    int cid = *(int*)arg;
    free(arg);

    /* Simulate a requested check-in/check-out (just for display) */
    int checkin  = 1 + rand() % 28;
    int checkout = checkin + 1 + rand() % 5;

    while (1) {
        for (int i = 0; i < NUM_ROOMS; i++) {
            if (rooms[i].available) {                   /* BUG 1 — race */
                rooms[i].available = false;             /* BUG 1 — race */
                rooms[i].booked_by = cid;
                printf("Client %3d -> Room %d (%s) dates %d-%d\n",
                       cid, rooms[i].id, cat_name(rooms[i].category),
                       checkin, checkout);

                usleep(50000);   /* simulate stay */

                /* BUG 2: read-modify-write with artificial gap */
                int old = total_bookings;
                usleep(1);
                total_bookings = old + 1;

                long old_rev = total_revenue;
                usleep(1);
                int fare = (rooms[i].category == SUITE)   ? 300 :
                           (rooms[i].category == DELUXE)  ? 200 : 100;
                total_revenue = old_rev + fare + rand() % 50;

                rooms[i].available = true;
                rooms[i].booked_by = -1;
                return NULL;
            }
        }
        usleep(500);   /* BUG 3 — busy wait */
    }
}

int main(void) {
    srand((unsigned)time(NULL) ^ (unsigned)clock());

    /* Assign categories: first third standard, middle deluxe, rest suite */
    for (int i = 0; i < NUM_ROOMS; i++) {
        rooms[i].id        = i;
        rooms[i].available = true;
        rooms[i].booked_by = -1;
        rooms[i].category  = (i < NUM_ROOMS/3) ? STANDARD :
                             (i < 2*NUM_ROOMS/3) ? DELUXE : SUITE;
    }

    pthread_t th[NUM_CLIENTS];
    for (int i = 0; i < NUM_CLIENTS; i++) {
        int* p = malloc(sizeof(int));
        *p = i;
        pthread_create(&th[i], NULL, client, p);
    }
    for (int i = 0; i < NUM_CLIENTS; i++) pthread_join(th[i], NULL);

    printf("\nTotal bookings : %d  (expected %d)\n", total_bookings, NUM_CLIENTS);
    printf("Total revenue  : %ld\n", total_revenue);
    printf("Match          : %s\n",
           total_bookings == NUM_CLIENTS ? "YES" : "NO — LOST UPDATES DETECTED");
    return 0;
}
