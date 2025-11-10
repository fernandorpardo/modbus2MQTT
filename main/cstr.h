/** ************************************************************************************************
 *	char string library header
 *
 ** ************************************************************************************************
**/
#define CSTRLIB_VERSION	"1.2.2"

// Char * str functions
int cstr_find(char* str, char const* needle, int pos, int max);
void cstr_replace (char *str, char a, char b);
char *cstr_sub (char *str, char *sub, int p0, int p1);
void cstr_dump(char * bf, int nbytes);
void cstr_fdump(FILE *fp, char * bf, int nbytes);
char* cstr_copy(char* dst, char* org , size_t dst_max_sz);

// END OF FILE