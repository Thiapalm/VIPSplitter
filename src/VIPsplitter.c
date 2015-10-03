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
#include <unistd.h>
#include <stdbool.h>

//MACROS

//CONSTS
const char version[] = "V1.0.1";

// ENUM & STRUCT
enum exit_codes
{
  X_SUCCESS = 0,
  X_FAILARGS,
  X_FAILFILEOPEN,
  X_WRONGFILETYPE,
  X_FILERROR,
  X_NOFILE,
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

typedef struct files_t
{
  uint8_t header_saved;
  char name[128];
  FILE * filename;
} files_t;

//GLOBAL VARIABLES
e_states main_state = STATE_INITIAL;
uint8_t filenumber = 0;
char temp_filename[] = "VIPSplitter_temp.tmp";

char header[255] = "";
char readline[255] = "";
char * charptr;

unsigned long current_line = 0;

FILE * file;
FILE * temp_file;
files_t array[17];
bool singlefiles = false;

// PROTOTYPES
void closeFiles();

//FUNCTION START
void printhelp()
{
  fprintf(stdout,"Usage: VIPSplitter [-options] filename.csv\r\n\r\n");
  fprintf(stdout,"[-options]: \r\n");
  fprintf(stdout,"  -h			Display this help\r\n");
  fprintf(stdout,"  -s			Single Files output\r\n\r\n");
  fprintf(stdout,"In single files output format, when data is being saved and a comment line is found\r\n");
  fprintf(stdout,"no new file for that address will be created, instead, the comments will be appended\r\n");
  fprintf(stdout,"to the same file. Default option is single files disabled.\r\n");

}


FILE * create_temp(FILE * temp)
{
  temp = fopen(temp_filename, "w+");
  return temp;
}

int delete_temp(FILE * temp)
{
  int result = 0;
  if (temp != NULL)
  {
    fclose(temp);
    result = remove(temp_filename);
  }
  return result;
}

int write_temp(char * line, FILE * temp)
{
  int result = EOF;
  if (temp != NULL)
  {
    result = fputs(line, temp);
  }

  return result;
}

int copy_temp(FILE * temp, FILE * file)
{
  int result = EOF;
  char ch;

  if ((temp != NULL) && (file != NULL))
  {
    fseek(temp, 0, SEEK_SET);			// move pointer to beginning of file
    ch = fgetc(temp); 					// Start reading file
    while (ch != EOF)
    {
      fputc(ch, file);
      ch = fgetc(temp);
    }
    result = 0;
  }

  return result;
}

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
      fprintf(stdout,"\r\nVIPMeter File Splitter is over, Thanks for using!\r\n");
      break;
    case X_FAILARGS:
      fprintf(stderr,"Usage: VIPSplitter [-options] filename.csv\r\n");
      break;
    case X_FAILFILEOPEN:
      fprintf(stderr,"File %s does not exist, exiting!\r\n", charptr);
      break;
    case X_WRONGFILETYPE:
      fprintf(stderr,"%s is not a VIPMeter generated file, exiting!\r\n", charptr);
      break;
    case X_NOFILE:
      fprintf(stderr,"No file to split, use 'VIPSplitter -h' for help!\r\n");
      break;
    case X_FILERROR:
      fprintf(stderr,"%s has errors on line %lu, exiting!\r\n", charptr, current_line);
      fprintf(stderr,"==> %s ", readline);
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
      // COMMENT - COMMENT
      // INITIAL - COMMENT
      // DATA - COMMENT
      if (main_state <= STATE_COMMENT)
      {
        main_state = STATE_COMMENT;
        result = 1;
      }
      if (main_state == STATE_DATA)
      {
        main_state = STATE_COMMENT;
        if (singlefiles)
        {
          delete_temp(temp_file);
        } else
        {
          filenumber++;
          closeFiles();
        }

        temp_file = create_temp(temp_file);
        if (temp_file == NULL)
        {
          fprintf(stderr, "Unable to create temp file\r\n");
        }
        result = 1;
      }
      break;
    case STATE_HEADER:
      // COMMENT - HEADER
      if (main_state == STATE_COMMENT)
      {
        main_state = STATE_HEADER;
        result = 1;
      }
      break;
    case STATE_DATA:
      // DATA - DATA
      // HEADER - DATA
      // If state is INITIAL, keep it as INITIAL
      if ((main_state == STATE_HEADER) || (main_state == STATE_DATA))
      {
        main_state = STATE_DATA;
        result = 1;
      }
      if (main_state == STATE_INITIAL)
      {
          main_state = STATE_INITIAL;
          result = 1;
      }
      break;
    case STATE_INITIAL:
      // INITIAL - INITIAL
      if (main_state <= STATE_COMMENT)
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
    if (temp_file != NULL)
    {
      copy_temp(temp_file, array[pos].filename);
    }

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
  delete_temp(temp_file);
}

void handleData(char * data, char * origin_filename)
{
  char name[255];
  char temp[10];
  uint8_t name_number;

  //Creating file name
  name_number = data[3] - '0';

  if (name_number > 9)
  {
    name_number = (data[3] - 'A') + 10;
    if (name_number > 15)
    {
      myexit(X_FILERROR);
    }
  }

  strcpy(name, origin_filename);

  sprintf(temp, "_0x4%c_%d.csv", data[3], filenumber);

  char * ptstr;
  ptstr = strstr(name, ".csv"); //find address of substring

  if (ptstr == NULL)
  {
    myexit(X_WRONGFILETYPE);
  }

  int result = ptstr - name;

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

  // Now create file with filename and save on it
  createFile(name_number, name);		// create file
  saveinFile(name_number, data);		// store header/data

}


int main(int argc, char * argv[]) {
  fprintf (stdout, "\r\nVIPSplitter %s, a VIPMeter file splitter\r\n\r\n", version);
  charptr = argv[0];

  char op;
  while ((op = getopt(argc, argv, "hs")) != EOF)
  {
    switch (op) {
      case 'h':
        printhelp();
        myexit(X_SUCCESS);
        break;
      case 's':
        singlefiles = true;
        break;
      default:
        myexit(X_FAILARGS);
        break;
    }

  }
  argc -= optind;
  //argv += optind;

  if (argc > 1)						// Wrong number of input arguments
  {
    myexit(X_FAILARGS);
  }

  file = fopen(argv[optind],"r");

  if (NULL == file)					// File does not exist
  {
    if (argv[optind] == NULL)
    {
      myexit(X_NOFILE);
    } else
    {
      charptr = argv[optind];
      myexit(X_FAILFILEOPEN);
    }

  }

  temp_file = create_temp(temp_file);
  if (temp_file == NULL)
  {
    fprintf(stderr,"Unable to create temp file\r\n");
  }

  charptr = argv[optind];
  main_state = STATE_INITIAL;
  current_line = 0;

  char ch;
  uint8_t index = 0;

  fprintf(stdout, "Please Wait, this may take a while. Working... \r\n");

  ch = fgetc(file); 					// Start reading file
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
        write_temp(readline, temp_file);

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
