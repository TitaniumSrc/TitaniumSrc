#include "version.h"
#include "platform.h"
#include "util.h"

#include <stdio.h>
#include <string.h>

#include "glue.h"

char* titlestr;
char verstr[128];
char platstr[128];

void makeVerStrs(void) {
    titlestr = "TitaniumSrc " STR(TISRC_BUILD);
    static const char* months[12] = {
        "Jan", "Feb", "Mar", "Apr",
        "May", "Jun", "Jul", "Aug",
        "Sep", "Oct", "Nov", "Dec"
    };
    snprintf(
        verstr, sizeof(verstr),
        "TitaniumSrc build %u (%s %u, %u; rev %u)",
        (unsigned)TISRC_BUILD,
        months[(((unsigned)TISRC_BUILD / 10000) % 100) - 1],
        ((unsigned)TISRC_BUILD / 100) % 100,
        (unsigned)TISRC_BUILD / 1000000,
        ((unsigned)TISRC_BUILD % 100) + 1
    );
    snprintf(
        platstr, sizeof(platstr),
        "Platform: %s (Platform ID " STR(PLATFORM) "); Architecture: " ARCHSTR,
        platname[PLATFORM]
    );
}
