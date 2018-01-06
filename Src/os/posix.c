#include <time.h>

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

void os_SetFileCreationModificationDate(char *path, struct file_descriptive_entry *entry) {

}

void os_GetFileCreationModificationDate(char *path, struct prodos_file *file) {
//  struct stat dirstat;
//  if (stat(path, &dirstat)) return;
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
int my_mkdir(char *path)
{
  return mkdir(path, S_IRWXU);
}

/**
 * Delegator to strcasecmp
 *
 * @brief my_stricmp
 * @param string1
 * @param string2
 * @return
 */
int my_stricmp(char *string1, char *string2)
{
  return(strcasecmp(string1,string2));
}

/**
 * Delegator to strnicmp
 *
 * @brief my_strnicmp
 * @param string1
 * @param string2
 * @param length
 * @return
 */
int my_strnicmp(char *string1, char *string2, size_t length)
{
  return(strncasecmp(string1,string2,length));
}
