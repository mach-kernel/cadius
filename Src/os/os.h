

/***********************************************************************/
/*                                                                     */
/*   Dc_OS.h : Header pour les fonctions spécifiques à l'OS.           */
/*                                                                     */
/***********************************************************************/
/*  Auteur : Olivier ZARDINI  *  Brutal Deluxe Software  *  Mar 2012   */
/***********************************************************************/

#pragma once

#define IS_WINDOWS defined (_WIN32) || defined(_WIN64)
#define IS_DARWIN defined(__APPLE__) || defined(__MACH__)
#define IS_LINUX defined(__linux__)

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/timeb.h>
#include <sys/types.h>
#include <time.h>

#if IS_WINDOWS
#pragma warning(disable:4996)

#define FOLDER_CHARACTER "\\"

#include <malloc.h>
#include <io.h>
#include <direct.h>
#include <windows.h>

#else
#define FOLDER_CHARACTER "/"

#include <dirent.h>
#include <utime.h>

#endif

#if !defined(S_ISDIR) && defined(S_IFDIR)
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#endif

#define SET_FILE_VISIBLE 1
#define SET_FILE_HIDDEN 2

#include "../Dc_Shared.h"
#include "../Dc_Prodos.h"
#include "../Dc_Memory.h"

/**
 * Delete file at path
 * @brief os_DeleteFile
 * @param file_path
 */
static void os_DeleteFile(char *file_path)
{
  #if IS_WINDOWS
  os_SetFileAttribute(file_path,SET_FILE_VISIBLE);
  #endif
  unlink(file_path);
}

/**
 * Recursively (if necessary) creates a directory. This should work
 * on both POSIX and the classic Win32 C runtime, but will not work
 * with UWP.
 *
 * @brief os_CreateDirectory Create a directory
 * @param directory char *directory
 * @return
 */
static int os_CreateDirectory(char *directory)
{
  int error = 0;
  struct stat dirstat;

  char *dir_tokenize = strdup(directory);

  char *buffer = calloc(1, 1024);
  char *token = strtok(dir_tokenize, FOLDER_CHARACTER);

  while (token) {
    if (strlen(buffer) + strlen(token) > 1024) return(-1);
    strcat(buffer, token);

    if (stat(buffer, &dirstat) != 0)
      error = my_mkdir(buffer);
    else if (!S_ISDIR(dirstat.st_mode))
      error = my_mkdir(buffer);

    strcat(buffer, FOLDER_CHARACTER);
    token = strtok(NULL, FOLDER_CHARACTER);
  }

  return error;
}

int os_GetFolderFiles(char *,char *);
void os_SetFileCreationModificationDate(char *,struct file_descriptive_entry *);
void os_GetFileCreationModificationDate(char *,struct prodos_file *);
void os_SetFileAttribute(char *,int);
int my_stricmp(char *,char *);
int my_strnicmp(char *,char *,size_t);
int my_mkdir(char *path);

char *my_strcpy(char *s1, char *s2);
char *my_strdup(const char *s);

/***********************************************************************/
