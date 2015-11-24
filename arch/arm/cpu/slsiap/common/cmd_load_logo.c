/*
 * (C) Copyright 2009
 * jung hyun kim, Nexell Co, <jhkim@nexell.co.kr>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#include <config.h>
#include <common.h>
#include <command.h>
#include <malloc.h>
#include <mach-api.h>


#if (0)
#define	pr_dbg(msg...)	printf(msg)
#else
#define	pr_dbg(msg...)	do { } while (0)
#endif

#ifndef CONFIG_CMD_LOGO_LOAD
#define	CONFIG_CMD_LOGO_LOAD 	NULL
#endif

extern int run_command (const char *cmd, int flag);

static int parse_logo_cmd (char *line, char *args[])
{
	int nargs = 0;

	while (nargs < CONFIG_SYS_MAXARGS) {

		/* skip any white space */
		while ((*line == ' ') || (*line == '\t') || (*line == ';')) {
			++line;
		}

		if (*line == '\0') {	/* end of line, no more args	*/
			args[nargs] = NULL;
			return (nargs);
		}

		args[nargs++] = line;	/* begin of argument string	*/

		/* find end of string */
		while (*line && (*line != ';') && (*line != '\t')) {
			++line;
		}

		if (*line == '\0') {	/* end of line, no more args	*/
			args[nargs] = NULL;
			return (nargs);
		}

		*line++ = '\0';		/* terminate current arg	 */
	}

	printf ("** Too many args (max. %d) **\n", CONFIG_SYS_MAXARGS);
	return (nargs);
}

#define	COMMNAD_LINE_SIZE 128

/*  priority
 *
 *	1. bootlogo environment
 *	2. loadbmp command parameters run_command("loadbmp \"ext4load mmc 1:1 0x43000000 logo.bmp;bootlogo 0x43000000\"", 0);
 *  3. macro command CONFIG_CMD_LOGO_LOAD
 */
static int do_loadbmp(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	char *default_command = CONFIG_CMD_LOGO_LOAD;
	char **args = NULL;
	char *envs = NULL;
	unsigned int addr = 0;
	int i, ret = 0, use_env;

	/* get 'bootlogo' environment */
	envs = getenv ("bootlogo");
	if (NULL == envs)
		envs = default_command;
	else
		use_env = 1;

	if (NULL == envs && argc == 1) {
		printf("no bootlogo environments...\n");
		return 1;
	}

	/* no input arguments */
	args = (char **)malloc(COMMNAD_LINE_SIZE * 8);
	if  (!args) {
		printf("Fail malloc fot logo command %d\n", COMMNAD_LINE_SIZE * (argc-1));
		return -1;
	}

	if (use_env) {
		argc = parse_logo_cmd(envs, (char **)args);
	} else {
		argc = parse_logo_cmd(argv[1], (char **)args);
	}

	for (i=0; argc > i; ++i) {
		char *arg = args[i];
		pr_dbg("logo arg = %s\n", arg);
		if (strncmp(arg, "bootlogo", strlen("bootlogo")) == 0) {
			char *arg2 = arg + strlen("bootlogo");

			while (*arg2 == ' ') { arg2++; }

			if ((*arg2 == '\t') ||
				(*arg2 == ';')  ||
				(*arg2 == '\0'))
				continue;

			addr = simple_strtoul(arg2, NULL, 16);
			if (addr)
				lcd_set_logo_bmp_addr(addr);
			continue;
		}
		pr_dbg("[run_command = %s]\n", arg);
		if (-1 == run_command (arg, 0)) {
			printf("fail command:%s\n", arg);
			ret = 1;
			break;
		}
	}

	free(args);
	pr_dbg("%s loadbmp\n", ret?"Fail":"DONE");

	return ret;
}

U_BOOT_CMD(
	loadbmp, 2, 1,	do_loadbmp,
	"load bmpfile with command or 'bootlog' environment ",
	"    - load bmpfile to framebuffer with 'bootlogo' command\n"
);

