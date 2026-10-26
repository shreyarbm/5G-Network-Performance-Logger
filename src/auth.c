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

AppStatus auth_login_from(FILE *input, FILE *output)
{
    char username[AUTH_INPUT_CAPACITY];
    char password[AUTH_INPUT_CAPACITY];
    unsigned int attempt;

    if (input == NULL || output == NULL) {
        return APP_STATUS_INVALID_INPUT;
    }
    if (fputs("Demo login (not secure; demonstration only).\n", output) == EOF) {
        return APP_STATUS_ERROR;
    }

    for (attempt = 0U; attempt < AUTH_MAX_ATTEMPTS; ++attempt) {
        if (utils_read_line_from(input, output, "Username: ", username,
                                 sizeof(username)) != APP_STATUS_OK ||
            username[0] == '\0') {
            if (fputs("Username is empty or could not be read.\n", output) == EOF) {
                return APP_STATUS_ERROR;
            }
            continue;
        }

        if (utils_read_line_from(input, output, "Password: ", password,
                                 sizeof(password)) != APP_STATUS_OK ||
            password[0] == '\0') {
            if (fputs("Password is empty or could not be read.\n", output) == EOF) {
                return APP_STATUS_ERROR;
            }
            continue;
        }

        if (strcmp(username, DEMO_USERNAME) == 0 &&
            strcmp(password, DEMO_PASSWORD) == 0) {
            if (fputs("Demo login successful.\n", output) == EOF) {
                return APP_STATUS_ERROR;
            }
            return APP_STATUS_OK;
        }

        if (fputs("Invalid username or password.\n", output) == EOF) {
            return APP_STATUS_ERROR;
        }
    }

    if (fputs("Maximum login attempts reached.\n", output) == EOF) {
        return APP_STATUS_ERROR;
    }
    return APP_STATUS_ERROR;
}

AppStatus auth_login(void)
{
    return auth_login_from(stdin, stdout);
}


