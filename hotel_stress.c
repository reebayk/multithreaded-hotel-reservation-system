/* hotel_fixed.c
 * CORRECTED VERSION — Multithreaded Hotel Reservation System
 * Features: 3 room categories, date-range bookings, cancel, query,
 *           check-in, and a background auditor thread.
 * Compile: gcc -O2 -pthread hotel_fixed.c -o fixed
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>

/* ------------------------------------------------------------------ */
/*  Configuration                                                        */
/* ------------------------------------------------------------------ */
#ifndef NUM_ROOMS
#define NUM_ROOMS      10     /* override with -DNUM_ROOMS=100 */
#endif
#ifndef NUM_CLIENTS
#define NUM_CLIENTS    200    /* override with -DNUM_CLIENTS=10000 */
#endif
#ifndef STAY_US
#define STAY_US          1000   /* simulated work per op, override -DSTAY_US */
#endif
#ifndef ARRIVAL_GAP_US
#define ARRIVAL_GAP_US 500    /* delay between launching clients */
#endif
#ifndef VERBOSE
#define VERBOSE 0             /* override -DVERBOSE=1 for per-op logs */
#endif
/* room category split: 40% standard, 30% deluxe, 30% suite */
#define ROOMS_STD      (NUM_ROOMS * 4 / 10)
#define ROOMS_DELUXE   (NUM_ROOMS * 3 / 10)
#define ROOMS_SUITE    (NUM_ROOMS - ROOMS_STD - ROOMS_DELUXE)

/* ------------------------------------------------------------------ */
/*  Data structures                                                      */
/* ------------------------------------------------------------------ */
typedef enum { STANDARD = 0, DELUXE, SUITE } RoomCategory;
typedef enum { STATUS_FREE = 0, STATUS_RESERVED, STATUS_ACTIVE,
               STATUS_CANCELLED } BookingStatus;

typedef struct {
    int           booking_id;
    int           room_id;
    int           client_id;
    int           checkin;    /* day number  */
    int           checkout;
    int           fare;
    BookingStatus status;
} Booking;

typedef struct {
    int          id;
    RoomCategory category;
} Room;

/* ------------------------------------------------------------------ */
/*  Room definitions (static — no need to lock)                         */
/* ------------------------------------------------------------------ */
static Room ROOMS[NUM_ROOMS];

static void rooms_init(void) {
    int r = 0;
    for (int i = 0; i < ROOMS_STD; i++)    ROOMS[r++] = (Room){r, STANDARD};
    for (int i = 0; i < ROOMS_DELUXE; i++) ROOMS[r++] = (Room){r, DELUXE};
    for (int i = 0; i < ROOMS_SUITE; i++)  ROOMS[r++] = (Room){r, SUITE};
}

static const char* cat_name(RoomCategory c) {
    return c == STANDARD ? "Standard" : c == DELUXE ? "Deluxe" : "Suite";
}

static int base_fare(RoomCategory c) {
    return c == STANDARD ? 100 : c == DELUXE ? 200 : 350;
}

/* ------------------------------------------------------------------ */
/*  Booking database                                                     */
/* ------------------------------------------------------------------ */
#define MAX_BOOKINGS  (NUM_CLIENTS * 2)

static Booking   db[MAX_BOOKINGS];
static int       db_count    = 0;
static int       next_bid    = 1;          /* booking id counter */
static pthread_mutex_t db_mtx = PTHREAD_MUTEX_INITIALIZER;

/* ------------------------------------------------------------------ */
/*  Per-category available-room queues (mutex + semaphore each)         */
/* ------------------------------------------------------------------ */
typedef struct {
    int            q[NUM_ROOMS];
    int            head, tail, size;
    pthread_mutex_t mtx;
    sem_t           sem;
} RoomQueue;

static RoomQueue cat_q[3];   /* index = RoomCategory */

static void rq_init(RoomQueue* rq) {
    rq->head = rq->tail = rq->size = 0;
    pthread_mutex_init(&rq->mtx, NULL);
    sem_init(&rq->sem, 0, 0);
}

static void rq_push(RoomQueue* rq, int rid) {
    rq->q[rq->tail] = rid;
    rq->tail = (rq->tail + 1) % NUM_ROOMS;
    rq->size++;
}

__attribute__((unused))
static int rq_pop(RoomQueue* rq) {
    int r = rq->q[rq->head];
    rq->head = (rq->head + 1) % NUM_ROOMS;
    rq->size--;
    return r;
}

/* ------------------------------------------------------------------ */
/*  Global statistics                                                    */
/* ------------------------------------------------------------------ */
static pthread_mutex_t stats_mtx = PTHREAD_MUTEX_INITIALIZER;
static long total_bookings  = 0;
static long total_cancels   = 0;
static long total_checkins  = 0;
static long total_revenue   = 0;   /* sum of fares for ACTIVE/RESERVED */
static long lifetime_revenue = 0;  /* sum of all fares ever charged */
static pthread_mutex_t wait_mtx = PTHREAD_MUTEX_INITIALIZER;
static double wait_sum_ms = 0.0;
static double wait_max_ms = 0.0;
static long   wait_n = 0;

/* ------------------------------------------------------------------ */
/*  Utility                                                              */
/* ------------------------------------------------------------------ */
static long now_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000L + ts.tv_nsec / 1000;
}

/* ------------------------------------------------------------------ */
/*  OPERATION 1: Book a room                                             */
/*  Returns booking_id on success, -1 if no room in that category.      */
/* ------------------------------------------------------------------ */
static int op_book(int client_id, RoomCategory cat, int checkin, int checkout) {
    int fare = base_fare(cat) * (checkout - checkin) + rand() % 50;

    pthread_mutex_lock(&db_mtx);

    /* Find a room of this category with no overlapping non-cancelled
       booking. The scan and the insertion happen atomically under
       db_mtx, so no other thread can race between the check and the
       reservation (this is what the semaphore/queue version was missing:
       it tracked physical room availability, not date-range conflicts). */
    int rid = -1;
    for (int r = 0; r < NUM_ROOMS; r++) {
        if (ROOMS[r].category != cat) continue;

        bool conflict = false;
        for (int i = 0; i < db_count; i++) {
            if (db[i].room_id != r) continue;
            if (db[i].status == STATUS_CANCELLED) continue;
            if (!(checkout <= db[i].checkin || checkin >= db[i].checkout)) {
                conflict = true;
                break;
            }
        }
        if (!conflict) { rid = r; break; }
    }

    if (rid < 0) {
        pthread_mutex_unlock(&db_mtx);
        return -1;   /* no room in that category free for these dates */
    }

    int bid = next_bid++;
    db[db_count++] = (Booking){bid, rid, client_id, checkin, checkout,
                                fare, STATUS_RESERVED};

    pthread_mutex_lock(&stats_mtx);
    total_bookings++;
    total_revenue += fare;
    lifetime_revenue += fare;
    pthread_mutex_unlock(&stats_mtx);

    pthread_mutex_unlock(&db_mtx);

    if(VERBOSE) printf("[BOOK  ] client=%d room=%d(%s) dates=%d-%d fare=%d bid=%d\n",
           client_id, rid, cat_name(cat), checkin, checkout, fare, bid);
    return bid;
}

/* ------------------------------------------------------------------ */
/*  OPERATION 2: Cancel a booking                                        */
/* ------------------------------------------------------------------ */
static bool op_cancel(int client_id, int booking_id) {
    pthread_mutex_lock(&db_mtx);
    for (int i = 0; i < db_count; i++) {
        if (db[i].booking_id == booking_id &&
            db[i].client_id  == client_id  &&
            db[i].status     == STATUS_RESERVED) {

            db[i].status = STATUS_CANCELLED;
            int rid  = db[i].room_id;
            int fare = db[i].fare;

            pthread_mutex_lock(&stats_mtx);
            total_cancels++;
            total_revenue -= fare;
            pthread_mutex_unlock(&stats_mtx);

            pthread_mutex_unlock(&db_mtx);

            if(VERBOSE) printf("[CANCEL] client=%d bid=%d room=%d\n",
                   client_id, booking_id, rid);
            return true;
        }
    }
    pthread_mutex_unlock(&db_mtx);
    return false;
}

/* ------------------------------------------------------------------ */
/*  OPERATION 3: Query availability for a date range                    */
/* ------------------------------------------------------------------ */
static int op_query(RoomCategory cat, int checkin, int checkout) {
    /* Count rooms of this category not reserved in the overlap window */
    int available = 0;
    pthread_mutex_lock(&db_mtx);
    /* Start from rooms in category, subtract conflicting bookings */
    int total_in_cat = (cat == STANDARD) ? ROOMS_STD :
                       (cat == DELUXE)   ? ROOMS_DELUXE : ROOMS_SUITE;
    int conflict = 0;
    for (int i = 0; i < db_count; i++) {
        if (ROOMS[db[i].room_id].category != cat)     continue;
        if (db[i].status == STATUS_CANCELLED)         continue;
        /* Overlap: not (checkout <= db checkin OR checkin >= db checkout) */
        if (!(checkout <= db[i].checkin || checkin >= db[i].checkout))
            conflict++;
    }
    available = total_in_cat - conflict;
    pthread_mutex_unlock(&db_mtx);

    if(VERBOSE) printf("[QUERY ] cat=%s dates=%d-%d available=%d\n",
           cat_name(cat), checkin, checkout, available);
    return available;
}

/* ------------------------------------------------------------------ */
/*  OPERATION 4: Check-in (mark booking active)                         */
/* ------------------------------------------------------------------ */
static bool op_checkin(int client_id, int booking_id) {
    pthread_mutex_lock(&db_mtx);
    for (int i = 0; i < db_count; i++) {
        if (db[i].booking_id == booking_id &&
            db[i].client_id  == client_id  &&
            db[i].status     == STATUS_RESERVED) {
            db[i].status = STATUS_ACTIVE;
            pthread_mutex_unlock(&db_mtx);

            pthread_mutex_lock(&stats_mtx);
            total_checkins++;
            pthread_mutex_unlock(&stats_mtx);

            if(VERBOSE) printf("[CHECKIN] client=%d bid=%d\n", client_id, booking_id);
            return true;
        }
    }
    pthread_mutex_unlock(&db_mtx);
    return false;
}

/* ------------------------------------------------------------------ */
/*  AUDITOR THREAD                                                       */
/*  Runs continuously; checks for overbooking and revenue integrity.    */
/* ------------------------------------------------------------------ */
#include <stdatomic.h>
static atomic_int auditor_running = 1;
static volatile long audit_violations = 0;

void* auditor(void* arg) {
    (void)arg;
    while (auditor_running) {
        usleep(100000);   /* audit every 100ms */

        pthread_mutex_lock(&db_mtx);

        /* Check overbooking: any two ACTIVE/RESERVED bookings
           on the same room with overlapping dates? */
        for (int i = 0; i < db_count; i++) {
            if (db[i].status == STATUS_CANCELLED) continue;
            for (int j = i + 1; j < db_count; j++) {
                if (db[j].status == STATUS_CANCELLED) continue;
                if (db[i].room_id != db[j].room_id)  continue;
                /* Overlap check */
                if (!(db[j].checkin  >= db[i].checkout ||
                      db[j].checkout <= db[i].checkin)) {
                    audit_violations++;
                    fprintf(stderr,
                        "!! AUDITOR OVERBOOK: room %d bid %d(%d-%d) vs bid %d(%d-%d)\n",
                        db[i].room_id,
                        db[i].booking_id, db[i].checkin, db[i].checkout,
                        db[j].booking_id, db[j].checkin, db[j].checkout);
                }
            }
        }

        /* Revenue integrity: recompute from DB and compare.
           Read total_revenue BEFORE releasing db_mtx (nesting stats_mtx
           inside db_mtx, same order used everywhere else) so no other
           thread can mutate either value between the two reads. */
        long computed = 0;
        for (int i = 0; i < db_count; i++) {
            if (db[i].status == STATUS_RESERVED ||
                db[i].status == STATUS_ACTIVE)
                computed += db[i].fare;
        }
        pthread_mutex_lock(&stats_mtx);
        long reported = total_revenue;
        pthread_mutex_unlock(&stats_mtx);
        pthread_mutex_unlock(&db_mtx);

        if (computed != reported) {
            audit_violations++;
            fprintf(stderr,
                "!! AUDITOR REVENUE MISMATCH: computed=%ld reported=%ld\n",
                computed, reported);
        }
    }
    return NULL;
}

/* ------------------------------------------------------------------ */
/*  CLIENT THREAD — randomly performs all 4 operations                  */
/* ------------------------------------------------------------------ */
void* client(void* arg) {
    int cid = *(int*)arg;
    free(arg);

    int checkin  = 1  + rand() % 25;
    int checkout = checkin + 1 + rand() % 5;
    RoomCategory cat = (RoomCategory)(rand() % 3);

    long t0 = now_us();
    int bid = op_book(cid, cat, checkin, checkout);
    long t1 = now_us();
    double wait_ms = (t1 - t0) / 1000.0;
    pthread_mutex_lock(&wait_mtx);
    wait_sum_ms += wait_ms;
    if (wait_ms > wait_max_ms) wait_max_ms = wait_ms;
    wait_n++;
    pthread_mutex_unlock(&wait_mtx);

    if (bid < 0) return NULL;   /* no room available in that category */

    usleep(STAY_US);

    int action = rand() % 3;
    if (action == 0) {
        /* Cancel the booking */
        op_cancel(cid, bid);
    } else if (action == 1) {
        /* Query availability then check in */
        op_query(cat, checkin, checkout);
        usleep(500);
        op_checkin(cid, bid);
        usleep(5000 + rand() % 5000);
        /* After stay, room is "checked out" — free it and remove its fare */
        pthread_mutex_lock(&db_mtx);
        for (int i = 0; i < db_count; i++) {
            if (db[i].booking_id == bid) {
                db[i].status = STATUS_FREE;
                pthread_mutex_lock(&stats_mtx);
                total_revenue -= db[i].fare;
                pthread_mutex_unlock(&stats_mtx);
                break;
            }
        }
        pthread_mutex_unlock(&db_mtx);
    } else {
        /* Just check in (stay then release) */
        op_checkin(cid, bid);
        usleep(5000 + rand() % 5000);

        pthread_mutex_lock(&db_mtx);
        for (int i = 0; i < db_count; i++) {
            if (db[i].booking_id == bid) {
                db[i].status = STATUS_FREE;
                pthread_mutex_lock(&stats_mtx);
                total_revenue -= db[i].fare;
                pthread_mutex_unlock(&stats_mtx);
                break;
            }
        }
        pthread_mutex_unlock(&db_mtx);
    }
    return NULL;
}

/* ------------------------------------------------------------------ */
/*  main                                                                 */
/* ------------------------------------------------------------------ */
int main(void) {
    srand(time(NULL));
    rooms_init();

    /* Initialize per-category queues */
    for (int c = 0; c < 3; c++) rq_init(&cat_q[c]);
    for (int i = 0; i < NUM_ROOMS; i++) {
        RoomCategory c = ROOMS[i].category;
        pthread_mutex_lock(&cat_q[c].mtx);
        rq_push(&cat_q[c], i);
        pthread_mutex_unlock(&cat_q[c].mtx);
        sem_post(&cat_q[c].sem);
    }

    /* Start auditor */
    pthread_t aud;
    pthread_create(&aud, NULL, auditor, NULL);

    /* Launch clients */
    pthread_t* th = malloc(sizeof(pthread_t) * NUM_CLIENTS);
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 256 * 1024); /* 256KB, default 8MB wastes RAM at scale */
    long t0 = now_us();
    for (int i = 0; i < NUM_CLIENTS; i++) {
        int* p = malloc(sizeof(int)); *p = i;
        pthread_create(&th[i], &attr, client, p);
        usleep(ARRIVAL_GAP_US);
    }
    for (int i = 0; i < NUM_CLIENTS; i++) pthread_join(th[i], NULL);
    pthread_attr_destroy(&attr);
    free(th);
    long t1 = now_us();

    auditor_running = 0;
    pthread_join(aud, NULL);

    double wall_s  = (t1 - t0) / 1e6;
    double avg_ms  = wait_n ? wait_sum_ms / wait_n : 0.0;
    double tput    = wall_s > 0 ? total_bookings / wall_s : 0.0;

    fprintf(stderr, "\n===== SUMMARY =====\n");
    fprintf(stderr, "Bookings  : %ld\n",  total_bookings);
    fprintf(stderr, "Cancels   : %ld\n",  total_cancels);
    fprintf(stderr, "Check-ins : %ld\n",  total_checkins);
    fprintf(stderr, "Revenue   : %ld\n",  total_revenue);
    fprintf(stderr, "Lifetime$ : %ld\n",  lifetime_revenue);
    fprintf(stderr, "Wall time : %.2f s\n", wall_s);
    fprintf(stderr, "Audit violations: %ld\n", audit_violations);

    /* CSV row: rooms,clients,op_us,gap_us,wall_s,avg_ms,max_ms,tput,violations */
    printf("%d,%d,%d,%d,%.2f,%.3f,%.3f,%.1f,%ld\n",
           NUM_ROOMS, NUM_CLIENTS, STAY_US, ARRIVAL_GAP_US,
           wall_s, avg_ms, wait_max_ms, tput, audit_violations);

    /* Cleanup */
    for (int c = 0; c < 3; c++) {
        sem_destroy(&cat_q[c].sem);
        pthread_mutex_destroy(&cat_q[c].mtx);
    }
    return (audit_violations == 0) ? 0 : 1;
}
