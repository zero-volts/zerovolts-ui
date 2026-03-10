#include "error_handler.h"

#define MAX_ERRORS          10
#define MESSAGE_MAX_LENGTH  256

static char errors[MAX_ERRORS][MESSAGE_MAX_LENGTH] = {0};
static int current_index_error = 0;

void set_last_error(const char *msg)
{
    if (!msg) 
    {
        int idx = current_index_error - 1;
        if (idx < 0)
            idx = MAX_ERRORS - 1;

        errors[idx][0] = '\0';
        return;
    }

    snprintf(errors[current_index_error], MESSAGE_MAX_LENGTH, "%s", msg);

    current_index_error++;
    if (current_index_error >= MAX_ERRORS )
        current_index_error = 0;
}

const char *last_error(void)
{
    int idx = current_index_error - 1;
    if (idx < 0)
        idx = MAX_ERRORS - 1;

    return errors[idx];
}

void print_all_errors()
{
    for (int idx = 0; idx < MAX_ERRORS; idx++)
    {
        printf("Error [%d]: %s\n", idx, errors[idx]);
    }
}