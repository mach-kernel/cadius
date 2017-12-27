/***********************************************************************/
/*                                                                     */
/*   Dc_OS.h : Header pour les fonctions spécifiques à l'OS.           */
/*                                                                     */
/***********************************************************************/
/*  Auteur : Olivier ZARDINI  *  Brutal Deluxe Software  *  Mar 2012   */
/***********************************************************************/

#define SET_FILE_VISIBLE 1
#define SET_FILE_HIDDEN  2

#if defined(WIN32) || defined(WIN64)
#define FOLDER_CHARACTER  "\\"
#else
#define FOLDER_CHARACTER  "/"
#endif

void my_DeleteFile(char *);
int GetFolderFiles(char *,char *);
int my_CreateDirectory(char *);
void my_SetFileCreationModificationDate(char *,struct file_descriptive_entry *);
void my_GetFileCreationModificationDate(char *,struct prodos_file *);
void my_SetFileAttribute(char *,int);
int my_stricmp(char *,char *);
int my_strnicmp(char *,char *,size_t);

/***********************************************************************/
