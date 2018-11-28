#include "dataPack.h"

#include <stdlib.h>
#include <string.h>

const char *UserRoleString[4] = 
{
    "None",
    "Admin", "Student", "Professor"
};

const char *CommandString[37] = 
{
    "NONE",
    "USER_LOGIN_REQUEST", "USER_LOGOUT_REQUEST",
    "LECTURE_LIST_REQUEST", "LECTURE_CREATE_REQUEST", "LECTURE_REMOVE_REQUEST",
    "LECTURE_ENTER_REQUEST", "LECTURE_LEAVE_REQUEST", "LECTURE_REGISTER_REQUEST", "LECTURE_DEREGISTER_REQUEST",
    "ATTENDANCE_START_REQUEST", "ATTNEDANCE_STOP_REQUEST", "ATTNEDANCE_EXTEND_REQUEST", "ATTENDANCE_RESULT_REQUEST", "ATTENDANCE_CHECK_REQUEST",
    "CHAT_ENTER_REQUEST", "CHAT_LEAVE_REQUEST", "CHAT_USER_LIST_REQUEST", "CHAT_SEND_MESSAGE_REQUEST",
    
    "USER_LOGIN_RESPONSE", "USER_LOGOUT_RESPONSE",
    "LECTURE_LIST_RESPONSE", "LECTURE_CREATE_RESPONSE", "LECTURE_REMOVE_RESPONSE",
    "LECTURE_ENTER_RESPONSE", "LECTURE_LEAVE_RESPONSE", "LECTURE_REGISTER_RESPONSE", "LECTURE_DEREGISTER_RESPONSE",
    "ATTENDANCE_START_RESPONSE", "ATTNEDANCE_STOP_RESPONSE", "ATTNEDANCE_EXTEND_RESPONSE", "ATTENDANCE_RESULT_RESPONSE", "ATTENDANCE_CHECK_RESPONSE",
    "CHAT_ENTER_RESPONSE", "CHAT_LEAVE_RESPONSE", "CHAT_USER_LIST_RESPONSE", "CHAT_SEND_MESSAGE_RESPONSE",
};

void resetDataPack(DataPack *dataPack)
{
    memset(dataPack, 0, sizeof(DataPack));
}

void buildDataPack(DataPack *targetDataPack, char *data1, char *data2, char *data3, char *data4, char *message)
{
    if (data1 != NULL)
        strncpy(targetDataPack->data1, data1, sizeof(targetDataPack->data1));
    if (data2 != NULL)
        strncpy(targetDataPack->data2, data2, sizeof(targetDataPack->data2));
    if (data3 != NULL)
        strncpy(targetDataPack->data3, data3, sizeof(targetDataPack->data3));
    if (data4 != NULL)
        strncpy(targetDataPack->data4, data4, sizeof(targetDataPack->data4));
    if (message != NULL)
        strncpy(targetDataPack->message, message, sizeof(targetDataPack->message));
}