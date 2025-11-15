// Provided under the GPL v2 license. See the included LICENSE.txt for details.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include "statements.h"
#include "keywords.h"
#include <math.h>
#define BB_VERSION_INFO "batari Basic v1.9 (c)2025\n"
#define BB_MAX_LINE 2048

int includesfile_user_override = 0;

static char (*bb_define_names)[BB_REDEF_ENTRY_LENGTH] = NULL;
static char (*bb_define_values)[BB_REDEF_ENTRY_LENGTH] = NULL;
static size_t bb_define_capacity = 0;

static void bb_allocate_define_storage(size_t capacity)
{
    if (capacity < BB_MIN_REDEF_CAPACITY)
    {
        capacity = BB_MIN_REDEF_CAPACITY;
    }
    if (capacity > BB_MAX_REDEF_CAPACITY)
    {
        capacity = BB_MAX_REDEF_CAPACITY;
    }

    bb_define_names =
        (char (*)[BB_REDEF_ENTRY_LENGTH]) calloc(capacity, sizeof(*bb_define_names));
    bb_define_values =
        (char (*)[BB_REDEF_ENTRY_LENGTH]) calloc(capacity, sizeof(*bb_define_values));
    if (!bb_define_names || !bb_define_values)
    {
        fprintf(stderr,
                "ERROR: Unable to allocate space for %zu symbol definitions.\n",
                capacity);
        exit(1);
    }
    bb_define_capacity = capacity;
}

static void bb_ensure_define_capacity(size_t index)
{
    size_t minimum_capacity = index + 1;

    if (bb_define_capacity == 0)
    {
        size_t initial = bb_get_configured_redefinition_limit();
        if (initial < minimum_capacity)
        {
            initial = minimum_capacity;
        }
        bb_allocate_define_storage(initial);
    }

    while (minimum_capacity > bb_define_capacity)
    {
        size_t new_capacity = bb_define_capacity * 2;
        if (new_capacity < bb_define_capacity || new_capacity > BB_MAX_REDEF_CAPACITY)
        {
            new_capacity = BB_MAX_REDEF_CAPACITY;
        }
        if (new_capacity < minimum_capacity)
        {
            fprintf(stderr,
                    "ERROR: Maximum number of symbol definitions (%d) exceeded.\n",
                    BB_MAX_REDEF_CAPACITY);
            exit(1);
        }

        char (*new_names)[BB_REDEF_ENTRY_LENGTH] =
            (char (*)[BB_REDEF_ENTRY_LENGTH]) realloc(bb_define_names,
                                                      new_capacity * sizeof(*bb_define_names));
        char (*new_values)[BB_REDEF_ENTRY_LENGTH] =
            (char (*)[BB_REDEF_ENTRY_LENGTH]) realloc(bb_define_values,
                                                      new_capacity * sizeof(*bb_define_values));

        if (!new_names || !new_values)
        {
            fprintf(stderr,
                    "ERROR: Unable to expand symbol definition storage to %zu entries.\n",
                    new_capacity);
            exit(1);
        }

        bb_define_names = new_names;
        bb_define_values = new_values;

        {
            size_t i;
            for (i = bb_define_capacity; i < new_capacity; ++i)
            {
                bb_define_names[i][0] = '\0';
                bb_define_values[i][0] = '\0';
            }
        }

        bb_define_capacity = new_capacity;
    }
}

static int bb_token_equals(const char *token, const char *word)
{
    size_t i = 0;

    if (!token || !word)
	return 0;

    while (token[i] && word[i])
    {
	if (tolower((unsigned char) token[i]) != tolower((unsigned char) word[i]))
	    return 0;
	i++;
    }

    return (token[i] == '\0') && (word[i] == '\0');
}

static const char *bb_next_token(const char *line, char *token, size_t maxlen)
{
    size_t out = 0;
    const unsigned char *cursor = (const unsigned char *) line;

    while (*cursor && isspace(*cursor))
	cursor++;

    while (*cursor && !isspace(*cursor) && (out + 1 < maxlen))
    {
	token[out++] = (char) *cursor++;
    }

    token[out] = '\0';

    if (out > 0 && token[out - 1] == ':')
	token[out - 1] = '\0';

    return (const char *) cursor;
}

static int bb_should_convert_commas(const char *line)
{
    char tokens[4][BB_MAX_LINE];
    const char *cursor = line;
    int i;

    for (i = 0; i < 4; ++i)
    {
	cursor = bb_next_token(cursor, tokens[i], sizeof(tokens[i]));
    }

    if (bb_token_equals(tokens[0], "rem"))
	return 0;

    if (bb_token_equals(tokens[0], "on")
	&& (bb_token_equals(tokens[2], "goto")
	    || bb_token_equals(tokens[2], "gosub")))
	return 1;

    if (bb_token_equals(tokens[1], "on")
	&& (bb_token_equals(tokens[3], "goto")
	    || bb_token_equals(tokens[3], "gosub")))
	return 1;

    return 0;
}

extern int bank;

extern int bs;
extern int isPXE;
extern int numconstants;
extern int playfield_index[];
extern int line;

int main(int argc, char *argv[])
{
    char **statement;
    int i, j, k;
    int unnamed = 0;
    int defcount = 0;
    char *c;
    char single;
    char code[BB_MAX_LINE];
    char displaycode[BB_MAX_LINE];
    FILE *header = NULL;
    int multiplespace = 0;
    char *includes_file = "default.inc";
    char *filename = "2600basic_variable_redefs.h";
    char *path = 0;
    char finalcode[BB_MAX_LINE];
    char *codeadd;
    char mycode[BB_MAX_LINE];
    int defi = 0;
    // get command line arguments
    while ((i = getopt(argc, argv, "i:r:v")) != -1)
    {
	switch (i)
	{
	case 'i':
	    path = (char *) malloc(BB_MAX_LINE);
	    path = optarg;
	    break;
	case 'r':
	    filename = (char *) malloc(100);
	    //strcpy(filename, optarg);
	    filename = optarg;
	    break;
	case 'v':
	    printf("%s", BB_VERSION_INFO);
	    exit(0);
	case '?':
	    fprintf(stderr, "usage: %s -r <variable redefs file> -i <includes path>\n", argv[0]);
	    exit(1);
	}
    }

    fprintf(stderr, BB_VERSION_INFO);

    printf("game\n");		// label for start of game
    header_open(header);
    init_includes(path);

    playfield_index[0]=0;

    bb_allocate_define_storage(bb_get_configured_redefinition_limit());

    statement = (char **) malloc(sizeof(char *) * 200);
    for (i = 0; i < 200; ++i)
    {
	statement[i] = (char *) malloc(sizeof(char) * 200);
    }

    while (1)
    {				// clear out statement cache
	for (i = 0; i < 200; ++i)
	{
	    for (j = 0; j < 200; ++j)
	    {
		statement[i][j] = '\0';
	    }
	}
	c = fgets(code, BB_MAX_LINE, stdin);	// get next line from input
	incline();
	strcpy(displaycode, code);

	// look for defines and remember them
	strcpy(mycode, code);
        int k_def_search; // Use a different loop variable to avoid conflict with outer 'i'
        for (k_def_search = 0; k_def_search < BB_MAX_LINE - 5; ++k_def_search)
	    if (code[k_def_search] == ' ')
		break;
        if (k_def_search < BB_MAX_LINE - 5 && code[k_def_search] == ' ' && /* Ensure space was found */
            (k_def_search + 4 < BB_MAX_LINE - 1) && /* Bounds check for code access */
        code[k_def_search + 1] == 'd' && code[k_def_search + 2] == 'e' && 
        code[k_def_search + 3] == 'f' && code[k_def_search + 4] == ' ')
	{			// found a define
	    int current_pos = k_def_search + 5; // current_pos now points to start of define name.

	    bb_ensure_define_capacity((size_t) defi);

	    for (j = 0; current_pos < BB_MAX_LINE - 1 && code[current_pos] != ' ' && code[current_pos] != '\0' && code[current_pos] != '\n' && code[current_pos] != '\r'; current_pos++)
	    {
	        if (j >= 99) {
	            fprintf(stderr, "(%d) ERROR: Define name too long (max 99 chars).\n", bbgetline());
		    exit(1);
		}
		bb_define_names[defi][j++] = code[current_pos];
	    }
	    bb_define_names[defi][j] = '\0';

	    if (j == 0) { // Empty define name
	        fprintf(stderr, "(%d) ERROR: Malformed define statement. Empty define name.\n", bbgetline());
	         exit(1);
	    }

	    // Expect " = " sequence after define name
	    if (!(current_pos <= BB_MAX_LINE - 3 && code[current_pos] == ' ' && code[current_pos+1] == '=' && code[current_pos+2] == ' ')) {
	        fprintf(stderr, "(%d) ERROR: Malformed define statement. Expected \" = \" after define name '%s'.\n", bbgetline(), bb_define_names[defi]);
		exit(1);
	    }
	    current_pos += 3; // Skip " = "

	    for (j = 0; current_pos < BB_MAX_LINE - 1 && code[current_pos] != '\0' && code[current_pos] != '\n' && code[current_pos] != '\r'; current_pos++)
	    {
	        if (j >= 99) {
	            fprintf(stderr, "(%d) ERROR: Define replacement string too long (max 99 chars) for define '%s'.\n", bbgetline(), bb_define_names[defi]);
	            exit(1);
	        }
	        bb_define_values[defi][j++] = code[current_pos];
	    }
	    bb_define_values[defi][j] = '\0';
	    removeCR(bb_define_values[defi]);
	    printf (";PARSED_DEFINE: .%s. = .%s.\n", bb_define_names[defi], bb_define_values[defi]); // Clarified debug print
	    defi++;
	}
	else if (defi) // This 'i' refers to the outer loop variable for iterating through existing defines
	{
            int def_idx;
	    for (def_idx = 0; def_idx < defi; ++def_idx) // Use new loop var def_idx
	    {
		codeadd = NULL;
		finalcode[0] = '\0';
		defcount = 0;
		while (1)
		{
		    if (defcount++ > BB_MAX_LINE)
		    {
			fprintf(stderr, "(%d) Infinitely repeating definition or too many instances of a definition\n",
				bbgetline());
			exit(1);
		    }
		    codeadd = strstr (mycode, bb_define_names[def_idx]);
		    if (codeadd == NULL)
			break;
		    for (j = 0; j < BB_MAX_LINE; ++j)
			finalcode[j] = '\0';
		    strncpy(finalcode, mycode, strlen(mycode) - strlen(codeadd));
		    strcat (finalcode, bb_define_values[def_idx]);
		    strcat (finalcode, codeadd + strlen (bb_define_names[def_idx]));
		    strcpy(mycode, finalcode);
		}
	    }
	}
	if (strcmp(mycode, code))
	    strcpy(code, mycode);
	if (!c)
	    break;		//end of file

// preprocessing removed in favor of a simplistic lex-based preprocessor

	i = 0;
	j = 0;
	k = 0;

// look for spaces, reject multiples
	int convert_commas = bb_should_convert_commas(code);
	while (code[i] != '\0')
	{
	    single = code[i++];
	    if (single == ',' && convert_commas)
	    {
		// Treat commas exactly like spaces so constructs such as
		// "on x goto label0, label1" parse identically to the
		// space-separated form without extra syntax handling.
		single = ' ';
	    }
	    if (single == ' ')
	    {
		if (!multiplespace)
		{
		    j++;
		    k = 0;
		}
		multiplespace++;
	    }
	    else
	    {
		multiplespace = 0;
		if (k < 199)	// avoid overrun with long horizontal separators
		    statement[j][k++] = single;
	    }

	}
	if (j > 190)
	{
	    fprintf(stderr, "(%d) Warning: long line\n", bbgetline());
	}
	if (statement[0][0] == '\0')
	{
	    sprintf(statement[0], "L0%d", unnamed++);
	}
	else
	{
	    if (strchr(statement[0], '.') != NULL)
	    {
		fprintf(stderr, "(%d) Invalid character in label.\n", bbgetline());
		exit(1);
	    }

	}
	if (strncmp(statement[0], "end\0", 3))
            printf (".%s ;;line %d;; %s\n", statement[0], line, displaycode);
	else
	    doend();

	keywords(statement);
        if(numconstants==(MAXCONSTANTS-1))
        { 
		fprintf(stderr, "(%d) Maximum number of constants exceeded.\n", bbgetline());
		exit(1);
        }

    }
    bank = bbank();
    bs = bbs();
    barf_sprite_data();

    printf(" if ECHOFIRST\n");
    if (bs == 28){
        if(isPXE)
            printf("       echo \"    \",[(end_of_address_space - *)]d , \"bytes of ROM space left");
        else
            printf("       echo \"    \",[(DPC_graphics_end - *)]d , \"bytes of ROM space left");
    } else
	printf("       echo \"    \", [(*)]h , \" (\" , [($10000 - *)]d , \" bytes left to $10000)\"");
    if (bs == 8)
	printf(" in bank 2");
    if (bs == 16)
	printf(" in bank 4");
    if ((bs == 28) && !isPXE)
	printf(" in graphics bank");
    if (bs == 32)
	printf(" in bank 8");
    if (bs != 64) {
	printf("\")\n");
    }
    printf(" endif \n");
    printf("ECHOFIRST = 1\n");
    printf(" \n");
    /* Bank reporting moved to statements.c newbank() function */
    /* Banks 1-15 are reported when we encounter the NEXT bank (in newbank())
     * Bank 16 is reported at the end of newbank(16) since there's no bank 17
     * This ensures calculations happen in the correct address space
     */
    
    printf(" \n");
    printf(" \n");
    header_write(header, filename);
    if (!includesfile_user_override)
    {
        create_includes(includes_file);
    }
    
    fprintf(stderr, "2600 Basic compilation complete.\n");
    return 0;
}
