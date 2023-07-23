#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <getopt.h>
#include <limits.h>
#include <string.h>

void display_usage(){
    fprintf(stdout, "Usage: ./spfind -d <directory> -p <permissions string> [-h]\n");
}

int main(int argc, char **argv){

    if(argc == 1){
        display_usage();
        return EXIT_SUCCESS;
    }

    opterr = 0;
    int d_flag = 0;
    int p_flag = 0;
    int c;

    while ((c = getopt(argc, argv, "d:p:h")) != -1){
        switch(c){
            case 'd':
                d_flag = 1;
                break;
            case 'p':
                p_flag = 1;
                break;
            case 'h':
                display_usage();
                return EXIT_SUCCESS;
            case '?':
                switch(optopt){
                    case 'd':
                        fprintf(stderr, "Error: Required argument -d <directory> not found.\n");
                        return EXIT_FAILURE;
                    case 'p':
                        fprintf(stderr, "Error: Required argument -p <permissions string> not found.\n");
                        return EXIT_FAILURE;
                    default:
                        fprintf(stderr, "Error: Unknown option '-%c' received.\n", optopt);
                        return EXIT_FAILURE;
                }
                fprintf(stderr, "Error: Unknown option '-%c' received.\n", optopt);
                return EXIT_FAILURE;
        }
    }

    if(d_flag == 0){
        fprintf(stderr, "Error: Required argument -d <directory> not found.\n");
        return EXIT_FAILURE;
    }
    if(p_flag == 0){
        fprintf(stderr, "Error: Required argument -p <permissions string> not found.\n");
        return EXIT_FAILURE;
    }


    // changing executable name so that
    // it could be used in execv of pfind
    argv[0] = "./pfind";

    int pfind_to_sort[2];
    int sort_to_parent[2];
    pid_t pid[2];

    if(pipe(pfind_to_sort) == -1 || pipe(sort_to_parent) == -1){
        fprintf(stderr, "Error: pipe() failed. %s.\n", strerror(errno));
        return EXIT_FAILURE;
    }

    // pfind child
    if((pid[0] = fork()) < 0){
        fprintf(stderr, "Error: fork() failed. %s.\n", strerror(errno));
        return EXIT_FAILURE;
    }
    else if(pid[0] == 0){
        // close read end of pfind
        close(pfind_to_sort[0]);
        // close unused pipe ends
        close(sort_to_parent[0]);
        close(sort_to_parent[1]);

        if(dup2(pfind_to_sort[1], STDOUT_FILENO) == -1){
            fprintf(stderr, "Error: dup2() failed. %s.\n", strerror(errno));
            return EXIT_FAILURE;
        }
        close(pfind_to_sort[1]);

        if(execv("./pfind", argv) == -1){
            return EXIT_FAILURE;
        } 
    }

    // sort child
    if((pid[1] = fork()) < 0){
        fprintf(stderr, "Error: fork() failed. %s.\n", strerror(errno));
        return EXIT_FAILURE;
    }
    else if(pid[1] == 0){
        // close read end of sort
        close(sort_to_parent[0]);
        // close unused pipe ends
        close(pfind_to_sort[1]);

        if(dup2(pfind_to_sort[0], STDIN_FILENO) == -1){
            fprintf(stderr, "Error: dup2() failed. %s.\n", strerror(errno));
            return EXIT_FAILURE;
        }
        close(pfind_to_sort[0]);

        if(dup2(sort_to_parent[1], STDOUT_FILENO) == -1){
            fprintf(stderr, "Error: dup2() failed. %s.\n", strerror(errno));
            return EXIT_FAILURE;
        }
        close(sort_to_parent[1]);

        if(execlp("sort", "sort", NULL) == -1){
            return EXIT_FAILURE;
        }
    }

    // parent process
    // close write end of sort
    close(sort_to_parent[1]);
    // close unused pipe ends
    close(pfind_to_sort[0]);
    close(pfind_to_sort[1]);

    int status;
    int status2;

    pid_t w = waitpid(pid[0], &status, 0);

    if(w == -1) {
        fprintf(stderr, "Error: waitpid() failed. %s.\n", strerror(errno));
        return EXIT_FAILURE;
    }

    if(!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        fprintf(stderr, "Error: pfind failed.\n");
        return EXIT_FAILURE;
    }

    pid_t w2 = waitpid(pid[1], &status2, 0);

    if(w2 == -1){
        fprintf(stderr, "Error: waitpid() failed. %s.\n", strerror(errno));
        return EXIT_FAILURE;
    }

    if(!WIFEXITED(status2) || WEXITSTATUS(status2) != 0) {
        fprintf(stderr, "Error: sort failed.\n");
        return EXIT_FAILURE;
    }

    char buf[PATH_MAX];
    int matches = 0;
    int bytes_read = 0;

    // since we do not know how many bytes are in a line
    // read 1 byte at a time and print when we encounter a \n
    while(1){
        char c;
        ssize_t result = read(sort_to_parent[0], &c, 1);
        if(result == -1){
            fprintf(stderr, "Error: read() failed. %s.\n", strerror(errno));
            return EXIT_FAILURE;
        }
        if(result == 0){
            break;
        }
        buf[bytes_read++] = c;
        if(c == '\n'){
            printf("%.*s", bytes_read, buf);
            bytes_read = 0;
            matches++;
        }
    }
    
    close(sort_to_parent[0]);

    printf("Total matches: %d\n", matches);

    return EXIT_SUCCESS;

}
