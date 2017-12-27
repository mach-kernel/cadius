/***********************************************************************/
/*                                                                     */
/*  Dc_Shared.h : Header pour la bibliothèque de fonctions génériques. */
/*                                                                     */
/***********************************************************************/
/*  Auteur : Olivier ZARDINI  *  Brutal Deluxe Software  *  Dec 2011   */
/***********************************************************************/

typedef unsigned char   BYTE;
typedef unsigned short  WORD;
typedef unsigned long   DWORD;

#define BUFFER_SIZE 2048

struct file_path
{
  char *path;

  struct file_path *next;
};

unsigned char *LoadTextFile(char *,int *);
unsigned char *LoadBinaryFile(char *,int *);
int Get24bitValue(unsigned char *,int);
int GetWordValue(unsigned char *,int);
int GetByteValue(unsigned char *,int);
void SetWordValue(unsigned char *,int,WORD);
void SetDWordValue(unsigned char *,int,DWORD);
void Set24bitValue(unsigned char *,int,int);
int CreateBinaryFile(char *,unsigned char *,int);
int CreateTextFile(char *,unsigned char *,int);
char **BuildFileList(char *,int *);
int MatchHierarchie(char *,char *);
void CleanHierarchie(char *);
char *mh_stristr(char *,char *);
int mh_stricmp(char *,char *);
char **BuildUniqueListFromFile(char *,int *);
int GetContainerNumber(int,int);
void mem_free_list(int,char **);
void mem_free_filepath(struct file_path *);

/***********************************************************************/
