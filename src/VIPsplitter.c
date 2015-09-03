/*
 ============================================================================
 Name        : VIPsplitter.c
 Author      : Thiago Palmieri
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>

enum exit_codes
{
	X_SUCCESS = 0,
	X_FAILARGS,
	X_FAILFILEOPEN,
	X_WRONGFILETYPE,
	X_FILERROR,
	X_NULL
};

typedef enum states
{
	STATE_INITIAL = 0,
	STATE_COMMENT,
	STATE_HEADER,
	STATE_DATA,
	STATE_NULL
} e_states;

e_states main_state = STATE_INITIAL;

char header[255] = "";
char readline[255] = "";
char * charptr;

unsigned long current_line = 0;

FILE * file;

typedef struct files_t
{
	uint8_t header_saved;
	char name[128];
	FILE * filename;
} files_t;

files_t array[16];


void closeFiles();

/**
 * \brief exit function
 *
 * \param exit_code exit code to be used
 *
 */
void myexit(uint8_t exit_code)
{
	switch (exit_code)
	{
		case X_SUCCESS:
			puts("VIPMeter File Splitter is over, Thanks for using!");
			break;
		case X_FAILARGS:
			printf("Usage: %s filename.csv\r\n", charptr);
			break;
		case X_FAILFILEOPEN:
			printf("File %s does not exist, exiting!\r\n", charptr);
			break;
		case X_WRONGFILETYPE:
			printf("%s is not a VIPMeter generated file, exiting!\r\n", charptr);
			break;
		case X_FILERROR:
			printf("%s has errors on line %lu, exiting!\r\n", charptr, current_line);
			break;
		default:
			break;
	}
	if (NULL != file)					// Close file if open
	{
		fclose(file);
	}
	closeFiles();
	exit(EXIT_SUCCESS);
}

int findString(char * to_find, char * where_to_look)
{
	int result = -1;
	char * ptstr;

	ptstr = strstr(where_to_look, to_find); //find address of substring
	if (ptstr == NULL)						//return -1 if not found
	{
		return result;
	}
	result = ptstr - where_to_look; 		// find the "distance" of substring in int

	return result;
}

int stateChange(e_states new_state)
{
	int result = 0;
	switch (new_state) {
		case STATE_COMMENT:
			//main_state <= STATE_COMMENT ? main_state = STATE_COMMENT : result = 0;
			if (main_state <= STATE_COMMENT)
			{
				main_state = STATE_COMMENT;
				result = 1;
			}
			break;
		case STATE_HEADER:
			//main_state == STATE_COMMENT ? main_state = STATE_HEADER : result = 0;
			if (main_state == STATE_COMMENT)
			{
				main_state = STATE_HEADER;
				result = 1;
			}
			break;
		case STATE_DATA:
			//main_state == STATE_HEADER ? main_state = STATE_DATA : result = 0;
			if ((main_state == STATE_HEADER) || (main_state == STATE_DATA))
			{
				main_state = STATE_DATA;
				result = 1;
			}
			break;
		case STATE_INITIAL:
			//main_state == STATE_HEADER ? main_state = STATE_DATA : result = 0;
			if (main_state == STATE_INITIAL)
			{
				main_state = STATE_INITIAL;
				result = 1;
			}
			break;
		default:
			result = 0;
			break;
	}
	return result;
}

int createFile(uint8_t pos, char * name)
{
	int result = 1;
	if (array[pos].filename == NULL)
	{
		strcpy(array[pos].name, name);
		array[pos].filename = fopen(name, "w");
		if (array[pos].filename == NULL)
		{
			result = 0;
		} else
		{
			fprintf(stdout, "created: %s\r\n", name);
			array[pos].header_saved = 0;
		}
	}

	return result;
}

int saveinFile(uint8_t pos, char * data)
{

	int result = 0;
	if (array[pos].filename != NULL)
	{
		if (!array[pos].header_saved)
		{
			fputs(header, array[pos].filename);
			array[pos].header_saved = 1;
		}
		fputs(data, array[pos].filename);
		//fputs("\r\n", array[pos].filename);

		result = 1;
	}
	return result;
}

void closeFiles()
{
	int index = 0;
	while (index != 16)
	{
		if (NULL != array[index].filename)					// Close file if open
		{
			fclose(array[index].filename);
			array[index].filename = NULL;
		}
		index++;
	}
}


void handleData(char * data, char * origin_filename)
{
	char name[255];
	char temp[10];
	uint8_t name_number;

	name_number = data[3] - '0';

	strcpy(name, origin_filename);

	sprintf(temp, "_0x4%c.csv", data[3]);

	char * ptstr;
	ptstr = strstr(name, ".csv"); //find address of substring
	if (ptstr == NULL)
	{
		myexit(X_WRONGFILETYPE);
	}

	int result = ptstr - name;
	//fprintf(stderr, "result %d\r\n", result);

	int index = strlen(temp);
	index += result;

	int i = 0;

	while (result != (index))
	{
		name[result] = temp[i];
		result++;
		i++;
	}
	name[result] = '\0';

/*	fprintf(stderr, "number: %d\r\n", name_number);
	fprintf(stderr, "filename: %s\r\n", name);*/

	createFile(name_number, name);		// create file
	saveinFile(name_number, data);		// store header/data

}

int main(int argc, char * argv[]) {
	puts("VIPSplitter, a VIPMeter file splitter\r\n");
	if (argc != 2)						// Wrong number of input arguments
	{
		charptr = argv[0];
		myexit(X_FAILARGS);
	}

	file = fopen(argv[1],"r");

	if (NULL == file)					// File does not exist
	{
		charptr = argv[1];
		myexit(X_FAILFILEOPEN);
	}

	charptr = argv[1];
	main_state = STATE_INITIAL;
	current_line = 0;

	char ch;
	uint8_t index = 0;

	puts("Please Wait, this may take a while. Working... ");

	ch = fgetc(file);
	while (ch != EOF)
	{
		readline[index] = ch;

		if (ch == '\n')
		{
			current_line++;
			readline[index + 1] = '\0';
			index = 0;
			//printf("%s", readline);
			// When a line is read, check what kind of line it is
			if (findString("@", readline) == 0)
			{
				if (!stateChange(STATE_COMMENT))
				{
					myexit(X_FILERROR);
				}

			} else if(findString("Address", readline) == 0)
			{
				if (!stateChange(STATE_HEADER))
				{
					myexit(X_FILERROR);
				}

				strcpy(header, readline);

			} else if(findString("0x", readline) == 0)
			{
				if (!stateChange(STATE_DATA))
				{
					myexit(X_FILERROR);
				}
				// Store data in new file
				handleData(readline, charptr);

			} else
			{
				if (!stateChange(STATE_INITIAL))
				{
					myexit(X_FILERROR);
				}
			}


		} else
		{
			index++;
		}

		ch = fgetc(file);
	}
	closeFiles();

	myexit(X_SUCCESS);
	return EXIT_SUCCESS;				// Non reachable code, used to avoid warnings
}
