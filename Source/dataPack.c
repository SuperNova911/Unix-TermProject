#include "dataPack.h"

#include <stdlib.h>
#include <string.h>

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