#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <dirent.h>
#include <getopt.h>

#define PATH_MAX 4096

void printUsage(const char *programName);
void printFileInfo(const char *filePath, int showInode, int readable);
void listFiles(const char *dirPath, int showInode, int recursive, int readable);
void printHumanReadableSize(off_t size);
void printHumanReadableDate(time_t rawtime);

static struct option long_options[] = {
    {"all", no_argument, 0, 'a'},
    {"format", no_argument, 0, 'f'},
    {"help", no_argument, 0, '?'},
    {"human", no_argument, 0, 'h'},
    {"inode", no_argument, 0, 'i'},
    {"log", no_argument, 0, 'l'},
    {0, 0, 0, 0}
};

int main(int argc, char *argv[]) {
    int showInode = 0;
    int showAll = 0;
    int recursive = 0;
    int human = 0;
    int opt, option_index = 0;

    while ((opt = getopt_long(argc, argv, "i?", long_options, &option_index)) != -1) {
         switch (opt) {
            case '?':
                printUsage(argv[0]);
                return 0;
            case 'i':
                showInode = 1;
                break;
            case 'a':
                showAll = 1;
                break;
            case 'r':
                recursive = 1;
                break;
            case 'h':
                human = 1;
                break;
            case 'f':
                
                break;
            case 'l':
                
                break;
            default:
                printUsage(argv[0]);
                return 1;
        }
    }

    if (showAll) {
        char *dirPath = (optind < argc) ? argv[optind] : ".";
        listFiles(dirPath, showInode, recursive, human);
    } else {
        if (optind >= argc) {
            printUsage(argv[0]);
            return 1;
        }
        printFileInfo(argv[optind], showInode, human);
    }

    return 0;
}

void printUsage(const char *programName) {
    printf("Usage: %s [OPTION] [file_path|directory_path]\n", programName);
    printf("Displays information about the specified file or directory.\n\n");
    printf("Options:\n");
    printf("  -?, --help          Display this help and exit.\n");
    printf("  -i, --inode         Display detailed inode information for the specified file.\n");
    printf("  -a, --all           Display inode information for all files within the specified directory.\n");
    printf("  -r, --recursive     Recursively list files and directories.\n");
}

void printFileInfo(const char *filePath, int showInode, int readable) {
    struct stat fileInfo;

    if (stat(filePath, &fileInfo) != 0) {
        fprintf(stderr, "Error getting file info for %s: %s\n", filePath, strerror(errno));
        return;
    }

    printf("Information for %s:\n", filePath);

    if (showInode) {
        printf("File Inode: %llu\n", fileInfo.st_ino);
    }

    printf("File Type: ");
    if (S_ISREG(fileInfo.st_mode))
        printf("regular file\n");
    else if (S_ISDIR(fileInfo.st_mode))
        printf("directory\n");
    else if (S_ISCHR(fileInfo.st_mode))
        printf("character device\n");
    else if (S_ISBLK(fileInfo.st_mode))
        printf("block device\n");
    else if (S_ISFIFO(fileInfo.st_mode))
        printf("FIFO (named pipe)\n");
    else if (S_ISLNK(fileInfo.st_mode))
        printf("symbolic link\n");
    else if (S_ISSOCK(fileInfo.st_mode))
        printf("socket\n");
    else
        printf("unknown?\n");

    printf("Number of Hard Links: %hu\n", fileInfo.st_nlink);
    printf("File Size: %llu bytes\n", fileInfo.st_size);

    char accessTime[50];
    strftime(accessTime, sizeof(accessTime), "%b %d %Y %H:%M", localtime(&fileInfo.st_atime));
    printf("Last Access Time: %s\n", accessTime);
    
    char modificationTime[50];
    strftime(modificationTime, sizeof(modificationTime), "%b %d %Y %H:%M", localtime(&fileInfo.st_mtime));
    printf("Last Modification Time: %s\n", modificationTime);

    char statusChangeTime[50];
    strftime(statusChangeTime, sizeof(statusChangeTime), "%b %d %Y %H:%M", localtime(&fileInfo.st_ctime));
    printf("Last Status Change Time: %s\n\n", statusChangeTime);
}

void listFiles(const char *dirPath, int showInode, int recursive, int readable) {
    DIR *dir;
    struct dirent *entry;

    if ((dir = opendir(dirPath)) == NULL) {
        fprintf(stderr, "Error opening directory %s: %s\n", dirPath, strerror(errno));
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char filePath[PATH_MAX];
        snprintf(filePath, sizeof(filePath), "%s/%s", dirPath, entry->d_name);

        printFileInfo(filePath, showInode, readable);

        if (recursive && entry->d_type == DT_DIR) {
            printf("\n");
            listFiles(filePath, showInode, recursive, readable);
        }
    }

    closedir(dir);
}

void printHumanReadableSize(off_t size) {
    if (size < 1024) printf("%lld B", size);
    else if (size < 1024 * 1024) printf("%.1f K", size / 1024.0);
    else if (size < 1024 * 1024 * 1024) printf("%.1f M", size / (1024.0 * 1024));
    else printf("%.1f G", size / (1024.0 * 1024 * 1024));
}

void printHumanReadableDate(time_t rawtime) {
    struct tm *timeinfo = localtime(&rawtime);
    char buffer[80];
    strftime(buffer, sizeof(buffer), "%b %d %Y %H:%M", timeinfo);
    printf("%s", buffer);
}