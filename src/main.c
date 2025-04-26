#include "common.h"
#include <signal.h>
#include "game.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include "config.h"

// Global pointer to shared game state
Game *shared_game;
int shm_fd; // Store fd globally for cleanup

void handle_alarm(int signum);
void handle_sigint(int signum); // Renamed from handle_sigkill to match actual signal
void cleanup_resources(void);   // New function for atexit
pid_t start_process(const char *binary, Config *config);

void handle_alarm(int signum) {

    // Check if referee requested a reset
    if (shared_game->reset_round_time_flag) {
        shared_game->round_time = 0;
    } else {
        shared_game->round_time++;
        shared_game->elapsed_time++;
    }

    alarm(1);
}

void handle_sigint(int signum) {
    exit(0); // Let atexit handle cleanup
}

// Function for cleaning up resources, registered with atexit()
void cleanup_resources(void) {
    printf("Cleaning up resources...\n");
    fflush(stdout);

    if (shared_game != NULL && shared_game != MAP_FAILED) {
        if (munmap(shared_game, sizeof(Game)) == -1) {
            perror("munmap failed");
        }
    }

    shm_unlink("/game_shared_mem");

    if (shm_fd > 0) {
        close(shm_fd);
    }

    kill(pid_graphics, SIGKILL);
    kill(pid_referee, SIGKILL);

    printf("Cleanup complete\n");
    fflush(stdout);
}

int main(int argc, char *argv[]) {

    printf("********** The Rope Pulling Game **********\n\n");
    fflush(stdout);

    Config config;

    if (load_config("../config.txt", &config) == -1) {
        return 1;
    }

    // Register cleanup function with atexit
    atexit(cleanup_resources);

    // shared_game = malloc(sizeof(Game)); // Remove this line - using mmap instead

    // Create shared game state using mmap
    shm_fd = shm_open("/game_shared_mem", O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open failed");
        exit(EXIT_FAILURE);
    }

    if (ftruncate(shm_fd, sizeof(Game)) == -1) {
        perror("ftruncate failed");
        exit(EXIT_FAILURE);
    }

    shared_game = mmap(NULL, sizeof(Game), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_game == MAP_FAILED) {
        perror("mmap failed");
        exit(EXIT_FAILURE);
    }

    fcntl(shm_fd, F_SETFD, fcntl(shm_fd, F_GETFD) & ~FD_CLOEXEC);

    // Pass fd as string
    char fd_str[16];
    sprintf(fd_str, "%d", shm_fd);

    // Initialize game state
    init_game(shared_game);
    //
    pid_t pid_graphics = start_process("./graphics", &config);
    pid_t pid_chefs = start_process("./chefs", &config);
    pid_t pid_bakers = start_process("./bakers", &config);
    pid_t pid_sellers = start_process("./sellers", &config);
    pid_t pid_supply_chain = start_process("./supply_chain", &config);

    char gui_pid_str[16];
    sprintf(gui_pid_str, "%d", pid_graphics);

    // Setup signal handler for time management
    signal(SIGALRM, handle_alarm);
    signal(SIGINT, handle_sigint); // Renamed to match actual signal
    alarm(1);  // Start the timer

    int status_referee, status_graphics, status_chefs, status_bakers, status_sellers;
    int status_supply_chain;
    waitpid(pid_graphics, &status_referee, 0);
    waitpid(pid_chefs, &status_graphics, 0);
    waitpid(pid_bakers, &status_chefs, 0);
    waitpid(pid_sellers, &status_bakers, 0);
    waitpid(pid_supply_chain, &status_sellers, 0);
    waitpid(pid_supply_chain, &status_supply_chain, 0);

    if (WIFEXITED(status_referee)) {
        printf("Referee Child process exited with code: %d\n", WEXITSTATUS(status_referee));
    } else if (WIFSIGNALED(status_referee)) {
        printf("Referee Child process terminated by signal: %d\n", WTERMSIG(status_referee));
    }

    if (WIFEXITED(status_graphics)) {
        printf("Graphics Child process exited with code: %d\n", WEXITSTATUS(status_graphics));
    } else if (WIFSIGNALED(status_graphics)) {
        printf("Graphics Child process terminated by signal: %d\n", WTERMSIG(status_graphics));

        return 0; // cleanup_resources will be called automatically via atexit
    }

    if (WIFEXITED(status_chefs)) {
        printf("Chefs Child process exited with code: %d\n", WEXITSTATUS(status_chefs));
    } else if (WIFSIGNALED(status_chefs)) {
        printf("Chefs Child process terminated by signal: %d\n", WTERMSIG(status_chefs));
    }

    if (WIFEXITED(status_bakers)) {
        printf("Bakers Child process exited with code: %d\n", WEXITSTATUS(status_bakers));
    } else if (WIFSIGNALED(status_bakers)) {
        printf("Bakers Child process terminated by signal: %d\n", WTERMSIG(status_bakers));
    }

    if (WIFEXITED(status_sellers)) {
        printf("Sellers Child process exited with code: %d\n", WEXITSTATUS(status_sellers));
    } else if (WIFSIGNALED(status_sellers)) {
        printf("Sellers Child process terminated by signal: %d\n", WTERMSIG(status_sellers));
    }

    if (WIFEXITED(status_supply_chain)) {
        printf("Supply Chain Child process exited with code: %d\n", WEXITSTATUS(status_supply_chain));
    } else if (WIFSIGNALED(status_supply_chain)) {
        printf("Supply Chain Child process terminated by signal: %d\n", WTERMSIG(status_supply_chain));
    }

}


pid_t start_process(const char *binary, Config *config) {
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        // Now pass two arguments: shared memory fd and GUI pid.
        char buffer[50];
        serialize_config(config, buffer);
        if (execl(binary, "./referee", buffer, NULL)) {
            perror("execl referee");
            exit(EXIT_FAILURE);
        }
    }
    return pid;
}