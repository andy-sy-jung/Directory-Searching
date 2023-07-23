#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <limits.h>



void display_usage(){
    fprintf(stdout, "Usage: ./pfind -d <directory> -p <permissions string> [-h]\n");
}

int traverse_directory(char *path, char *permission){
    DIR *directory;
    if((directory = opendir(path)) == NULL){
        fprintf(stderr, "Error: Cannot open directory '%s'. %s.\n", path, strerror(errno));
        return EXIT_FAILURE;
    }
     
    char full_filename[PATH_MAX + 1];
     
    if(strcmp(path, "/")) {
        size_t copy_len = strnlen(path, PATH_MAX);
        memcpy(full_filename, path, copy_len);
        full_filename[copy_len] = '\0';
    } 
    size_t pathlen = strlen(full_filename) + 1;
    full_filename[pathlen - 1] = '/';
    full_filename[pathlen] = '\0';

    struct dirent *entry;
    struct stat sb;

    while((entry = readdir(directory)) != NULL) {
        if(strcmp(entry->d_name, ".") == 0 ||
           strcmp(entry->d_name, "..") == 0){
           continue;
           }

        strncpy(full_filename + pathlen, entry->d_name, PATH_MAX - pathlen);
        
        if(lstat(full_filename, &sb) < 0){
            fprintf(stderr, "Error: Cannot stat '%s'. %s.\n", full_filename, strerror(errno));
            continue;
        }

        char perm_str[10] = "---------";

        if(S_ISDIR(sb.st_mode)){
            traverse_directory(full_filename, permission);
        }
        else{
            perm_str[0] = (sb.st_mode & S_IRUSR) ? 'r' : '-';
            perm_str[1] = (sb.st_mode & S_IWUSR) ? 'w' : '-';
            perm_str[2] = (sb.st_mode & S_IXUSR) ? 'x' : '-';
            perm_str[3] = (sb.st_mode & S_IRGRP) ? 'r' : '-';
            perm_str[4] = (sb.st_mode & S_IWGRP) ? 'w' : '-';
            perm_str[5] = (sb.st_mode & S_IXGRP) ? 'x' : '-';
            perm_str[6] = (sb.st_mode & S_IROTH) ? 'r' : '-';
            perm_str[7] = (sb.st_mode & S_IWOTH) ? 'w' : '-';
            perm_str[8] = (sb.st_mode & S_IXOTH) ? 'x' : '-';
            perm_str[9] = '\0';
        }

        if(strcmp(permission, perm_str) == 0){
            fprintf(stdout, "%s\n", full_filename);
        }

    }

    closedir(directory);
    return EXIT_SUCCESS;

}

int main(int argc, char **argv){
    char *dir = NULL;
    char *permission = NULL;
    opterr = 0;
    int d_flag = 0;
    int p_flag = 0;
    int c;

    while ((c = getopt(argc, argv, "d:p:h")) != -1){
        switch(c){
            case 'd':
                d_flag = 1;
                dir = optarg;
                break;
            case 'p':
                p_flag = 1;
                permission = optarg;
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

    char path[PATH_MAX];
    if(realpath(dir, path) == NULL){
        fprintf(stderr, "Error: Cannot get full path of '%s'. %s.\n", dir, strerror(errno));
        return EXIT_FAILURE;
    }

    struct stat sb;
    if(lstat(path, &sb) < 0){
        fprintf(stderr, "Error: Cannot stat '%s'. %s.\n", path, strerror(errno));
        return EXIT_FAILURE;
    }

    if(strlen(permission) != 9){
        fprintf(stderr, "Error: Permissions string '%s' is invalid.\n", permission);
        return EXIT_FAILURE;
    }

    for(int i = 0; i < 9; i++){
        if(i % 3 == 0){
            if(permission[i] != 'r' && permission[i] != '-'){
                fprintf(stderr, "Error: Permissions string '%s' is invalid.\n", permission);
                return EXIT_FAILURE;
            }
        }
        else if(i % 3 == 1 && (permission[i] != 'w' && permission[i] != '-')){
            fprintf(stderr, "Error: Permissions string '%s' is invalid.\n", permission);
            return EXIT_FAILURE;
        }
        else if(i % 3 == 2 && (permission[i] != 'x' && permission[i] != '-')){
            fprintf(stderr, "Error: Permissions string '%s' is invalid.\n", permission);
            return EXIT_FAILURE;
        }
    }

    if(S_ISDIR(sb.st_mode)){
        traverse_directory(path, permission);
    }
    else{
        fprintf(stderr, "Error: '%s' is not a directory.\n", path);
    }


    return EXIT_SUCCESS;
}



