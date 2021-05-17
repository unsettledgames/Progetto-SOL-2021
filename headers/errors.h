 #if !defined(ERRORS_H_)

 #define ERRORS_H_

 #define OK 0

// CLIENT errors

#define INPUT_TYPE_ERROR            -1
#define INCONSISTENT_INPUT_ERROR    -2
#define NAN_INPUT_ERROR             -3
#define INVALID_NUMBER_INPUT_ERROR  -4

#define MISSING_SOCKET_NAME     -5
#define FILESYSTEM_ERROR        -6
#define NOT_A_FOLDER            -7

#define OPEN_FAILED             -8

// SERVER ERRORS

#define CONFIG_FILE_ERROR       -1
#define INVALID_LAST_OPERATION  -2

// UTILITY errors

#define NAN_ERROR       -1
#define INVALID_NUMBER  -2
#define FILE_NOT_FOUND  -3

// FILE errors

#define ALREADY_OPENED      -1
#define NOT_OPENED          -2
#define ALREADY_CLOSED      -3

#endif