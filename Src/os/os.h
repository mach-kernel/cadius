

/***********************************************************************/
/*                                                                     */
/*   Dc_OS.h : Header pour les fonctions spécifiques à l'OS.           */
/*                                                                     */
/***********************************************************************/
/*  Auteur : Olivier ZARDINI  *  Brutal Deluxe Software  *  Mar 2012   */
/***********************************************************************/

#pragma once

#if defined(_WIN32) || defined(_WIN64)
#define BUILD_WINDOWS 1
#else
#define BUILD_POSIX 1
#endif

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/timeb.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#ifdef BUILD_POSIX

#define FOLDER_CHARACTER "/"

#include <dirent.h>
#include <utime.h>

#endif

#ifdef BUILD_WINDOWS

#pragma warning(disable:4996)

#define FOLDER_CHARACTER "\\"

#include <malloc.h>
#include <io.h>
#include <direct.h>
#include <windows.h>

#endif


#if !defined(S_ISDIR) && defined(S_IFDIR)
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#endif

#define SET_FILE_VISIBLE 1
#define SET_FILE_HIDDEN 2

#include "../Dc_Shared.h"
#include "../Dc_Prodos.h"
#include "../Dc_Memory.h"

int os_GetFolderFiles(char *,char *);
int os_CreateDirectory(char *directory);
void os_DeleteFile(char *file_path);
void os_SetFileCreationModificationDate(char *,struct file_descriptive_entry *);
void os_GetFileCreationModificationDate(char *,struct prodos_file *);
void os_SetFileAttribute(char *,int);
int my_stricmp(char *,char *);
int my_strnicmp(char *,char *,size_t);
int my_mkdir(char *path);

char *my_strcpy(char *s1, int s1_size, char *s2);
char *my_strdup(const char *s);

uint32_t swap32(uint32_t num);
uint16_t swap16(uint16_t num);

/***********************************************************************/
