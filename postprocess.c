// Provided under the GPL v2 license. See the included LICENSE.txt for details.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

// This reads the includes file created by bB and builds the
// final assembly that will be sent to DASM.
//
// This task used to be done with a batch file/shell script when
// the files in the final asm were static.
// Now, since the files in the final asm can be different, and it doesn't
// make sense to require the user to create a new batch file/shell script.

int main(int argc, char *argv[])
{
    FILE *includesfile;
    FILE *asmfile;
    char ***readbBfile;
    char path[500];
    char prepend[500];
    char line[500];
    char asmline[500];
    int bB = 2;			// part of bB file
    int writebBfile[17] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    int i;
    int j;
    int last_end_bank = 0;
    while ((i = getopt(argc, argv, "i:")) != -1)
    {
	switch (i)
	{
	case '?':
	    path[0] = '\0';
	    break;
	case 'i':
	    strcpy(path, optarg);
	}
    }
    if ((path[strlen(path) - 1] == '\\') || (path[strlen(path) - 1] == '/'))
	strcat(path, "includes/");
    else
	strcat(path, "/includes/");
    if ((includesfile = fopen("includes.bB", "r")) == NULL)	// open file
    {
	fprintf(stderr, "Cannot open includes.bB for reading\n");
	exit(1);
    }
    readbBfile = (char ***) malloc(sizeof(char **) * 17);

//  while (fgets(line,500,includesfile))
    while (1)
    {
	if (fscanf(includesfile, "%s", line) == EOF)
	    break;
	if ((!strncmp(line, "bB", 2)) && (line[2] != '.'))
	{
	    i = (int) (line[2]) - 48;
	    for (j = 1; j < writebBfile[i]; ++j)
		printf("%s", readbBfile[i][j]);
	    continue;
	}
	else
	{
	    strcpy(prepend, path);
	    strcat(prepend, line);
	    if ((asmfile = fopen(line, "r")) == NULL)	// try file w/o includes path
	    {
		if ((asmfile = fopen(prepend, "r")) == NULL)	// open file
		{
		    fprintf(stderr, "Cannot open %s for reading\n", line);
		    exit(1);
		}
	    }
	    else if (strncmp(line, "bB\0", 2))
	    {
		fprintf(stderr, "User-defined %s found in current directory\n", line);
	    }
	}
	while (fgets(asmline, 500, asmfile))
	{
	    if (!strncmp(asmline, "; bB.asm file is split here", 20))
	    {
		writebBfile[bB]++;
		readbBfile[bB] = (char **) malloc(sizeof(char *) * 50000);
		readbBfile[bB][writebBfile[bB]] = (char *) malloc(strlen(line) + 3);
		sprintf(readbBfile[bB][writebBfile[bB]], ";%s\n", line);
	    }
	    char *trimmed = asmline;
	    while (*trimmed == ' ' || *trimmed == '\t')
		trimmed++;
	    if (!strncmp(trimmed, "asm", 3) &&
		(trimmed[3] == '\0' || trimmed[3] == '\n' || trimmed[3] == '\r'))
	    {
		last_end_bank = 0;
		continue;
	    }
	    if (!strncmp(trimmed, "end", 3) &&
		(trimmed[3] == '\0' || trimmed[3] == '\n' || trimmed[3] == '\r'))
	    {
		if (!last_end_bank)
		    continue;
		last_end_bank = 0;
	    }
	    else if (!strncmp(trimmed, "end_bank", 8) && strstr(trimmed, "SET"))
		last_end_bank = 1;
	    else
		last_end_bank = 0;
	    int export_label = 0;
	    if (trimmed[0] == '.' && isupper((unsigned char) trimmed[1]) &&
		(trimmed[2] != '\0') && !isdigit((unsigned char) trimmed[2]))
		export_label = 1;
	    if (!writebBfile[bB])
	    {
		printf("%s", asmline);
		if (export_label)
		    printf("%s\n", trimmed + 1);
	    }
	    else
	    {
		readbBfile[bB][++writebBfile[bB]] =
		    (char *) malloc(strlen(asmline) + 3);
		sprintf(readbBfile[bB][writebBfile[bB]], "%s", asmline);
		if (export_label)
		{
		    size_t alias_len = strlen(trimmed + 1);
		    readbBfile[bB][++writebBfile[bB]] =
			(char *) malloc(alias_len + 2);
		    memcpy(readbBfile[bB][writebBfile[bB]], trimmed + 1,
			   alias_len);
		    readbBfile[bB][writebBfile[bB]][alias_len] = '\n';
		    readbBfile[bB][writebBfile[bB]][alias_len + 1] = '\0';
		}
	    }
	}
	fclose(asmfile);
	if (writebBfile[bB])
	    bB++;
	// if (writebBfile) fclose(bBfile);
    }
}
