 #if !defined(ERRORS_H_)

 #define ERRORS_H_

 #define OK 0

// CLIENT errors

#define INPUT_TYPE_ERROR            -100
#define INCONSISTENT_INPUT_ERROR    -101
#define NAN_INPUT_ERROR             -102
#define INVALID_NUMBER_INPUT_ERROR  -103

#define MISSING_SOCKET_NAME     -200
#define FILESYSTEM_ERROR        -201
#define NOT_A_FOLDER            -202

#define OPEN_FAILED             -203

// SERVER ERRORS

#define CONFIG_FILE_ERROR       -300
#define INVALID_LAST_OPERATION  -301
#define LRU_FAILURE             -302
#define FILE_TOO_BIG            -303
#define FILE_AMOUNT_LIMIT       -304

// UTILITY errors

#define NAN_ERROR       -400
#define INVALID_NUMBER  -401
#define FILE_NOT_FOUND  -402

// FILE errors

#define ALREADY_OPENED      -500
#define NOT_OPENED          -501
#define ALREADY_CLOSED      -502
 
#endif