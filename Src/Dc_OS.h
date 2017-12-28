/***********************************************************************/
/*                                                                     */
/*   Dc_OS.h : Header pour les fonctions spécifiques à l'OS.           */
/*                                                                     */
/***********************************************************************/
/*  Auteur : Olivier ZARDINI  *  Brutal Deluxe Software  *  Mar 2012   */
/***********************************************************************/

#define SET_FILE_VISIBLE 1
#define SET_FILE_HIDDEN

#define IS_WINDOWS defined(_WIN32) || defined(_WIN64)
#define IS_DARWIN defined(__APPLE__) || defined(__MACH__)
#define IS_LINUX defined(__linux__)

#if IS_WINDOWS
  #define FOLDER_CHARACTER  "\\"
#else
  #define FOLDER_CHARACTER  "/"
#endif

void my_DeleteFile(char *);

int GetFolderFiles(char *,char *);
int GetFolderFiles_Win32(char*, char*);

int my_CreateDirectory(char *);
void my_SetFileCreationModificationDate(char *,struct file_descriptive_entry *);
void my_GetFileCreationModificationDate(char *,struct prodos_file *);
void my_SetFileAttribute(char *,int);
int my_stricmp(char *,char *);
int my_strnicmp(char *,char *,size_t);
int my_mkdir(char* buf);

/***********************************************************************/
