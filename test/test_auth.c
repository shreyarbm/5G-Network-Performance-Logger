#include <stddef.h>
#include <stdio.h>

#include "auth.h"

int main(void)
{
    if (auth_check_credentials("demo", "5g-demo") == 0 ||
        auth_check_credentials("demo", "wrong") != 0 ||
        auth_check_credentials("", "5g-demo") != 0 ||
        auth_check_credentials("demo", "") != 0 ||
        auth_check_credentials(NULL, "x") != 0) {
        puts("FAIL: authentication checks");
        return 1;
    }
    puts("PASS: authentication checks");
    return 0;
}
