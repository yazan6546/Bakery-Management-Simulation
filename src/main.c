#include "config.h"

int main(int argc, char *argv[]) {

    Config config;

    if (load_config("../config.txt", &config) == -1) {
        fprintf(stderr, "Failed to load configuration.\n");
        return 1;
        printf("Hello, World!\n");
    }

    print_config(&config);



}
