/* Misc functions - strings etc */
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "utils.h"

/* FIXME: vasprintf is a GNU extension, so it maybe
 * has to be rewritten in platform-independent way ?
 */
char *make_string(const char *template, ...)
{
    va_list ap;
    char *buffer;

    va_start(ap, template);
    vasprintf(&buffer, template, ap);
    va_end(ap);

    return buffer;
}

void add_to_string(char **string, char *add)
{
    int length = (string && *string) ? strlen(*string) : 0;

    *string = realloc(*string, length + strlen(add) + 1);
    memcpy((char *)*string + length, add, strlen(add) + 1);
}

void add_pattern_to_string(char **string, const char *template,
			   ...)
{
    va_list ap;
    char *buffer;

    va_start(ap, template);
    vasprintf(&buffer, template, ap);
    va_end(ap);

    add_to_string(string, buffer);

    if(buffer)
	free(buffer);
}

int file_exists_and_normal(char *filename)
{
    struct stat status;
    int result = !stat(filename, &status);

    if(result && !S_ISREG(status.st_mode) && !S_ISLNK(status.st_mode))
	result = FALSE;

    if(result && access(filename, R_OK)) {
	result = FALSE;
    }

    return result;
}

char *file_read_string(FILE *file)
{
    char *string = NULL;
    int max_length = 0;
    int length = 0;

    while(!feof(file)){
        int current = fgetc(file);

        if(current == EOF || current == '\n')
            break;

        if(length == max_length){
            max_length += 1024;
            string = realloc(string, max_length + 1);
        }
        
        string[length] = current;
        string[length + 1] = '\0';
    
        length ++;
    }

    /* FIXME: memory allocated is larger than string length here */
    /* But it will be freed, so it is not critical */
    
    return string;
}
