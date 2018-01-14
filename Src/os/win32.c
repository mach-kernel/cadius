/**
 * Win32 C runtime calls for file manipulations
 *
 * Author: Olivier Zardini, Brutal Deluxe Software, Mar. 2012
 *
 */

#if BUILD_WIN32

#include "os.h"

/**
 * Win32 C runtime get contents of directory.
 *
 * @brief GetFolderFiles
 * @param folder_path
 * @param hierarchy
 * @return
 */
int os_GetFolderFiles(char *folder_path, char *hierarchy)
{
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
          error = os_GetFolderFiles(buffer_file_path,hierarchy);
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
}

/**
 * Win32 C runtime get file modification date
 *
 * @brief my_GetFileCreationModificationDate
 * @param file_data_path
 * @param current_file
 */
void os_GetFileCreationModificationDate(char *file_data_path, struct prodos_file *current_file)
{
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
}

/**
 * Win32 C runtime set file creation / modification date
 *
 * @brief os_SetFileCreationModificationDate
 * @param file_data_path
 * @param current_entry
 */
void os_SetFileCreationModificationDate(char *file_data_path, struct file_descriptive_entry *current_entry)
{
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
}

/**
 * Win32 C runtime case insensitive string comparison
 * @brief mystricmp
 * @param string1
 * @param string2
 * @return
 */
int my_stricmp(char *string1, char *string2)
{
  return(stricmp(string1, string2));
}

/**
 * Win32 C runtime case insensitive bounded string compare
 *
 * @brief my_strnicmp
 * @param string1
 * @param string2
 * @param length
 * @return
 */
int my_strnicmp(char *string1, char *string2, size_t length)
{
  return strnicmp(string1, string2, length);
}

char *my_strcpy(char *s1, char *s2)
{
	return strcpy_s(s1, strlen(s2), s2);
}

char *my_strdup(char *s)
{
	return _strdup(s);
}

int my_mkdir(char *path)
{
	return mkdir(path);
}

#endif
