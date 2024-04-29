#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <dirent.h>
#include <getopt.h>
#include <pwd.h>
#include <grp.h>
#include <sys/types.h>

#define PATH_MAX 4096

static struct option long_options[] = {
    {"all", no_argument, 0, 'a'},
    {"format", no_argument, 0, 'f'},
    {"help", no_argument, 0, '?'},
    {"human", no_argument, 0, 'h'},
    {"inode", no_argument, 0, 'i'},
    {"log", required_argument, 0, 'l'},
    {0, 0, 0, 0}
};

char *formatTime(time_t time, int readable);
char* getHumanReadableSize(off_t size);
char* getPermissions(mode_t mode);
void listFiles(const char *dirPath, int showInode, int recursive, int readable, int jsonOutput, FILE *logfp);
void printFileInfo(const char *filePath, int showInode, int humanReadable, int jsonOutput);
void printHumanReadableDate(time_t rawtime);
void printHumanReadableSize(off_t size);
void printJSONOutput(const char *filePath, struct stat *fileInfo, int readable);
void printTextOutput(const char *filePath, struct stat *fileInfo, int readable);
void printUsage(const char *programName);

int main(int argc, char *argv[]) {
    int opt, option_index = 0;
    int showAll = 0, showInode = 0, log = 0;
    int recursive = 0, human = 0, format = 0;
    int jsonOutput = 0, textOutput = 0;
    char *logFile = NULL;
    FILE *logfp = NULL;

    while ((opt = getopt_long(argc, argv, "a?f::hil", long_options, &option_index)) != -1) {
        switch (opt) {
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
                if (optind < argc && argv[optind] != NULL) {
                    char *format = argv[optind];
                    if (strcmp(format, "json") == 0) {
                        jsonOutput = 1;
                        optind++;
                    } else if (strcmp(format, "text") == 0) {
                            printf("Using Standard Text Output\n\n");
                        jsonOutput = 0;
                        optind++;
                    } else {
                        printf("Using NonStandard Text Output\n\n");
                        fprintf(stderr, "Invalid format: %s. Using default text format.\n", format);
                        jsonOutput = 0;
                    }
                } else {
                    printf("Using NonStandard Text Output\n\n");
                    jsonOutput = 0;
                }
                break;
            case 'l':
                if (!log) {
                    logFile = optarg;
                    log = 1;
                }
                break;
            case '?':
                if (optopt == 'l') {
                    if (!log) {
                        logFile = optarg;
                        log = 1;
                    }
                } else if (optopt == 0) {
                    printUsage(argv[0]);
                    return 0;
                } else {
                    fprintf(stderr, "Unknown option: -%c\n", optopt);
                    printUsage(argv[0]);
                    return 1;
                }
            break;
            default:
                printUsage(argv[0]);
                return 1;
        }
    }
    if (logFile != NULL) {
        logfp = fopen(logFile, "w");
        if (logfp == NULL) {
            fprintf(stderr, "Error opening log file %s: %s\n", logFile, strerror(errno));
            return 1;
        }
    }

    if (showAll) {
        char *dirPath = (optind < argc) ? argv[optind] : ".";
         if (logfp != NULL) {
            fprintf(logfp, "Listing files in directory: %s\n", dirPath);
        }
        listFiles(dirPath, showInode, recursive, human, jsonOutput, logfp);
    } else {
        if (optind >= argc) {
            printUsage(argv[0]);
            return 1;
        }
        if (logfp != NULL) {
            fprintf(logfp, "Inspecting file: %s\n", argv[optind]);
        }
        printFileInfo(argv[optind], showInode, human, jsonOutput);
    }
    if (logfp != NULL) {
        fclose(logfp);
    }

    return 0;
}


void listFiles(const char *dirPath, int showInode, int recursive, int readable, int jsonOutput, FILE *logfp) {
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
        if (logfp != NULL) {
            fprintf(logfp, "Processing file: %s\n", filePath);
        }
        printFileInfo(filePath, showInode, readable, jsonOutput);

        if (recursive && entry->d_type == DT_DIR) {
            printf("\n");
            listFiles(filePath, showInode, recursive, readable, jsonOutput, logfp);
        }
    }

    closedir(dir);
}

void printJSONOutput(const char *filePath, struct stat *fileInfo, int readable) {
    printf("{\n");
    printf("  \"filePath\": \"%s\",\n", filePath);
    printf("  \"inode\": {\n");
    printf("    \"number\": %ld,\n", (long)fileInfo->st_ino);
    printf("    \"type\": \"%s\",\n", S_ISDIR(fileInfo->st_mode) ? "directory" : (S_ISLNK(fileInfo->st_mode) ? "symbolic link" : "regular file"));
    printf("    \"permissions\": \"%s\",\n", getPermissions(fileInfo->st_mode));  // Assuming getPermissions() is defined elsewhere
    printf("    \"linkCount\": %ld,\n", (long)fileInfo->st_nlink);
    printf("    \"uid\": %d,\n", fileInfo->st_uid);
    printf("    \"gid\": %d,\n", fileInfo->st_gid);
    printf("    \"size\": \"%s\",\n", getHumanReadableSize(fileInfo->st_size));
    printf("    \"accessTime\": \"%s\",\n", formatTime(fileInfo->st_atime, readable));
    printf("    \"modificationTime\": \"%s\",\n", formatTime(fileInfo->st_mtime, readable));
    printf("    \"statusChangeTime\": \"%s\"\n", formatTime(fileInfo->st_ctime, readable));
    printf("  }\n");
    printf("}");
}

void printTextOutput(const char *filePath, struct stat *fileInfo, int readable) {
    printf("File Path: %s\n", filePath);
    printf("  Inode Number: %ld\n", (long)fileInfo->st_ino);
    printf("  Type: %s\n", S_ISDIR(fileInfo->st_mode) ? "directory" : (S_ISLNK(fileInfo->st_mode) ? "symbolic link" : "regular file"));
    printf("  Permissions: %s\n", getPermissions(fileInfo->st_mode));
    printf("  Link Count: %ld\n", (long)fileInfo->st_nlink);
    printf("  UID: %d\n", fileInfo->st_uid);
    printf("  GID: %d\n", fileInfo->st_gid);

    if(readable == 1){printf("  Size: %s\n", getHumanReadableSize(fileInfo->st_size));}
    else printf("  Size: %lld bytes\n",(fileInfo->st_size));
    
    printf("  Access Time: %s\n", formatTime(fileInfo->st_atime, readable));
    printf("  Modification Time: %s\n", formatTime(fileInfo->st_mtime, readable));
    printf("  Status Change Time: %s\n", formatTime(fileInfo->st_ctime, readable));
}

char* getPermissions(mode_t mode) {
    static char perms[11];
    perms[0] = S_ISDIR(mode) ? 'd' : S_ISLNK(mode) ? 'l' : '-';
    perms[1] = (mode & S_IRUSR) ? 'r' : '-';
    perms[2] = (mode & S_IWUSR) ? 'w' : '-';
    perms[3] = (mode & S_IXUSR) ? 'x' : '-';
    perms[4] = (mode & S_IRGRP) ? 'r' : '-';
    perms[5] = (mode & S_IWGRP) ? 'w' : '-';
    perms[6] = (mode & S_IXGRP) ? 'x' : '-';
    perms[7] = (mode & S_IROTH) ? 'r' : '-';
    perms[8] = (mode & S_IWOTH) ? 'w' : '-';
    perms[9] = (mode & S_IXOTH) ? 'x' : '-';
    perms[10] = '\0';

    // Handle special permissions: setuid, setgid, sticky bit
    if (mode & S_ISUID) perms[3] = (perms[3] == 'x') ? 's' : 'S';
    if (mode & S_ISGID) perms[6] = (perms[6] == 'x') ? 's' : 'S';
    if (mode & S_ISVTX) perms[9] = (perms[9] == 'x') ? 't' : 'T';

    return perms;
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
        printTextOutput(filePath, &fileInfo, humanReadable); 
    }
}

char *formatTime(time_t time, int readable) {
    static char buffer[20];
    if (readable) {
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", localtime(&time));
        return buffer;
     } 
    else {
        sprintf(buffer, "%ld", (long)time);
        return buffer;
    }
}

char* getHumanReadableSize(off_t size) {
    static char readableSize[20];
    if (size < 1024) sprintf(readableSize, "%lldB", (long long)size);
    else if (size < 1024 * 1024) sprintf(readableSize, "%.1fK", size / 1024.0);
    else if (size < 1024 * 1024 * 1024) sprintf(readableSize, "%.1fM", size / (1024.0 * 1024));
    else sprintf(readableSize, "%.1f G", size / (1024.0 * 1024 * 1024));
    return readableSize;
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