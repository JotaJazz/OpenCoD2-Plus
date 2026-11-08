#include "common_types.h"
#include "imports.h"
#include "cod2_feature_config.h"

extern char buf[64];

char *getBuildNumber(void)
{
#if COD2_IS_PATCH_13

    sprintf(buf, "%s %s", "pc_1.3_1_1", "Mon May 01 2006 05:05:43PM");
#else
    sprintf(buf, "%s %s", "crc6_v1", "Thu Oct 06 2005 07:13:42PM");
#endif
    return buf;
}
