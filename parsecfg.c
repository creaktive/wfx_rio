#include "parsecfg.h"

int parsecfg (char *filename, char comment, int process(char *key, char *val))
{
	FILE *f;
	char buf[LINELEN + 1], *p, *q;
	int len, i, n;

	if ((f = fopen(filename, "rt")) == NULL)
		return 0;

	for (n = 1; fgets(buf, LINELEN, f); n++)
	{
		// cut out trailing spaces
		for (len = strlen(buf) - 1; isspace(buf[len]); len--);
		buf[++len] = '\0';

		// seek for beginning of KEY
		for (p = buf; isspace(*p); p++);
		// is it a comment/white line?
		if (*p && *p != comment)
		{
			// now, seek for ending of KEY
			for (i = 0, q = p; *q && !isspace(*q); q++, i++);
			// ...and beginning of VALUE
			for (; isspace(*q); q++);

			// cut KEY
			p[i] = '\0';

			// check for valid format
			if (!*q)
			{
				fclose(f);
				return n;
			}

			// process KEY/VALUE pair
			if (process(p, q))
			{
				fclose(f);
				return n;
			}
		}
	}

	fclose(f);
	return -1;
}
