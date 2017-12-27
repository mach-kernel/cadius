/**********************************************************************/
/*                                                                    */
/*  Dc_Prodos.h : Header pour la bibliothèque de gestion du Prodos.   */
/*                                                                    */
/**********************************************************************/
/*  Auteur : Olivier ZARDINI  *  Brutal Deluxe Software  *  Dec 2011  */
/**********************************************************************/

#define IMG_HEADER_SIZE  0x40   /* 2MG Header Size */
#define HDV_HEADER_SIZE  0x00   /* HDV Header Size */
#define  PO_HEADER_SIZE  0x00   /*  PO Header Size */

#define IMAGE_UNKNOWN       0
#define IMAGE_2MG           1   /* 2MG */
#define IMAGE_HDV           2   /* HDV */
#define IMAGE_PO            3   /*  PO */

#define BLOCK_SIZE       512    /* Taille d'un block */
#define INDEX_PER_BLOCK  256    /* Nombre d'index de block dans un block */

#define UPDATE_ADD     1
#define UPDATE_REMOVE  2

#define TYPE_ENTRY_SEEDLING  1
#define TYPE_ENTRY_SAPLING   2
#define TYPE_ENTRY_TREE      3
#define TYPE_ENTRY_EXTENDED  5   /* Data + Resource */

struct prodos_date
{
  int year;
  int month;
  int day;

  char ascii[20];
};

struct prodos_time
{
  int hour;
  int minute;

  char ascii[20];
};

#define BLOCK_TYPE_EMPTY   0    /* Par défaut */
#define BLOCK_TYPE_BOOT    1
#define BLOCK_TYPE_VOLUME  2
#define BLOCK_TYPE_BITMAP  3
#define BLOCK_TYPE_FILE    4
#define BLOCK_TYPE_FOLDER  5

struct prodos_image
{
  char *image_file_path;

  int image_format;   /* 2mg, hdv, po*/
  int image_header_size;

  int image_length;
  unsigned char *image_data;
  int nb_block;
  int nb_free_block;

  unsigned char *block_modified;     /* Tableau des blocks modifiés */

  struct volume_directory_header *volume_header;

  /* Version integer de la bitmap */
  int *block_allocation_table;  

  int *block_usage_type;       /* Type de données de chaque bloc (pour le CHECK_VOLUME) */
  void **block_usage_object;   /* Objet lié à chaque bloc (pour le CHECK_VOLUME) */

  /** Fichiers et répertoires attachés au Volume Directory **/
  int nb_file;              /* Liste des fichiers de ce répertoire */
  struct file_descriptive_entry **tab_file;

  int nb_directory;         /* Liste des répertoires de ce répertoire */
  struct file_descriptive_entry **tab_directory;

  /** Statistiques **/
  int nb_extract_file;
  int nb_extract_folder;
  int nb_extract_error;

  int nb_add_file;
  int nb_add_folder;
  int nb_add_error;
};

#define VOLUME_STORAGETYPE_OFFSET      0x04
#define VOLUME_LOWERCASE_OFFSET        0x1A
#define VOLUME_NAME_OFFSET             0x05
#define VOLUME_DATEMODIF_OFFSET        0x16
#define VOLUME_TIMEMODIF_OFFSET        0x18
#define VOLUME_ENTRYLENGTH_OFFSET      0x23
#define VOLUME_ENTRIESPERBLOCK_OFFSET  0x24
#define VOLUME_FILECOUNT_OFFSET        0x25

struct volume_directory_header
{
  int previous_block;    /* Tjs à zéro */
  int next_block;

  BYTE storage_type;
  char storage_type_ascii[50];
  char storage_type_ascii_short[10];

  int name_length;
  char volume_name[16];
  char volume_name_case[16];
  WORD lowercase;                                 /* GS/OS */

  struct prodos_date volume_creation_date;
  struct prodos_time volume_creation_time;

  struct prodos_date volume_modification_date;     /* GS/OS */
  struct prodos_time volume_modification_time;

  int version_formatted;
  int min_version;

  BYTE access;
  char access_ascii[50];

  int entry_length;
  int entries_per_block;
  int file_subdir_count;

  int bitmap_block;
  int total_blocks;

  int struct_size;
};

#define DIRECTORY_STORAGETYPE_OFFSET         0x04
#define DIRECTORY_NAME_OFFSET                0x05
#define DIRECTORY_LOWERCASE_OFFSET           0x20
#define DIRECTORY_ENTRYLENGTH_OFFSET         0x23
#define DIRECTORY_ENTRIESPERBLOCK_OFFSET     0x24
#define DIRECTORY_FILECOUNT_OFFSET           0x25
#define DIRECTORY_PARENTPOINTERBLOCK_OFFSET  0x27
#define DIRECTORY_PARENTENTRY_OFFSET         0x29

struct sub_directory_header
{
  int previous_block;
  int next_block;

  BYTE storage_type;
  char storage_type_ascii[50];
  char storage_type_ascii_short[10];
  
  int name_length;
  char subdir_name[16];
  char subdir_name_case[16];
  WORD lowercase;                                 /* GS/OS */

  struct prodos_date subdir_creation_date;
  struct prodos_time subdir_creation_time;

  int version_created;
  int min_version;

  BYTE access;
  char access_ascii[50];

  int entry_length;         /* Constante : 27 */
  int entries_per_block;    /* Constante : 0D */
  int file_count;           /* Nombre de fichiers non supprimés */
  int parent_pointer_block; /* Block (Volume Header ou Sub Dir) contenant l'entrée de ce SubDir */
  int parent_entry;         /* Indice 1->D de cette entrée dans le Dir Parent */
  int parent_entry_length;

  int struct_size;
};

#define FILE_STORAGETYPE_OFFSET    0x00
#define FILE_NAME_OFFSET           0x01
#define FILE_LOWERCASE_OFFSET      0x1C
#define FILE_HEADERPOINTER_OFFSET  0x25

struct file_descriptive_entry
{
  BYTE storage_type;
  char storage_type_ascii[50];
  char storage_type_ascii_short[10];

  int name_length;
  char file_name[16];
  char file_name_case[16];
  WORD lowercase;                                 /* GS/OS */

  char *file_path;         /* Chemin du ficher dans l'image */

  BYTE file_type;
  WORD file_aux_type;
  char file_type_ascii[50];

  int key_pointer_block;   /* Block contenant les données */
  int blocks_used;
  int eof_location;        /* Taille du fichier en Byte */

  struct prodos_date file_creation_date;
  struct prodos_time file_creation_time;

  struct prodos_date file_modification_date;
  struct prodos_time file_modification_time;

  int version_created;
  int min_version;

  BYTE access;
  char access_ascii[50];

  /* Répartition des données du fichier */
  int data_size;
  int data_block;
  int resource_size;
  int resource_block;

  int index_block;  /* Nombre de bloc utilisés pour les index */
  int nb_sparse;    /* Nombre de bloc fantome */

  int nb_used_block;      /* Nombre de blocs utilisés par le fichier (index+data+resource) */
  int *tab_used_block;    /* Tableau des numéros de bloc */

  int header_pointer_block; /* Block du SubDir décrivant cette entrée */

  int struct_size;          /* Taille de la structure ODS */
  int depth;                /* Profondeur de cette entrée dans l'arbre des fichiers : 1->N */
  int block_location;       /* Numéro du block (d'un directory) où cette entrée est décrite */
  int entry_offset;         /* Offset en byte par rapport au début du bloc de cette entrée File */
  int processed;            /* Entrée déjà traitée */

  struct file_descriptive_entry *parent_directory;  /* Entrée Parent Sub Directory */

  int nb_file;              /* Liste des fichiers de ce répertoire */
  struct file_descriptive_entry **tab_file;

  int nb_directory;         /* Liste des répertoires de ce répertoire */
  struct file_descriptive_entry **tab_directory;

  int delete_folder_depth;  /* Ce répertoire doit être supprimé (niveau de profondeur) */

  struct file_descriptive_entry *next;
};


struct prodos_file
{
  int entry_type;          /* Seedling, Sapling, Tree, Extended */
  int entry_disk_block;    /* Nombre de blocks pour stocker cette entrée (data+resource+index) */
  int nb_data_block;
  int *tab_data_block;     /* Liste des block alloués pour les data (permet une libération si pb) */
  int nb_resource_block;
  int *tab_resource_block; /* Liste des block alloués pour les resource (permet une libération si pb) */

  int data_length;
  unsigned char *data;
  int type_data;           /* Seedling, Sapling, Tree */
  int block_data;          /* Nb de blocks utilisés pour stocker les data en mémoire */
  int block_disk_data;     /* Nb de blocks utilisés sur le disk pour stocker les data */
  int empty_data;          /* Tout est à zéro */
  int index_data;          /* Nb de blocks utilisés sur le disk pour stocker les index des data */

  int has_resource;
  int resource_length;
  unsigned char *resource;
  int type_resource;       /* Seedling, Sapling, Tree */
  int block_resource;      /* Nb de blocks utilisés pour stocker les resources en mémoire */
  int block_disk_resource; /* Nb de blocks utilisés sur le disk pour stocker les resources */
  int empty_resource;      /* Tout est à zéro */
  int index_resource;      /* Nb de blocks utilisés sur le disk pour stocker les index des resources */

  unsigned char resource_finderinfo_1[18];
  unsigned char resource_finderinfo_2[18];

  unsigned char type;
  WORD aux_type;
  unsigned char version_create;
  unsigned char min_version;
  unsigned char access;

  WORD file_creation_date;
  WORD file_creation_time;
  WORD file_modification_date;
  WORD file_modification_time;

  char *file_name_case;
  char *file_name;           /* Majuscule */
  WORD name_case;

  struct file_descriptive_entry *entry;
};

struct prodos_image *LoadProdosImage(char *);
struct file_descriptive_entry *ODSReadFileDescriptiveEntry(struct prodos_image *,char *,unsigned char *);
int UpdateProdosImage(struct prodos_image *);
struct file_descriptive_entry *GetProdosFile(struct prodos_image *,char *);
struct file_descriptive_entry *GetProdosFolder(struct prodos_image *,char *,int);
int *GetEntryBlock(struct prodos_image *,int,int,int,int *,int **,int *);
int GetDataFile(struct prodos_image *,struct file_descriptive_entry *,struct prodos_file *);
void GetBlockData(struct prodos_image *,int,unsigned char *);
void SetBlockData(struct prodos_image *,int,unsigned char *);
void GetProdosDate(WORD,struct prodos_date *);
void GetProdosTime(WORD,struct prodos_time *);
WORD BuildProdosDate(int,int,int);
WORD BuildProdosTime(int,int);
WORD BuildProdosCase(char *);
int CheckProdosName(char *);
void GetCurrentDate(WORD *,WORD *);
int *AllocateImageBlock(struct prodos_image *,int);
int AllocateFolderEntry(struct prodos_image *,struct file_descriptive_entry *,WORD *, BYTE *,WORD *);
int UpdateEntryTable(int,int *,struct file_descriptive_entry ***,struct file_descriptive_entry *);
int compare_entry(const void *,const void *);
void mem_free_image(struct prodos_image *);
void mem_free_entry(struct file_descriptive_entry *);
void mem_free_file(struct prodos_file *);

/***********************************************************************/
