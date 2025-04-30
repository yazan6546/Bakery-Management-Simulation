#include <signal.h>
#include "game.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/msg.h>
#include <string.h>
#include "assets.h"

// Global pointer to shared game state
Game *shared_game;
pid_t processes[10]; // Increased to accommodate more processes
int shm_fd; // Store fd globally for cleanup
int message_queues[2]; // Store queue IDs for cleanup [0]=request, [1]=response

void handle_alarm(int signum);
void cleanup_resources();
pid_t start_supply_chain(int shared_mem_fd, int request_queue, int response_queue);
pid_t start_chef(int shared_mem_fd, int request_queue, int response_queue, int chef_id);

void handle_alarm(int signum) {
    shared_game->elapsed_time++;
    alarm(1);
}

// Function for cleaning up resources, registered with atexit()
void cleanup_resources() {
    printf("Cleaning up resources...\n");
    fflush(stdout);

    // Terminate all child processes
    for (int i = 0; processes[i] != 0 && i < 10; i++) {
        kill(processes[i], SIGINT);
    }

    // Clean up message queues
    for (int i = 0; i < 2; i++) {
        if (message_queues[i] != -1) {
            msgctl(message_queues[i], IPC_RMID, NULL);
        }
    }

    game_destroy(shm_fd, shared_game);

    printf("Cleanup complete\n");
    fflush(stdout);
}

void handle_kill(int signum) {
    exit(0);
}

// Start a supply chain process
pid_t start_supply_chain(int shared_mem_fd, int request_queue, int response_queue) {
    pid_t pid = fork();
    
    if (pid < 0) {
        perror("Fork failed for supply chain");
        return -1;
    }
    
    if (pid == 0) {
        // Child process
        char fd_str[16], req_q_str[16], resp_q_str[16];
        sprintf(fd_str, "%d", shared_mem_fd);
        sprintf(req_q_str, "%d", request_queue);
        sprintf(resp_q_str, "%d", response_queue);
        
        execl("./supply_chain", "supply_chain", fd_str, req_q_str, resp_q_str, NULL);
        perror("execl failed for supply chain");
        exit(1);
    }
    
    return pid;
}

// Start a chef process
pid_t start_chef(int shared_mem_fd, int request_queue, int response_queue, int chef_id) {
    pid_t pid = fork();
    
    if (pid < 0) {
        perror("Fork failed for chef");
        return -1;
    }
    
    if (pid == 0) {
        // Child process
        char fd_str[16], req_q_str[16], resp_q_str[16], id_str[16];
        sprintf(fd_str, "%d", shared_mem_fd);
        sprintf(req_q_str, "%d", request_queue);
        sprintf(resp_q_str, "%d", response_queue);
        sprintf(id_str, "%d", chef_id);
        
        execl("./chefs", "chefs", fd_str, req_q_str, resp_q_str, id_str, NULL);
        perror("execl failed for chef");
        exit(1);
    }
    
    return pid;
}

int main(int argc, char *argv[]) {
    printf("********** Bakery Simulation **********\n\n");
    fflush(stdout);

    // Initialize message queues array
    message_queues[0] = -1;
    message_queues[1] = -1;
    
    // Initialize processes array
    memset(processes, 0, sizeof(processes));

    // Register cleanup function with atexit
    atexit(cleanup_resources);
    game_create(&shm_fd, &shared_game);

    signal(SIGALRM, handle_alarm);
    signal(SIGINT, handle_kill);
    signal(SIGKILL, handle_kill);

    if (load_config(CONFIG_PATH, &shared_game->config) == -1) {
        printf("Config file failed");
        return 1;
    }

    // Create message queues for chef-supply chain communication
    int request_queue = msgget(IPC_PRIVATE, 0666 | IPC_CREAT);
    int response_queue = msgget(IPC_PRIVATE, 0666 | IPC_CREAT);
    
    if (request_queue == -1 || response_queue == -1) {
        perror("Failed to create message queues");
        return 1;
    }
    
    // Store queue IDs for cleanup
    message_queues[0] = request_queue;
    message_queues[1] = response_queue;
    
    printf("Created message queues: request=%d, response=%d\n", request_queue, response_queue);
    
    // Start the basic game processes
    game_init(shared_game, processes, shm_fd);
    
    // Start supply chain processes (one per supply chain defined in config)
    int proc_index = 0;
    while (processes[proc_index] != 0) proc_index++;
    
    for (int i = 0; i < shared_game->config.NUM_SUPPLY_CHAIN; i++) {
        processes[proc_index] = start_supply_chain(shm_fd, request_queue, response_queue);
        printf("Started supply chain process %d\n", processes[proc_index]);
        proc_index++;
    }
    
    // Start chef processes
    for (int i = 0; i < shared_game->config.NUM_CHEFS; i++) {
        processes[proc_index] = start_chef(shm_fd, request_queue, response_queue, i);
        printf("Started chef process %d (ID: %d)\n", processes[proc_index], i);
        proc_index++;
    }

    alarm(1);  // Start the timer

    while (check_game_conditions(shared_game)) {
        // Main loop
        sleep(1);
    }

    return 0;
}