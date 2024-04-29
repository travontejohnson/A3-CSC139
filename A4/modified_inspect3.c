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

static struct option long_options[] = {
    {"all", no_argument, 0, 'a'},
    {"format", no_argument, 0, 'f'},
    {"help", no_argument, 0, '?'},
    {"human", no_argument, 0, 'h'},
    {"inode", no_argument, 0, 'i'},
    {"log", no_argument, 0, 'l'},
    {"format", required_argument, 0, 'f'},
    {0, 0, 0, 0}
};

void listFiles(const char *dirPath, int showInode, int recursive, int readable) ;
void printJSONOutput(const char *filePath, struct stat *fileInfo, int humanReadable);

void printUsage(const char *programName) {
    printf("Usage: %s [OPTION] [file_path|directory_path]\n", programName);
    printf("Displays information about the specified file or directory.\n\n");
    printf("Options:\n");
    printf("  -?, --help          Display this help and exit.\n");
    printf("  -i, --inode         Display detailed inode information for the specified file.\n");
    printf("  -a, --all           Display inode information for all files within the specified directory.\n");
    printf("  -r, --recursive     Recursively list files and directories.\n");
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
void printFileInfo(const char *filePath, int showInode, int humanReadable, int jsonOutput);

void print_json(struct stat fileStat, char *path) {
    printf("{\n\"filePath\": \"%s\",\n\"inode\": {\n", path);
    printf("    \"number\": %lu,\n", fileStat.st_ino);
    printf("    \"type\": \"regular file\",\n"); // Placeholder, actual implementation needed
    printf("    \"permissions\": \"%o\",\n", fileStat.st_mode); // Placeholder, actual conversion needed
    printf("    \"linkCount\": %lu,\n", fileStat.st_nlink);
    printf("    \"uid\": %u,\n", fileStat.st_uid);
    printf("    \"gid\": %u,\n", fileStat.st_gid);
    printf("    \"size\": \"%ld\",\n", fileStat.st_size); // Placeholder, actual human-readable conversion needed
    printf("    \"accessTime\": \"%s\",\n", ctime(&fileStat.st_atime)); // Convert to proper format
    printf("    \"modificationTime\": \"%s\",\n", ctime(&fileStat.st_mtime));
    printf("    \"statusChangeTime\": \"%s\"\n", ctime(&fileStat.st_ctime));
    printf("  }\n}\n");
}


int main(int argc, char *argv[]) {
    int opt, option_index = 0;
    int showAll = 0, showInode = 0, log = 0;
    int recursive = 0, human = 0, format = 0;
    int jsonOutput = 0;

    while ((opt = getopt_long(argc, argv, "ih?f", long_options, &option_index)) != -1) {
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
                jsonOutput = 1;
                break;
            case 'l':
                log = 0;
                break;
            default:
                printUsage(argv[0]);
                return 1;
        }
    }

    if (showAll) {
        char *dirPath = (optind < argc) ? argv[optind] : ".";
        listFiles(dirPath, showInode, recursive, human), jsonOutput;
    } else {
        if (optind >= argc) {
            printUsage(argv[0]);
            return 1;
        }
        printFileInfo(argv[optind], showInode, human, jsonOutput);
    }

    return 0;
}

void listFiles(const char *dirPath, int showInode, int recursive, int readable, int jsonOutput) {
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

        printFileInfo(filePath, showInode, readable, jsonOutput);

        if (recursive && entry->d_type == DT_DIR) {
            printf("\n");
            listFiles(filePath, showInode, recursive, readable, jsonOutput);
        }
    }

    closedir(dir);
}

void printJSONOutput(const char *filePath, struct stat *fileInfo, int humanReadable) {
    char accessTime[20], modificationTime[20], statusChangeTime[20];
    strftime(accessTime, sizeof(accessTime), "%Y-%m-%d %H:%M:%S", localtime(&fileInfo->st_atime));
    strftime(modificationTime, sizeof(modificationTime), "%Y-%m-%d %H:%M:%S", localtime(&fileInfo->st_mtime));
    strftime(statusChangeTime, sizeof(statusChangeTime), "%Y-%m-%d %H:%M:%S", localtime(&fileInfo->st_ctime));

    struct passwd *pw = getpwuid(fileInfo->st_uid);
    struct group *gr = getgrgid(fileInfo->st_gid);

    printf("{\n");
    printf("  \"filePath\": \"%s\",\n", filePath);
    printf("  \"inode\": {\n");
    printf("    \"number\": %ld,\n", (long)fileInfo->st_ino);
    printf("    \"type\": \"%s\",\n", S_ISDIR(fileInfo->st_mode) ? "directory" : (S_ISLNK(fileInfo->st_mode) ? "symbolic link" : "regular file"));
    printf("    \"permissions\": \"%s\",\n", getPermissions(fileInfo->st_mode));
    printf("    \"linkCount\": %ld,\n", (long)fileInfo->st_nlink);
    printf("    \"uid\": %d,\n", fileInfo->st_uid);
    printf("    \"gid\": %d,\n", fileInfo->st_gid);
    printf("    \"size\": \"%s\",\n", humanReadable ? getHumanReadableSize(fileInfo->st_size) : format("%ld", (long)fileInfo->st_size));
    printf("    \"accessTime\": \"%s\",\n", humanReadable ? accessTime : format("%ld", (long)fileInfo->st_atime));
    printf("    \"modificationTime\": \"%s\",\n", humanReadable ? modificationTime : format("%ld", (long)fileInfo->st_mtime));
    printf("    \"statusChangeTime\": \"%s\"\n", humanReadable ? statusChangeTime : format("%ld", (long)fileInfo->st_ctime));
    printf("  }\n");
    printf("}");
}

void printFileInfo(const char *filePath, int showInode, int humanReadable, int jsonOutput) {
    struct stat fileInfo;
    if (stat(filePath, &fileInfo) != 0) {
        perror("Failed to get file stats");
        return;
    }

    if (jsonOutput) {
        printJSONOutput(filePath, &fileInfo, humanReadable);
    } else {
        printf("File: %s\n", filePath);

        if (showInode) {
            printf("Inode: %ld\n", (long)fileInfo.st_ino);
        }

        printf("Size: ");
        if (humanReadable) {
            printHumanReadableSize(fileInfo.st_size);
        } else {
            printf("%ld bytes", (long)fileInfo.st_size);
        }
        printf("\n");

        printf("Modified: ");
        if (humanReadable) {
            printHumanReadableDate(fileInfo.st_mtime);
        } else {
            char buffer[80];
            struct tm *timeinfo = localtime(&fileInfo.st_mtime);
            strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
            printf("%s", buffer);
        }
        printf("\n");
    }
}
