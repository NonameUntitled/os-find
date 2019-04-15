#include <iostream>
#include <vector>
#include <exception>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <limits.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

using namespace std;

const char *NAME_FLAG_START = "-name";
const char *INOD_FLAG_START = "-inum";
const char *SIZE_FLAG_START = "-size";
const char *EXEC_FLAG_START = "-exec";
const char *LINK_FLAG_START = "-nlink";

bool IS_NAME_FLAG;
bool IS_EXEC_FLAG;
bool IS_INOD_FLAG;
bool IS_LINK_FLAG;
bool IS_SIZE_FLAG;

char* NAME_FLAG;

char* EXEC_FLAG;

ino_t INOD_FLAG;

nlink_t LINK_FLAG;

// 0 for =
// 1 for -
// 2 for +
int SIZE_SIGN_FLAG;
__off64_t SIZE_FLAG;


void doubleFlagsError(const char *message) {
    char* str = "Error duplicating flag";
    strcat(str, message);
    throw runtime_error(str);
}


template <class T>
void fromStrToNum(T &variable, char *str) {
    variable = 0;
    string tmpStr = str;

    for (char i : tmpStr) {
        if (i >= '0' && i <= '9') {
            variable *= 10;
            variable += i - '0';
        } else {
            throw runtime_error("Wrong value for flag");
        }
    }
}


void checkArgumentsAmount(int argc, int argNumber) {
    if (argNumber >= argc) {
        throw runtime_error("Wrong arguments amount");
    }
}


void processFlags(int argc, char *argv[]) {
    int argNumber = 2;

    while(argNumber < argc) {
        if (strcmp(argv[argNumber], NAME_FLAG_START) == 0) {
            if (IS_NAME_FLAG) {
                doubleFlagsError(NAME_FLAG_START);
            }

            ++argNumber;
            checkArgumentsAmount(argc, argNumber);
            IS_NAME_FLAG = true;
            NAME_FLAG = argv[argNumber];

        } else if (strcmp(argv[argNumber], INOD_FLAG_START) == 0) {
            if (IS_INOD_FLAG) {
                doubleFlagsError(INOD_FLAG_START);
            }

            ++argNumber;
            checkArgumentsAmount(argc, argNumber);
            IS_INOD_FLAG = true;

            fromStrToNum(INOD_FLAG, argv[argNumber]);
        } else if (strcmp(argv[argNumber], SIZE_FLAG_START) == 0) {
            if (IS_SIZE_FLAG) {
                doubleFlagsError(SIZE_FLAG_START);
            }

            ++argNumber;
            checkArgumentsAmount(argc, argNumber);
            IS_SIZE_FLAG = true;
            SIZE_SIGN_FLAG = -1;

            string sizeStr = argv[argNumber];

            for (char i : sizeStr) {
                if (SIZE_SIGN_FLAG == -1) {
                    if (i == '=') {
                        SIZE_SIGN_FLAG = 0;
                    } else if (i == '-') {
                       SIZE_SIGN_FLAG = 1;
                    } else if (i == '+') {
                        SIZE_SIGN_FLAG = 2;
                    } else {
                        throw runtime_error("Wrong sign for size");
                    }
                    continue;
                }

                if (i >= '0' && i <= '9') {
                    SIZE_FLAG *= 10;
                    SIZE_FLAG += i - '0';
                } else {
                    throw runtime_error("Wrong value for flag");
                }
            }
        } else if (strcmp(argv[argNumber], EXEC_FLAG_START) == 0) {
            if (IS_EXEC_FLAG) {
                doubleFlagsError(EXEC_FLAG_START);
            }

            ++argNumber;
            checkArgumentsAmount(argc, argNumber);
            IS_EXEC_FLAG = true;
            EXEC_FLAG = argv[argNumber];

        } else if (strcmp(argv[argNumber], LINK_FLAG_START) == 0) {
            if (IS_LINK_FLAG) {
                doubleFlagsError(LINK_FLAG_START);
            }

            ++argNumber;
            IS_LINK_FLAG = true;
            fromStrToNum(LINK_FLAG, argv[argNumber]);
        } else {
            throw runtime_error("Wrong argument(s)");
        }
        ++argNumber;
    }
}


bool checkFileOnFlags(dirent* curFile, struct stat &fileInfo) {

    if (IS_NAME_FLAG) {
        if (strcmp(curFile->d_name, NAME_FLAG) != 0) {
            return false;
        }
    }

    if (IS_INOD_FLAG) {
        if (fileInfo.st_ino != INOD_FLAG) {
            return false;
        }
    }

    if (IS_LINK_FLAG) {
        if (fileInfo.st_nlink != LINK_FLAG) {
            return false;
        }
    }

    if (IS_SIZE_FLAG) {
        if (SIZE_SIGN_FLAG == 0) {
            if (fileInfo.st_size != SIZE_FLAG) {
                return false;
            }
        }

        if (SIZE_SIGN_FLAG == 1) {
            if (fileInfo.st_size >= SIZE_FLAG) {
                return false;
            }
        }

        if (SIZE_SIGN_FLAG == 2) {
            if (fileInfo.st_size <= SIZE_FLAG) {
                return false;
            }
        }
    }

    return true;
}


void processDirectory(const char*);


void processFile(dirent *curFile, const char *pathToDir) {

    struct stat fileInfo{};
    char *pathToFile = new char[255];
    strcpy(pathToFile, pathToDir);
    strcat(pathToFile, "/");
    strcat(pathToFile, curFile->d_name);

    if (lstat(pathToFile, &fileInfo) == 0) {

        if (S_ISDIR(fileInfo.st_mode)) {
            processDirectory(pathToFile);
        } else if (S_ISREG(fileInfo.st_mode)) {
            if (checkFileOnFlags(curFile, fileInfo)) {
                if (IS_EXEC_FLAG) {

                    pid_t pid = fork();

                    if (pid < 0) {

                    } else if (pid == 0) {

                        int executeResult = execl(EXEC_FLAG, EXEC_FLAG, pathToFile, NULL);
                        if (executeResult == -1) {
                            cout << "Error occurred while executing process given" << endl;
                            exit(EXIT_FAILURE);
                        }
                    } else {
                        int result;
                        pid_t childResult = waitpid(pid, &result, 0);

                        if (childResult == -1) {
                            cout << "Error occurred while executing child process";
                        }
                    }
                } else {
                    cout << pathToFile << endl;
                }
            }
        }
    } else {

    }
}


void processDirectory(const char *pathToDir) {
    DIR *dir = nullptr;
    dirent *curFile = nullptr;

    dir = opendir(pathToDir);
    if (dir == nullptr) {
        throw runtime_error("Error opening a directory");
    }

    curFile = readdir(dir);
    while (curFile != nullptr) {

        if (strcmp(curFile->d_name, ".") != 0 && strcmp(curFile->d_name, "..") != 0) {
            processFile(curFile, pathToDir);
        }

        curFile = readdir(dir);
    }
}


void startProcessing(char *pathToDir) {
    struct stat sb{};

    if (lstat(pathToDir, &sb) == 0) {
        if (S_ISDIR(sb.st_mode)) {
            processDirectory(pathToDir);
        } else {
            throw runtime_error("File is not a library");
        }
    } else {
        throw runtime_error("Error processing path file");
    }
}


int main(int argc, char *argv[]) {
    if (argc < 2) {
        cout << "Wrong argument amount" << endl;
        return 1;
    }

    try {
        processFlags(argc, argv);
        startProcessing(argv[1]);
    } catch (runtime_error &e) {
        cout << e.what() << endl;
    }
}
