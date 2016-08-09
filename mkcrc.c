/* $Id: mkcrc.c,v 2.2 2005/11/07 23:22:18 luis Exp $
 * Author: Luis Colorado <lc@luiscoloradosistemas.com>
 * Date: Thu Nov  3 10:05:42 CET 2005
 *
 * Disclaimer:
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#define IN_MKCRC_C

#define F(fmt) "%s:%d:%s:" fmt, __FILE__, __LINE__, __func__

/* Standard include files */
#include <ctype.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "crc.h"
#include "mkcrc.h"
#include "bits.h"

/* variables */

int config_flags = 0;

/* functions */
static void do_usage(void);
static int check_name(char *s);
static inline long cycle(CRC_STATE p);

static void do_usage(void)
{
} /* do_usage */

static inline long cycle(CRC_STATE p)
{
	int v1, v2;
	CRC_STATE res = 0;

	v1 = v2 = 1;

	/* buscamos la repetición */
	do {
		v1 = slow_do_crc(1, v1, p); /* iteramos una vez */
		v2 = slow_do_crc(2, v2, p); /* iteramos dos veces */
	} while (v1 != v2); /* hasta que uno alcance al otro */

	do {
		v2 = slow_do_crc(1, v2, p); /* iteramos */
		res++;
	} while (v1 != v2); /* hasta recorrer una vez el ciclo */
	
	return res;
} /* cycle */

static int check_name(char *s)
{
    if (!isalpha(*s) && *s != '_') return -1;
    while (*++s)
        if (!isalnum(*s) && *s != '_') return -1;
    return 0;
}

/* main program */
int main (int argc, char **argv)
{
	int opt;
	CRC_STATE val;
	CRC_STATE pol_mask = DEFAULT_POL_MASK;
	CRC_STATE limite;
	int conf_index = 0;
	char name[256] = "";

	while ((opt = getopt(argc, argv, "hp:n:lgvL")) != EOF) {
		switch(opt) {
		case 'h': do_usage(); exit(0);
        case 'n':
            if (check_name(optarg)) {
                fprintf(stderr, F("invalid name format: %s\n"), optarg);
                exit(EXIT_FAILURE);
            }
            strncpy(name, optarg, sizeof name);
            break;
		case 'p':
            if (sscanf(optarg, "%llx", &pol_mask) != 1) {
                fprintf(stderr, F("invalid pol mask: %s\n"), optarg);
                exit(EXIT_FAILURE);
            }
            if (check_name(optarg)) {
                snprintf(name, sizeof name,
                        "pol_%#llx", pol_mask);
            }
		    break;
		case 'g': config_flags |= FLAGS_GEN; break;
		case 'v': config_flags |= FLAGS_VERBOSE; break;
		} /* switch */
	} /* while */

	limite = msbmask(pol_mask);

	if (config_flags & FLAGS_LIST) {
		while (pol_mask <= limite) {
			CRC_STATE cy = cycle(pol_mask);

			printf("%20llu: 0x%llx %s",
				cy, pol_mask, (cy == limite) ? " OK" : "");
			if ((cy == limite) || (config_flags & FLAGS_VERBOSE)) {
                char buffer[1024];
				printf(" --> %s", pol2str(pol_mask, buffer, sizeof buffer));
			} /* if */
			printf("\n");
			pol_mask++;
		} /* for */
	} else if (config_flags & FLAGS_GEN) {
		int i;
        int size = msbpos(pol_mask);;
        char buffer[1024];

		printf(
"/* THIS FILE GENERATED AUTOMATICALLY, DON'T EDIT.\n"
" * Author: Luis Colorado <lc@luiscoloradosistemas.com>\n"
" * Copyright: (C) 2014 LUIS COLORADO SISTEMAS, S.L.U.\n"
" *            All rights reserved.\n"
" */\n");
		printf("#include \"crc.h\"\n");
		printf("static struct crc_table_s _%s = {\n", name);

		printf("\t/* Comando usado:");
		for (i = 0; i < argc; i++) printf(" %s", argv[i]);
		printf(" */\n");
        printf("\t/* cr_name    : */ \"%s\",\n",    name);
        printf("\t/* cr_strpolin: */ \"%s\",\n",
                pol2str(pol_mask, buffer, sizeof buffer)); 
        printf("\t/* cr_size    : */ %lu,\n",       size);
        printf("\t/* cr_bytesize: */ %lu,\n",       (size+7) / 8);
        printf("\t/* cr_polin   : */ %#llx,\n",     pol_mask);
        printf("\t/* cr_mask    : */ %#llx,\n",     limite);
        printf("\t/* cr_table[] : */ {\n");

		for (val = 0; val < BYTE_VALUES; val++) {
			if (val % N_PER_LINE == 0) printf("\t/* %3llu */", val);
			printf(" 0x%llx,", slow_do_crc(BITS_PER_BYTE, val, pol_mask));
			if (val % N_PER_LINE == N_PER_LINE - 1) printf("\n");
		} /* for */
		if (val % N_PER_LINE != 0) printf("\n");
		printf("}}; /* %s */\n", name);
        printf("\n");
        printf("CRC_TABLE %s = &_%s;\n", name, name);

	} /* if */

	return 0;

} /* main */

/* $Id: mkcrc.c,v 2.2 2005/11/07 23:22:18 luis Exp $ */
