#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

/* The hexconvert function is only used to test reference vectors against
 * known answers. The contents of this file are therefore a component
 * to assist with test harnesses only
 */

int libscrypt_hexconvert(uint8_t *buf, size_t s, char *outbuf, size_t obs)
{

        size_t i;
	int len = 0;

        if (!buf || s < 1 || obs < (s * 2 + 1))
                return 0;

        memset(outbuf, 0, obs);
	

        for(i=0; i<=(s-1); i++)
        {
		/* snprintf(outbuf, s,"%s...", outbuf....) has undefined results
		* and can't be used. Using offests like this makes snprintf
		* nontrivial. we therefore have use inescure sprintf() and
		* lengths checked elsewhere (start of function) */
		/*@ -bufferoverflowhigh @*/ 
                len += sprintf(outbuf+len, "%02x", (unsigned int) buf[i]);
        }

	return 1;
}

