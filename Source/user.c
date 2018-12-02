#include "user.h"
#include <string.h>

const char *UserRoleString[4] = 
{
    "None", "Admin", "Professor", "Student"
};

User createUser(char *studentID, char *hashedPassword, char *userName, Role role)
{
    User user;
    resetUser(&user);

    strncpy(user.studentID, studentID, sizeof(user.studentID));
    strncpy(user.hashedPassword, hashedPassword, sizeof(user.hashedPassword));
    strncpy(user.userName, userName, sizeof(user.userName));
    user.role = role;
    time(&user.registerDate);

    return user;
}

void buildUser(User *user, char *studentID, char *hashedPassword, char *userName, Role role, time_t registerDate)
{
    resetUser(user);

    strncpy(user->studentID, studentID, sizeof(user->studentID));
    strncpy(user->hashedPassword, hashedPassword, sizeof(user->hashedPassword));
    strncpy(user->userName, userName, sizeof(user->userName));
    user->role = role;
    user->registerDate = registerDate;
}

UserInfo buildUserInfo(char *studentID, char *userName, Role role)
{
    UserInfo userInfo;
    resetUserInfo(&userInfo);

    strncpy(userInfo.studentID, studentID, sizeof(userInfo.studentID));
    strncpy(userInfo.userName, userName, sizeof(userInfo.userName));
    userInfo.role = role;

    return userInfo;
}

UserInfo getUserInfo(User *user)
{
    UserInfo userInfo;
    resetUserInfo(&userInfo);

    strncpy(userInfo.studentID, user->studentID, sizeof(userInfo.studentID));
    strncpy(userInfo.userName, user->userName, sizeof(userInfo.userName));
    userInfo.role = user->role;

    return userInfo;
}

void resetUser(User *user)
{
    memset(user, 0, sizeof(User));
}

void resetUserInfo(UserInfo *userInfo)
{
    memset(userInfo, 0, sizeof(userInfo));
}

bool hasPermission(Role role)
{
    return (role == USER_ADMIN || role == USER_PROFESSOR) ? true : false;
}