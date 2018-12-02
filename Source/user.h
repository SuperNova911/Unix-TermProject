#pragma once

#include <stdbool.h>
#include <time.h>

typedef enum
{
    USER_NONE = 0, USER_ADMIN, USER_PROFESSOR, USER_STUDENT
} Role;

typedef struct User_t
{
    char studentID[16];
    char hashedPassword[64];
    char userName[16];
    Role role;
    time_t registerDate;
} User;

typedef struct UserInfo_t
{
    char studentID[16];
    char userName[16];
    Role role;
} UserInfo;

typedef struct OnlineUser_t
{
    User user;
    int socket;
    int currentLectureID;
    bool onChat;
} OnlineUser;

User buildUser(char *studentID, char *hashedPassword, char *userName, Role role, time_t registerDate);
UserInfo buildUserInfo(char *studentID, char *userName, Role role);
UserInfo getUserInfo(User *user);

void resetUser(User *user);
void resetUserInfo(UserInfo *userInfo);

bool hasPermission(Role role);