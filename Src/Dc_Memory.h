/***********************************************************************/
/*                                                                     */
/* Dc_Memory.h : Header pour la bibliothèque de gestion de la mémoire. */
/*                                                                     */
/***********************************************************************/
/*  Auteur : Olivier ZARDINI  *  Brutal Deluxe Software  *  Dec 2011   */
/***********************************************************************/

#define MEMORY_INIT                 10
#define MEMORY_FREE                 11

#define MEMORY_ADD_ENTRY            20
#define MEMORY_GET_ENTRY_NB         21
#define MEMORY_GET_ENTRY            22
#define MEMORY_BUILD_ENTRY_TAB      23
#define MEMORY_REMOVE_ENTRY         24
#define MEMORY_FREE_ENTRY           25

#define MEMORY_ADD_DIRECTORY        30
#define MEMORY_GET_DIRECTORY_NB     31
#define MEMORY_GET_DIRECTORY        32
#define MEMORY_BUILD_DIRECTORY_TAB  33
#define MEMORY_REMOVE_DIRECTORY     34
#define MEMORY_FREE_DIRECTORY       35

#define MEMORY_ADD_FILE             40
#define MEMORY_GET_FILE_NB          41
#define MEMORY_GET_FILE             42
#define MEMORY_FREE_FILE            43

#define MEMORY_ADD_ERROR            50
#define MEMORY_GET_ERROR_NB         51
#define MEMORY_GET_ERROR            52
#define MEMORY_FREE_ERROR           53

struct parameter
{
  int action;

  char *image_file_path;
  char *prodos_file_path;
  char *prodos_folder_path;
  char *output_directory_path;

  char *new_file_name;
  char *new_folder_name;
  char *new_volume_name;

  char *new_file_path;
  char *new_folder_path;

  int new_volume_size_kb;

  char *file_path;
  char *folder_path;

  int verbose;
};

struct error
{
  char *message;

  struct error *next;
};

void my_Memory(int,void *,void *);
void mem_free_param(struct parameter *);

/***********************************************************************/