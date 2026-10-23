#ifndef AUTH_H
#define AUTH_H

#include "common.h"


int auth_check_credentials(const char *username, const char *password);
AppStatus auth_login(void);

#endif


