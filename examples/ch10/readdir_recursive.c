#include "csapp.h"

static int check_file_type(char *entry)
{
    struct stat stat;
    Stat(entry, &stat);
    if (S_ISREG(stat.st_mode))
        return 0;
    else if (S_ISDIR(stat.st_mode))
        return 1;
    else
        return 2;
}

static void print_dir_tree(DIR *streamp, int depth, char *root)
{
    struct dirent *dir_entry;

    errno = 0;
    while ((dir_entry = readdir(streamp)) != NULL)
    {
        char *file = dir_entry->d_name;
        if (strcmp(file, ".") == 0 || strcmp(file, "..") == 0 ||
            strcmp(file, ".git") == 0)
            continue;
        for (int i = 0; i < depth; i++)
            printf("  ");
        printf("%s\n", file);
        char filepath[1024];
        strncpy(filepath, root, 1024);
        strcat(filepath, "/");
        strcat(filepath, file);
        if (check_file_type(filepath) == 1)
        {
            DIR *next = Opendir(filepath);
            print_dir_tree(next, depth + 1, filepath);
            Closedir(next);
        }
    }

    if (errno != 0)
        unix_error("readdir error");
}

int main(int argc, char *argv[])
{
    DIR *streamp;
    streamp = Opendir(argv[1]);
    print_dir_tree(streamp, 0, argv[1]);
    Closedir(streamp);
    exit(0);
}
