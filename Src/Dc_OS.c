/***********************************************************************/
/*                                                                     */
/*   Dc_OS.c : Module pour les fonctions spécifiques à l'OS.           */
/*                                                                     */
/***********************************************************************/
/*  Auteur : Olivier ZARDINI  *  Brutal Deluxe Software  *  Mar 2012   */
/***********************************************************************/

#include <stdio.h>
#include <stdlib.h>

#if IS_WINDOWS
  #include <malloc.h>
  #include <io.h>
  #include <direct.h>
  #include <windows.h>
#else
  #include <dirent.h>

  #if IS_LINUX
  #include <strings.h>
  #endif
#endif

#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/timeb.h>
#include <errno.h>

#include "Dc_Shared.h"
#include "Dc_Prodos.h"
#include "Dc_Memory.h"
#include "Dc_OS.h"

#if !defined(S_ISDIR) && defined(S_IFDIR)
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#endif

static int MakeAllDir(char *);

/******************************************************/
/*  my_DeleteFile() :  Supprime un fichier du disque. */
/******************************************************/
void os_DeleteFile(char *file_path)
{
  /* Rend le fichier visible */
  os_SetFileAttribute(file_path,SET_FILE_VISIBLE);

  /* Supprime le fichier */
  unlink(file_path);
}

/**
 * Appears to be invoked via char **BuildFileList(), which is then
 * used for things like the CLI set high bit or indent actions.
 *
 * Heap allocs the path and then inserts it into a singleton store
 * via my_Memory.
 *
 * @brief GetFolderFiles Query OS and load path structure into store
 * @param folder_path
 * @param hierarchy
 * @return
 */
int os_GetFolderFiles(char *folder_path, char *hierarchy)
{
  #if IS_WINDOWS
  return GetFolderFiles_Win32(folder_path, hierarchy);
  #endif

  int error = 0;
  if (folder_path == NULL || strlen(folder_path) == 0) return(0);

  DIR *dirstream = opendir(folder_path);
  if (dirstream == NULL) return(1);

  while(dirstream != NULL) {
    struct dirent *entry = readdir(dirstream);
    if (entry == NULL) break;

    // +1 for \0
    char *heap_path = calloc(1, strlen(folder_path) + 1);
    strcpy(heap_path, folder_path);

    // If there's no trailing dir slash, we append it, and a glob
    // (but no longer check for the Win-style terminator)
    char last_char = heap_path[strlen(heap_path) - 1];
    if (last_char != '/') strcat(heap_path, FOLDER_CHARACTER);

    // Most POSIX filename limits are 255 bytes.
    char *heap_path_filename = calloc(1, strlen(folder_path) + 256);
    strcpy(heap_path_filename, heap_path);

    // Append the filename to the path we copied earlier
    strcat(heap_path_filename, entry->d_name);

    // POSIX only guarantees that you'll have d_name,
    // so we're going to use the S_ISREG macro which is more reliable
    struct stat dirent_stat;
    int staterr = stat(heap_path_filename, &dirent_stat);
    if (staterr) return(staterr);

    if (S_ISREG(dirent_stat.st_mode)) {
      if (MatchHierarchie(heap_path_filename, hierarchy)){
        my_Memory(MEMORY_ADD_FILE, heap_path_filename, NULL);
      }
    }
    else if (S_ISDIR(dirent_stat.st_mode)) {
      if (!my_stricmp(entry->d_name, ".") || !my_stricmp(entry->d_name, "..")) {
        continue;
      }

      error = os_GetFolderFiles(heap_path_filename, hierarchy);
      if (error) break;
    }
  }

  return error;
}

/********************************************************************/
/*  GetFolderFiles() :  Récupère tous les fichiers d'un répertoire. */
/********************************************************************/
int GetFolderFiles_Win32(char *folder_path, char *hierarchy)
{
#if IS_WINDOWS
  int error, rc;
  long hFile;
  int first_time = 1;
  struct _finddata_t c_file;
  char *buffer_folder_path = NULL;
  char *buffer_file_path = NULL;

  /* Rien à faire */
  if(folder_path == NULL)
    return(0);
  if(strlen(folder_path) == 0)
    return(0);
  error = 0;

  /* Allocation mémoire */
  buffer_folder_path = (char *) calloc(1,1024);
  buffer_file_path = (char *) calloc(1,1024);
  if(buffer_folder_path == NULL || buffer_file_path == NULL)
    return(1);
  strcpy(buffer_folder_path,folder_path);
  if(buffer_folder_path[strlen(buffer_folder_path)-1] != '\\' && buffer_folder_path[strlen(buffer_folder_path)-1] != '/')
    strcat(buffer_folder_path,FOLDER_CHARACTER);
  strcat(buffer_folder_path,"*.*");

  /** On boucle sur tous les fichiers présents **/
  while(1)
    {
      if(first_time == 1)
        {
          hFile = _findfirst(buffer_folder_path,&c_file);
          rc = (int) hFile;
        }
      else
        rc = _findnext(hFile,&c_file);

        /* On analyse le résultat */
      if(rc == -1)
          break;    /* no more files */

      /** On traite cette entrée **/
      first_time++;
      strcpy(buffer_file_path,folder_path);
      if(buffer_file_path[strlen(buffer_file_path)-1] != '\\' && buffer_file_path[strlen(buffer_file_path)-1] != '/')
        strcat(buffer_file_path,FOLDER_CHARACTER);
      strcat(buffer_file_path,c_file.name);

      /** Traite le dossier de façon récursive **/
      if((c_file.attrib & _A_SUBDIR) == _A_SUBDIR)
        {
          /* On ne traite ni . ni .. */
          if(!my_stricmp(c_file.name,".") || !my_stricmp(c_file.name,".."))
            continue;

          /* Recherche dans le contenu du dossier */
          error = GetFolderFiles(buffer_file_path,hierarchy);
          if(error)
            break;
        }
      else
        {
          /* Conserve le nom du fichier */
          if(MatchHierarchie(buffer_file_path,hierarchy))
            my_Memory(MEMORY_ADD_FILE,buffer_file_path,NULL);
        }
    }

  /* On ferme */
  _findclose(hFile);

  /* Libération mémoire */
  free(buffer_folder_path);
  free(buffer_file_path);

  return(error);
#endif
}


/******************************************************/
/*  my_CreateDirectory() :  Création d'un répertoire. */
/******************************************************/
int os_CreateDirectory(char *directory)
{
  int i, error;
  struct stat sts;
  char buffer[1024];

  /* Isole le nom du répertoire */
  strcpy(buffer,directory);
  for(i=strlen(directory); i>=0; i--)
    if(buffer[i] == '\\' || buffer[i] == '/')
      {
        buffer[i+1] = '\0';
        break;
      }

  /* Vérifie s'il existe */
  error = stat(buffer,&sts);
  if(error == 0)
    if(S_ISDIR(sts.st_mode))
      return(0);

  /** Création des répertoires **/
  error = MakeAllDir(buffer);
  if(error == 0)
    return(1);
  
  /* On veut savoir si le ce repertoire existe et on verifie qu'on a un repertoire */
  if(stat(buffer,&sts))
    my_mkdir(buffer);
  else if(!S_ISDIR(sts.st_mode))
    return(1);

  return(0);
}

/******************************************************/
/*  MakeAllDir() :  Creation d'un nouveau répertoire  */
/******************************************************/
static int MakeAllDir(char *newdir)
{
  int len = (int) strlen(newdir);
  char buffer[1024];
  char *p;

  if(len <= 0)
    return(0);
  strcpy(buffer,newdir);

  if(buffer[len-1] == '/' || buffer[len-1] == '\\')
    buffer[len-1] = '\0';

  if(my_mkdir(buffer) == 0)
    return(1);

  p = buffer+1;
  while(1)
    {
      char hold;

      while(*p && *p != '\\' && *p != '/')
        p++;
      hold = *p;
      *p = 0;
      if((my_mkdir(buffer) == -1) && (errno == ENOENT))
        return(0);
      if(hold == 0)
        break;
      *p++ = hold;
    }

  return(1);
}


/**********************************************************************************************/
/* SetFileCreationModificationDate() :  Positionne les dates de Création et Modification. */
/**********************************************************************************************/
void os_SetFileCreationModificationDate(char *file_data_path, struct file_descriptive_entry *current_entry)
{
#if IS_WINDOWS
  BOOL result;
  HANDLE fd;
  SYSTEMTIME system_date;
  SYSTEMTIME system_utc;
  FILETIME creation_date;
  FILETIME modification_date;

  /* Init */
  memset(&creation_date,0,sizeof(FILETIME));
  memset(&modification_date,0,sizeof(FILETIME));

  /* Récupère un Handle sur le fichier */
  fd = CreateFile((LPCTSTR)file_data_path,GENERIC_READ|GENERIC_WRITE,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
  if(fd == INVALID_HANDLE_VALUE)
    return;

  /** Création des dates au format Windows **/
  /* Création Date */
  memset(&system_date,0,sizeof(SYSTEMTIME));
  system_date.wYear = current_entry->file_creation_date.year + ((current_entry->file_creation_date.year < 70) ? 2000 : 1900);
  system_date.wMonth = current_entry->file_creation_date.month;
  system_date.wDay = current_entry->file_creation_date.day;
  system_date.wHour = current_entry->file_creation_time.hour;
  system_date.wMinute = current_entry->file_creation_time.minute;
  TzSpecificLocalTimeToSystemTime(NULL,&system_date,&system_utc);
  result = SystemTimeToFileTime(&system_utc,&creation_date);
  /* Modification Date */
  memset(&system_date,0,sizeof(SYSTEMTIME));
  system_date.wYear = current_entry->file_modification_date.year + ((current_entry->file_modification_date.year < 70) ? 2000 : 1900);
  system_date.wMonth = current_entry->file_modification_date.month;
  system_date.wDay = current_entry->file_modification_date.day;
  system_date.wHour = current_entry->file_modification_time.hour;
  system_date.wMinute = current_entry->file_modification_time.minute;
  TzSpecificLocalTimeToSystemTime(NULL,&system_date,&system_utc);
  result = SystemTimeToFileTime(&system_utc,&modification_date);

  /** Changement des Dates du fichier **/
  result = SetFileTime(fd,&creation_date,(LPFILETIME)NULL,&modification_date);

  /* Fermeture du fichier */
  CloseHandle(fd);
#endif
}


/********************************************************************************************/
/*  my_GetFileCreationModificationDate() :  Récupère les dates de Création et Modification. */
/********************************************************************************************/
void os_GetFileCreationModificationDate(char *file_data_path, struct prodos_file *current_file)
{
#if IS_WINDOWS
  BOOL result;
  HANDLE fd;
  SYSTEMTIME system_utc;
  SYSTEMTIME system_date;
  FILETIME creation_date;
  FILETIME access_date;
  FILETIME modification_date;

  /* Init */
  memset(&creation_date,0,sizeof(FILETIME));
  memset(&modification_date,0,sizeof(FILETIME));

  /* Récupère un Handle sur le fichier */
  fd = CreateFile((LPCTSTR)file_data_path,GENERIC_READ|GENERIC_WRITE,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
  if(fd == INVALID_HANDLE_VALUE)
    return;

  /* Récupération des dates */
  result = GetFileTime(fd,&creation_date,&access_date,&modification_date);

  /* Fermeture du fichier */
  CloseHandle(fd);

  /** Création des dates au format Prodos **/
  /* Création Date */
  FileTimeToSystemTime(&creation_date,&system_utc);
  SystemTimeToTzSpecificLocalTime(NULL,&system_utc,&system_date);
  current_file->file_creation_date = BuildProdosDate(system_date.wDay,system_date.wMonth,system_date.wYear);
  current_file->file_creation_time = BuildProdosTime(system_date.wMinute,system_date.wHour);

  /* Modification Date */
  FileTimeToSystemTime(&modification_date,&system_utc);
  SystemTimeToTzSpecificLocalTime(NULL,&system_utc,&system_date);
  current_file->file_modification_date = BuildProdosDate(system_date.wDay,system_date.wMonth,system_date.wYear);
  current_file->file_modification_time = BuildProdosTime(system_date.wMinute,system_date.wHour);
#endif
}

/**
 * Only applies to the Win32 API, you could ostensibly do this
 * by invoking rename() to make it a dotfile but that is silly.
 *
 * @brief my_SetFileAttribute Win32 toggle hidden file header for path
 * @param file_path Path
 * @param flag Either SET_FILE_VISIBLE or SET_FILE_HIDDEN
 */
void os_SetFileAttribute(char *file_path, int flag)
{
  #if IS_WINDOWS
    DWORD file_attributes;

    /* Attributs du fichier */
    file_attributes = GetFileAttributes(file_path);

    /* Change la visibilité */
    if(flag == SET_FILE_VISIBLE)
      {
        /* Montre le fichier */
        if((file_attributes | FILE_ATTRIBUTE_HIDDEN) == file_attributes)
          SetFileAttributes(file_path,file_attributes - FILE_ATTRIBUTE_HIDDEN);
      }
    else if(flag == SET_FILE_HIDDEN)
      {
        /* Cache le fichier */
        if((file_attributes | FILE_ATTRIBUTE_HIDDEN) != file_attributes)
          SetFileAttributes(file_path,file_attributes | FILE_ATTRIBUTE_HIDDEN);
      }
  #endif
}

/**
 * Creates a directory. The POSIX compliant one requires a mask,
 * but the Win32 one does not.
 *
 * FWIW, the Win32 POSIX mkdir has been deprecated so we use _mkdir
 * from direct.h (https://msdn.microsoft.com/en-us/library/2fkk4dzw.aspx)
 *
 * @param char *path
 * @return
 */
int my_mkdir(char *path) {
  #if IS_WINDOWS
    return _mkdir(path);
  #else
    // For all visible flags if you wish to customize the behavior
    // http://pubs.opengroup.org/onlinepubs/7908799/xsh/sysstat.h.html
    return mkdir(path, S_IRWXU);
  #endif
}

/*********************************************************/
/*  my_stricmp() :  Comparaison de chaine sans la casse. */
/*********************************************************/
int my_stricmp(char *string1, char *string2)
{
#if IS_WINDOWS
  return(stricmp(string1,string2));
#else
  return(strcasecmp(string1,string2));
#endif
}


/**********************************************************/
/*  my_strnicmp() :  Comparaison de chaine sans la casse. */
/**********************************************************/
int my_strnicmp(char *string1, char *string2, size_t length)
{
#if IS_WINDOWS
  return(strnicmp(string1,string2,length));
#else
  return(strncasecmp(string1,string2,length));
#endif
}

/***********************************************************************/
