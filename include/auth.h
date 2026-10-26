#ifndef AUTH_H
#define AUTH_H

#include <stdio.h>
#include "common.h"


int auth_check_credentials(const char *username, const char *password);
AppStatus auth_login(void);
AppStatus auth_login_from(FILE *input, FILE *output);

#endif


