#include <stdio.h>
#include <string.h>

#include "auth.h"
#include "utils.h"

#define AUTH_MAX_ATTEMPTS 3U
#define AUTH_INPUT_CAPACITY 64U


static const char DEMO_USERNAME[] = "demo";
static const char DEMO_PASSWORD[] = "5g-demo";

int auth_check_credentials(const char *username, const char *password)
{
    if (username == NULL || password == NULL || username[0] == '\0' || password[0] == '\0') {
        return 0;
    }
    return strcmp(username, DEMO_USERNAME) == 0 &&
           strcmp(password, DEMO_PASSWORD) == 0;
}

AppStatus auth_login(void)
{
    char username[AUTH_INPUT_CAPACITY];
    char password[AUTH_INPUT_CAPACITY];
    unsigned int attempt;

    puts("Demo login (not secure; demonstration only).");

    for (attempt = 0U; attempt < AUTH_MAX_ATTEMPTS; ++attempt) {
        if (utils_read_line("Username: ", username, sizeof(username)) != APP_STATUS_OK ||
            username[0] == '\0') {
            puts("Username is empty or could not be read.");
            continue;
        }

        if (utils_read_line("Password: ", password, sizeof(password)) != APP_STATUS_OK ||
            password[0] == '\0') {
            puts("Password is empty or could not be read.");
            continue;
        }

        if (strcmp(username, DEMO_USERNAME) == 0 &&
            strcmp(password, DEMO_PASSWORD) == 0) {
            puts("Demo login successful.");
            return APP_STATUS_OK;
        }

        puts("Invalid username or password.");
    }

    puts("Maximum login attempts reached.");
    return APP_STATUS_ERROR;
}


