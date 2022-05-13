#include"elevator.h"
#include<unistd.h>
#include<stdlib.h>
#include<stdio.h>
#include<pthread.h>
#include <assert.h>

struct Passenger_info {
    int id;
    int from_floor;
    int to_floor;
    int elenumber;
    int status;
    struct Passenger_info* next;
} passengers[PASSENGERS];

struct Elevator_info {
	int elevator_floor;	
	int open;
	int passenger_count;
	int trips;
    pthread_mutex_t elevator_lock;
    pthread_mutex_t passenger_lock;
    int elevator_direction;
    int wait_at;
    int is_elevator_ready;
    int is_passenger_ready;
    struct Passenger_info* riding;
    int start;
    int waitlist[PASSENGERS];
    int wait_count;
    int wait_num;
    int ridelist[3];
    int next_wait;
} elevators[ELEVATORS];

// pthread_mutex_t passenger_lock;
// pthread_mutex_t elevator_lock;
// int elevator_floor=0;
// int elevator_direction=1;

int wait_at = -1;
// int is_elevator_ready = 0;
int is_passenger_ready = 0;
pthread_cond_t elevator_signal = PTHREAD_COND_INITIALIZER;
pthread_cond_t passenger_signal = PTHREAD_COND_INITIALIZER;

struct Passenger_info* list = NULL;

//add_to_list and remove_from_list taken from Chris Kanich's class material from Spring 2021 by TA recommendation

// MUST hold lock and check for sufficient capacity before calling this,
// and MUST NOT be in the riding queue. adds to end of queue.
void add_to_list(struct Passenger_info** head, struct Passenger_info* passenger) {
    struct Passenger_info* p = *head;
    if (p == NULL)
        *head = passenger;
    else {
        while (p->next != NULL) {
            p = p->next;
        }
        p->next = passenger;
    }
}

// Must hold lock, does not update occupancy.
void remove_from_list(struct Passenger_info** head, struct Passenger_info* passenger) {
    struct Passenger_info* p = *head;
    struct Passenger_info* prev = NULL;
    while (p != passenger) {
        prev = p;
        p = (p)->next;
        // don't crash because you got asked to remove someone that isn't on this
        // list
        assert(p != NULL);
    }
    // removing the head - must fix head pointer too
    if (prev == NULL)
        *head = p->next;
    else
        prev->next = p->next;
    passenger->next = NULL;
}

void scheduler_init() {
    // pthread_mutex_init(&passenger_lock,0);
    // pthread_mutex_init(&elevator_lock,0);
    // pthread_mutex_lock(&elevator_lock);
    for (int i = 0; i < 4; i++) {
        elevators[i].elevator_floor = 0;
        elevators[i].elevator_direction = 1;
        elevators[i].passenger_count = 0;
        elevators[i].wait_at = -1;
        elevators[i].is_elevator_ready = 0;
        elevators[i].wait_count = 0;
        elevators[i].wait_num = 0;
        elevators[i].start = 0;
        elevators[i].next_wait = -1;
        pthread_mutex_init(&elevators[i].elevator_lock,0);
        pthread_mutex_init(&elevators[i].passenger_lock,0);
        pthread_mutex_lock(&elevators[i].elevator_lock);
        elevators[i].riding = NULL;
    }
}

//working function for single elevator
/*
void passenger_request(int passenger, int from_floor, int to_floor, void (*enter)(int, int), void(*exit)(int, int)) {
    // pthread_mutex_lock(&passenger_lock); //single elevator

    pthread_mutex_lock(&passenger_lock);

    // *************
    // ENTER THE LIFT
    // *************
    pthread_mutex_lock(&elevator_lock);

    // TODO 1b: Submit request to elevator by setting 'wait_at'.
    // 'wait_at' is used to store the next floor at which the elevator should stop
    // at. Replace 0 below with the correct value for this request.
    wait_at = from_floor;

    // TODO 1c:
    // After setting the 'wait_at' variable, we wait for the elevator to arrive at the
    // floor and inform us once it's ready.
    //
    // The passenger thread should wait for the 'is_elevator_ready' variable to be set
    // and the elevator sets the variable and signals this thread
    //
    // Fill in the while loop below to wait for the condition to be met.
    while(!is_elevator_ready) {
        pthread_cond_wait(&elevator_signal, &elevator_lock);
    }
    is_elevator_ready = 0;

    // enter the lift
    enter(passenger, 0);

    // TODO 1e:
    // We've now entered the elevator. It's now the passenger's turn to notify the elevator
    // that they have entered. As in the previous TODOs, use pthread_cond_signal to notify
    // the elevator thread that the passenger is ready.
    is_passenger_ready = 1;
    pthread_cond_signal(&passenger_signal);
    pthread_mutex_unlock(&elevator_lock);


    // *************
    // EXIT THE LIFT
    // *************
    // TODO 2: Use what you've learned in the previous TODOs to replace the below busy poll
    // with the condition algorithm.
    
    pthread_mutex_lock(&elevator_lock);

    
    // if(elevator_floor == to_floor) { 
    //     exit(passenger,0);
    //     pthread_mutex_unlock(&elevator_lock);
        
    // }
    wait_at = to_floor;
    while(!is_elevator_ready) {
        pthread_cond_wait(&elevator_signal, &elevator_lock);
    }
    is_elevator_ready = 0;
    exit(passenger, 0);
    is_passenger_ready = 1;
    pthread_cond_signal(&passenger_signal);

    pthread_mutex_unlock(&elevator_lock);
    
    pthread_mutex_unlock(&passenger_lock);
}*/


// version for multiple elevators

void passenger_request(int passenger, int from_floor, int to_floor, void (*enter)(int, int), void(*exit)(int, int)) {
    int elevator = passenger % 4; 
    // int elevator = 0; 

    // pthread_mutex_lock(&elevators[elevator].elevator_lock);

    // pthread_mutex_lock(&elevators[elevator].elevator_lock);
    // pthread_mutex_lock(&elevators[elevator].passenger_lock);
    elevators[elevator].waitlist[elevators[elevator].wait_count] = from_floor;
    elevators[elevator].wait_at = from_floor;
    log(9, "Adding request from passenger %i to elevator %i, from_floor is: %i\n", passenger, elevator, from_floor);
    elevators[elevator].wait_count++;
    passengers[passenger].from_floor = from_floor;
    passengers[passenger].to_floor = to_floor;
    passengers[passenger].id = passenger;
    passengers[passenger].elenumber = elevator;
    passengers[passenger].status = 0;
    elevators[elevator].start++;
    pthread_mutex_lock(&elevators[elevator].elevator_lock);
    // *************
    // ENTER THE LIFT
    // *************

    // TODO 1b: Submit request to elevator by setting 'wait_at'.
    // 'wait_at' is used to store the next floor at which the elevator should stop
    // at. Replace 0 below with the correct value for this request.
    // elevators[elevator].wait_at = from_floor;

    // TODO 1c:
    // After setting the 'wait_at' variable, we wait for the elevator to arrive at the
    // floor and inform us once it's ready.
    //
    // The passenger thread should wait for the 'is_elevator_ready' variable to be set
    // and the elevator sets the variable and signals this thread
    //
    // Fill in the while loop below to wait for the condition to be met.
    while(!elevators[elevator].is_elevator_ready || elevators[elevator].elevator_floor != from_floor || elevators[elevator].passenger_count >= 3) {
        if (elevators[elevator].passenger_count >= 3 ) {
            // log(9, "Hitting error: elevator %i full, passenger %i is waiting\n", elevator, passenger);
            // elevators[elevator].next_wait = from_floor;
            // elevators[elevator].is_elevator_ready = 0;
        }
        // if (elevators[elevator].is_elevator_ready) {
        //     log(9, "Hitting error: elevator %i opened, passenger %i is waiting\n", elevator, passenger);
        //     log(9, "From_floor: %i\nElevator floor: %i", from_floor, elevators[elevator].elevator_floor);
        // }

        pthread_cond_wait(&elevator_signal, &elevators[elevator].elevator_lock);
    }
    elevators[elevator].wait_at = from_floor;
    elevators[elevator].is_elevator_ready = 0;
    // enter the lift
    enter(passenger, elevator);
    passengers[passenger].status = 1;
    elevators[elevator].passenger_count++;
    add_to_list(&elevators[elevator].riding, &passengers[passenger]);
    // log(9, "Adding passenger %i to ridelist, to_floor of head is: %i\n", passenger, elevators[elevator].riding->to_floor);
    // log(9, "Current ride list for elevator %i is:\n ", elevator);
    // struct Passenger_info* p = elevators[elevator].riding;
    // while (p != NULL) {
        // log(9, "Passenger %i: %i -> %i\n", p->id, p->from_floor, p->to_floor);
        // p = p->next;
    // }


    // TODO 1e:
    // We've now entered the elevator. It's now the passenger's turn to notify the elevator
    // that they have entered. As in the previous TODOs, use pthread_cond_signal to notify
    // the elevator thread that the passenger is ready.
    elevators[elevator].is_passenger_ready = 1;
    pthread_cond_broadcast(&passenger_signal);
    // pthread_mutex_unlock(&elevators[elevator].passenger_lock);
    pthread_mutex_unlock(&elevators[elevator].elevator_lock);

    // *************
    // EXIT THE LIFT
    // *************
    // TODO 2: Use what you've learned in the previous TODOs to replace the below busy poll
    // with the condition algorithm.
    
    pthread_mutex_lock(&elevators[elevator].elevator_lock);

    // pthread_mutex_lock(&elevators[elevator].passenger_lock); // other passengers should not act before passenger leaves
    
    // if(elevator_floor == to_floor) { 
    //     exit(passenger,0);
    //     pthread_mutex_unlock(&elevator_lock);
        
    // }
    // elevators[elevator].wait_at = to_floor;
    while(!elevators[elevator].is_elevator_ready || elevators[elevator].elevator_floor != to_floor) {
        // if (to_floor == elevators[elevator].elevator_floor) {
        //     // log(9, "Hitting error: elevator %i opened, passenger %i is attempting to exit\n", elevator, passenger);
        // }
        pthread_cond_wait(&elevator_signal, &elevators[elevator].elevator_lock);
    }
    elevators[elevator].is_elevator_ready = 0;
    exit(passenger, elevator);
    
    elevators[elevator].passenger_count--;
    remove_from_list(&elevators[elevator].riding, &passengers[passenger]);

    elevators[elevator].is_passenger_ready = 1;
    // pthread_cond_broadcast(&passenger_signal);

    pthread_mutex_unlock(&elevators[elevator].elevator_lock);
    pthread_cond_broadcast(&passenger_signal);
    
    // pthread_mutex_unlock(&elevators[elevator].passenger_lock);
}


void elevator_ready(int elevator, int at_floor, void(*move_direction)(int, int), void(*door_open)(int), void(*door_close)(int)) {
    // pthread_mutex_lock(&elevators[elevator].elevator_lock);
    while (elevators[elevator].start<3){
        usleep(10);
    }

    if (1) {
    if(at_floor == FLOORS-1)
        elevators[elevator].elevator_direction = -1;
    if(at_floor == 0)  
        elevators[elevator].elevator_direction = 1;

    // if (elevators[elevator].wait_at == -1 || elevators[elevator].passenger_count >= 3) {
        // if (wait_at != -1) {
        //     elevators[elevator].wait_at = wait_at;
        // }
        if (elevators[elevator].riding != NULL) {
            // if (elevators[elevator].wait_at == -1) {
            //     log(9, "Picking new wait_at. Reason: elevator %i has no wait_at\n", elevator);
            // } else {
            //     log(9, "Picking new wait_at. Reason: elevator %i is full\n", elevator);
            // }
            // log(9, "Elevator %i choosing from ride list, number is %i.\n", elevator, elevators[elevator].riding->to_floor);
            struct Passenger_info* p = elevators[elevator].riding;
            int closest = elevators[elevator].riding->to_floor;
            while (p->next != NULL) {
                p = p->next;
                if (abs(p->to_floor-at_floor)<abs(closest-at_floor)) {
                    closest = p->to_floor;
                }
            }
            elevators[elevator].wait_at = closest;
        }
        else if (elevators[elevator].next_wait != -1) {
            // if (elevators[elevator].wait_at == -1) {
            //     log(9, "Picking new wait_at. Reason: elevator %i has no wait_at\n", elevator);
            // } else {
            //     log(9, "Picking new wait_at. Reason: elevator %i is full\n", elevator);
            // }
            // log(9, "Elevator %i choosing from next list, number is %i.\n", elevator, elevators[elevator].next_wait);
            elevators[elevator].wait_at = elevators[elevator].next_wait;
            elevators[elevator].next_wait = -1;
        } 
        else {
            int closest = -1;
            for (int i = 0; i < PASSENGERS; i++) {
                if (passengers[i].status == 0 && passengers[i].elenumber == elevator) {
                    elevators[elevator].wait_at = passengers[i].from_floor;
                    if (closest == -1) {
                        closest = passengers[i].from_floor;
                    }
                    if (abs(passengers[i].from_floor-at_floor)<abs(closest-at_floor)) {
                        closest = passengers[i].from_floor;
                    }
                }
            }
            elevators[elevator].wait_at = closest;
        // }
        log(9, "Current wait_at %i\n", elevators[elevator].wait_at);
        // else {
        //     log(9, "Elevator %i choosing from wait list, number is %i. Passenger order is %i\n", elevator, elevators[elevator].waitlist[elevators[elevator].wait_num], elevators[elevator].wait_num);
        //     elevators[elevator].wait_at = elevators[elevator].waitlist[elevators[elevator].wait_num];
        //     elevators[elevator].wait_num++;
        // }
    }   
    // if (elevators[elevator].riding != NULL) {
    //     log(9, "Elevator %i choosing from ride list, number is %i.\n", elevator, elevators[elevator].riding->to_floor);
    //     elevators[elevator].wait_at = elevators[elevator].riding->to_floor;
    // } else if (elevators[elevator].next_wait != -1) {
    //     elevators[elevator].wait_at = elevators[elevator].next_wait;
    //     elevators[elevator].next_wait = -1;
    // }     
    // log(9, "Current wait list for elevator %i is:\n ", elevator)
    // for (int i = 0; i< PASSENGERS; i++) {
    //     log(9, "%i\n",elevators[elevator].waitlist[i] );
    // }

    if (elevators[elevator].wait_at > at_floor && elevators[elevator].elevator_direction == -1) {
        elevators[elevator].elevator_direction = 1;
    }
    if (elevators[elevator].wait_at != -1 && elevators[elevator].wait_at < at_floor && elevators[elevator].elevator_direction == 1) {
        elevators[elevator].elevator_direction = -1;
    }

    if (elevators[elevator].wait_at == at_floor) {
        door_open(elevator);
        // There is a passenger waiting at this floor.
        // We set wait_at back to its default value so it can be used later.
        elevators[elevator].wait_at = -1;

        // TODO 1d:
        // The passenger thread waiting for the elevator will be waiting for the elevator
        // to be ready so that it can step into the elevator. The door is open now so the elevator
        // is ready! Time to signal the other thread.
        //
        // Signal the passenger thread using 'is_elevator_ready', 'elevator_signal' and pthread_cond_signal
        /* FILL IN CODE HERE */

        elevators[elevator].is_elevator_ready = 1;
        pthread_cond_broadcast(&elevator_signal);
        // usleep(1);
        // TODO 1f:
        // It's now the passengers turn to notify the elevator thread once it's entered.
        // Wait for the 'is_passenger_ready' condition to be ready. Use the 'passenger_signal'
        // variable along with pthread_cond_wait to wait.
        while(!elevators[elevator].is_passenger_ready) {
            pthread_cond_wait(&passenger_signal, &elevators[elevator].elevator_lock);
        }
        elevators[elevator].is_passenger_ready = 0;
        door_close(elevator);
    } 
    else {
        // There's no passenger waiting at this floor
        // Let's release the lock which will allow other passenger threads to run
        // and check the state of the elevator
        pthread_mutex_unlock(&elevators[elevator].elevator_lock);
    }
        
    move_direction(elevator,elevators[elevator].elevator_direction);
    elevators[elevator].elevator_floor = at_floor+elevators[elevator].elevator_direction;
    }
}