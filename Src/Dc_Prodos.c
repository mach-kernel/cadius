/**********************************************************************/
/*                                                                    */
/*  Dc_Prodos.c : Module pour la bibliothèque de gestion du Prodos.   */
/*                                                                    */
/**********************************************************************/
/*  Auteur : Olivier ZARDINI  *  Brutal Deluxe Software  *  Dec 2011  */
/**********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include "Dc_Shared.h"
#include "Dc_Memory.h"
#include "Dc_OS.h"
#include "Dc_Prodos.h"

static struct volume_directory_header *ODSReadVolumeDirectoryHeader(unsigned char *);
static struct sub_directory_header *ODSReadSubDirectoryHeader(unsigned char *);
static void GetAllDirectoryFile(struct prodos_image *);
static void GetOneSubDirectoryFile(struct prodos_image *,char *,struct file_descriptive_entry *);
static void BuildStorageTypeAscii(BYTE,char *,char *);
static void BuildFileTypeAscii(BYTE,WORD,char *);
static void BuildAccessAscii(BYTE,char *);
static void BuildLowerCase(char *,WORD,char *);
static int GetFileDataResourceSize(struct prodos_image *,struct file_descriptive_entry *);
static int *BuildUsedBlockTable(int,int *,int,int *,int *);
static int *BuildDirectoryUsedBlockTable(struct prodos_image *,struct file_descriptive_entry *,int *);
static void DecodeExpandBitmapBlock(struct prodos_image *);
static unsigned char *GetEntryData(struct prodos_image *,int,int,int);
static void mem_free_subdirectory(struct sub_directory_header *);

/******************************************************/
/*  LoadProdosImage() :  Charge un fichier image 2mg. */
/******************************************************/
struct prodos_image *LoadProdosImage(char *file_path)
{
  unsigned char *data_file;
  int i, nb_block, data_length;
  struct prodos_image *current_image;
  unsigned char one_block[BLOCK_SIZE];

  /* Allocation mémoire */
  current_image = (struct prodos_image *) calloc(1,sizeof(struct prodos_image));
  if(current_image == NULL)
    {
      printf("  Error, Impossible to allocate memory to process image file.\n");
      return(NULL);
    }
  current_image->image_file_path = strdup(file_path);
  if(current_image->image_file_path == NULL)
    {
      printf("  Error, Impossible to allocate memory to process image file.\n");
      mem_free_image(current_image);
      return(NULL);
    }

  /** Type d'image **/
  current_image->image_format = IMAGE_UNKNOWN;
  for(i=strlen(current_image->image_file_path); i>=0; i--)
    if(current_image->image_file_path[i] == '.')
      {
        if(!my_stricmp(&current_image->image_file_path[i],".2MG"))
          {
            current_image->image_format = IMAGE_2MG;
            current_image->image_header_size = IMG_HEADER_SIZE;
          }
        else if(!my_stricmp(&current_image->image_file_path[i],".HDV"))
          {
            current_image->image_format = IMAGE_HDV;
            current_image->image_header_size = HDV_HEADER_SIZE;
          }
        else if(!my_stricmp(&current_image->image_file_path[i],".PO"))
          {
            current_image->image_format = IMAGE_PO;
            current_image->image_header_size = PO_HEADER_SIZE;
          }
        break;
      }
  if(current_image->image_format == IMAGE_UNKNOWN)
    {
      printf("  Error, Unknown image file format : '%s'.\n",current_image->image_file_path);
      mem_free_image(current_image);
      return(NULL);
    }

  /** Chargement du fichier image en mémoire **/
  data_file = LoadBinaryFile(file_path,&data_length);
  if(data_file == NULL)
    {
      printf("  Error, Impossible to load Image file : '%s'\n",file_path);
      mem_free_image(current_image);
      return(NULL);
    }

  /* Saut au dessus du header de l'image */
  data_file += current_image->image_header_size;
  data_length -= current_image->image_header_size;

  /* On élimine les derniers octets parasites */
  nb_block = data_length / BLOCK_SIZE;
  data_length = nb_block*BLOCK_SIZE;

  /** Remplit la structure **/
  current_image->nb_block = nb_block;
  current_image->image_data = data_file;
  current_image->image_length = data_length;
  current_image->block_allocation_table = (int *) calloc(current_image->nb_block+8,sizeof(int));
  if(current_image->block_allocation_table == NULL)
    {
      printf("  Error, Impossible to allocate memory to process image file.\n");
      mem_free_image(current_image);
      return(NULL);
    }

  /* Tableau des blocks modifiés */
  current_image->block_modified = (unsigned char *) calloc(current_image->nb_block,sizeof(unsigned char));
  if(current_image->block_modified == NULL)
    {
      printf("  Error, Impossible to allocate memory to process image file.\n");
      mem_free_image(current_image);
      return(NULL);
    }

  /** Utilisation des blocs **/
  current_image->block_usage_type = (int *) calloc(current_image->nb_block,sizeof(int));
  if(current_image->block_usage_type == NULL)
    {
      printf("  Error, Impossible to allocate memory to process image file.\n");
      mem_free_image(current_image);
      return(NULL);
    }
  current_image->block_usage_object = (void **) calloc(current_image->nb_block,sizeof(void *));
  if(current_image->block_usage_object == NULL)
    {
      printf("  Error, Impossible to allocate memory to process image file.\n");
      mem_free_image(current_image);
      return(NULL);
    }

  /** Décodage du Volume Header (Block 2) **/
  GetBlockData(current_image,2,one_block);
  current_image->volume_header = ODSReadVolumeDirectoryHeader(one_block);
  if(current_image->volume_header == NULL)
    {
      mem_free_image(current_image);
      return(NULL);
    }

  /**************************************************************/
  /** Décodage des entrées du Volume Directory + Sub Directory **/
  GetAllDirectoryFile(current_image);

  /** Décodage du Block Allocation Table **/
  DecodeExpandBitmapBlock(current_image);

  /* Renvoi */
  return(current_image);
}


/************************************************************/
/*  UpdateProdosImage() :  Enregistre un fichier image 2mg. */
/************************************************************/
int UpdateProdosImage(struct prodos_image *current_image)
{
  int i, nb_write;
  FILE *fd;

  /* Ouverture du fichier en écriture */
  fd = fopen(current_image->image_file_path,"r+b");
  if(fd == NULL)
    {
      printf("  Error : Impossible to open Prodos image '%s' for writing.\n",current_image->image_file_path);
      return(1);
    }

  /** On va re-écrire tous les blocks mis à jour **/
  for(i=0; i<current_image->nb_block; i++)
    if(current_image->block_modified[i] == 1)
      {
        /* Se Positionne */
        fseek(fd,(long)(i*BLOCK_SIZE+current_image->image_header_size),SEEK_SET);

        /* Ecrit le block */
        nb_write = fwrite(&current_image->image_data[i*BLOCK_SIZE],1,BLOCK_SIZE,fd);

        /* Indique que c'est fait */
        current_image->block_modified[i] = 0;
      }

  /* On se place à la fin */
  fseek(fd,0L,SEEK_END);

  /* Fermeture du fichier */
  fclose(fd);

  /* OK */
  return(0);
}


/****************************************************************************************/
/*  ODSReadVolumeDirectoryHeader() :  Décodage d'une structure volume_directory_header. */
/****************************************************************************************/
static struct volume_directory_header *ODSReadVolumeDirectoryHeader(unsigned char *block_data)
{
  int offset;
  WORD date_word, time_word;
  struct volume_directory_header *volume_header;

  /* Allocation mémoire */
  volume_header = (struct volume_directory_header *) calloc(1,sizeof(struct volume_directory_header));
  if(volume_header == NULL)
    {
      printf("  Error, Impossible to allocate memory to process volume directory header.\n");
      return(NULL);
    }
  offset = 0;

  /** Décodage de la structure **/
  volume_header->previous_block = GetWordValue(block_data,offset);
  offset += 2;
  volume_header->next_block = GetWordValue(block_data,offset);
  offset += 2;
  volume_header->storage_type = ((block_data[offset] & 0xF0) >> 4);
  volume_header->name_length = (int) (block_data[offset] & 0x0F);
  offset++;
  memcpy(volume_header->volume_name,&block_data[offset],volume_header->name_length);
  volume_header->volume_name[volume_header->name_length] = '\0';
  offset += 15;
  offset += 2;    /* Reserved */
  date_word = GetWordValue(block_data,offset);
  GetProdosDate(date_word,&volume_header->volume_modification_date);    /* GS/OS */
  offset += 2;
  time_word = GetWordValue(block_data,offset);
  GetProdosTime(time_word,&volume_header->volume_modification_time);
  offset += 2;
  volume_header->lowercase = GetWordValue(block_data,offset);       /* GS/OS */
  offset += 2;
  date_word = GetWordValue(block_data,offset);
  GetProdosDate(date_word,&volume_header->volume_creation_date);
  offset += 2;
  time_word = GetWordValue(block_data,offset);
  GetProdosTime(time_word,&volume_header->volume_creation_time);
  offset += 2;
  volume_header->version_formatted = GetByteValue(block_data,offset);
  offset++;
  volume_header->min_version = GetByteValue(block_data,offset);
  offset++;
  volume_header->access = block_data[offset];
  offset++;
  volume_header->entry_length = GetByteValue(block_data,offset);
  offset++;
  volume_header->entries_per_block = GetByteValue(block_data,offset);
  offset++;
  volume_header->file_subdir_count = GetWordValue(block_data,offset);
  offset += 2;
  volume_header->bitmap_block = GetWordValue(block_data,offset);
  offset += 2;
  volume_header->total_blocks = GetWordValue(block_data,offset);
  offset += 2;

  /* Taille de la structure ODS */
  volume_header->struct_size = offset;

  /* Valeurs Ascii */
  BuildStorageTypeAscii(volume_header->storage_type,volume_header->storage_type_ascii,volume_header->storage_type_ascii_short);
  BuildAccessAscii(volume_header->access,volume_header->access_ascii);

  /* Nom LowerCase */
  BuildLowerCase(volume_header->volume_name,volume_header->lowercase,volume_header->volume_name_case);

  /* Renvoi la structure */
  return(volume_header);
}


/**********************************************************************************/
/*  ODSReadSubDirectoryHeader() :  Décodage d'une structure sub_directory_header. */
/**********************************************************************************/
static struct sub_directory_header *ODSReadSubDirectoryHeader(unsigned char *block_data)
{
  int offset;
  WORD date_word, time_word;
  struct sub_directory_header *directory_header;

  /* Allocation mémoire */
  directory_header = (struct sub_directory_header *) calloc(1,sizeof(struct sub_directory_header));
  if(directory_header == NULL)
    {
      printf("  Error, Impossible to allocate memory to process sub directory header.\n");
      return(NULL);
    }
  offset = 0;

  /** Décodage de la structure **/
  directory_header->previous_block = GetWordValue(block_data,offset);
  offset += 2;
  directory_header->next_block = GetWordValue(block_data,offset);
  offset += 2;
  directory_header->storage_type = ((block_data[offset] & 0xF0) >> 4);
  directory_header->name_length = (int) (block_data[offset] & 0x0F);
  offset++;
  memcpy(directory_header->subdir_name,&block_data[offset],directory_header->name_length);
  directory_header->subdir_name[directory_header->name_length] = '\0';
  offset += 15;
  offset++;
  offset += 7;
  date_word = GetWordValue(block_data,offset);
  GetProdosDate(date_word,&directory_header->subdir_creation_date);
  offset += 2;
  time_word = GetWordValue(block_data,offset);
  GetProdosTime(time_word,&directory_header->subdir_creation_time);
  offset += 2;
  directory_header->lowercase = GetWordValue(block_data,offset);         /* GS/OS */
  directory_header->version_created = GetByteValue(block_data,offset);
  offset++;
  directory_header->min_version = GetByteValue(block_data,offset);
  offset++;
  directory_header->access = block_data[offset];
  offset++;
  directory_header->entry_length = GetByteValue(block_data,offset);
  offset++;
  directory_header->entries_per_block = GetByteValue(block_data,offset);
  offset++;
  directory_header->file_count = GetWordValue(block_data,offset);
  offset += 2;
  directory_header->parent_pointer_block = GetWordValue(block_data,offset);
  offset += 2;
  directory_header->parent_entry = GetByteValue(block_data,offset);
  offset++;
  directory_header->parent_entry_length = GetByteValue(block_data,offset);
  offset++;

  /* Taille de la structure ODS */
  directory_header->struct_size = offset;

  /* Valeurs Ascii */
  BuildStorageTypeAscii(directory_header->storage_type,directory_header->storage_type_ascii,directory_header->storage_type_ascii_short);
  BuildAccessAscii(directory_header->access,directory_header->access_ascii);

  /* Nom LowerCase */
  BuildLowerCase(directory_header->subdir_name,directory_header->lowercase,directory_header->subdir_name_case);

  /* Renvoi la structure */
  return(directory_header);
}


/**************************************************************************************/
/*  ODSReadFileDescriptiveEntry() :  Décodage d'une structure file_descriptive_entry. */
/**************************************************************************************/
struct file_descriptive_entry *ODSReadFileDescriptiveEntry(struct prodos_image *current_image, char *folder_path, unsigned char *block_data)
{
  int offset, error;
  WORD date_word, time_word;
  struct file_descriptive_entry *file_entry;

  /* Allocation mémoire */
  file_entry = (struct file_descriptive_entry *) calloc(1,sizeof(struct file_descriptive_entry));
  if(file_entry == NULL)
    {
      printf("  Error : Impossible to allocate memory to process file descriptive entry.\n");
      return(NULL);
    }
  offset = 0;

  /** Décodage de la structure **/
  file_entry->storage_type = ((block_data[offset] & 0xF0) >> 4);
  file_entry->name_length = (int) (block_data[offset] & 0x0F);
  offset++;
  memcpy(file_entry->file_name,&block_data[offset],file_entry->name_length);
  file_entry->file_name[file_entry->name_length] = '\0';
  offset += 15;
  file_entry->file_type = GetByteValue(block_data,offset);
  offset++;
  file_entry->key_pointer_block = GetWordValue(block_data,offset);
  offset += 2;
  file_entry->blocks_used = GetWordValue(block_data,offset);
  offset += 2;
  file_entry->eof_location = block_data[offset] + 256*block_data[offset+1] + 65536*block_data[offset+2];
  offset += 3;
  date_word = GetWordValue(block_data,offset);
  GetProdosDate(date_word,&file_entry->file_creation_date);
  offset += 2;
  time_word = GetWordValue(block_data,offset);
  GetProdosTime(time_word,&file_entry->file_creation_time);
  offset += 2;
  file_entry->lowercase = GetWordValue(block_data,offset);             /* GS/OS : minVersion & 0x80 */
  file_entry->version_created = GetByteValue(block_data,offset);
  offset++;
  file_entry->min_version = GetByteValue(block_data,offset);
  offset++;
  file_entry->access = block_data[offset];
  offset++;
  memcpy(&file_entry->file_aux_type,&block_data[offset],sizeof(WORD));
  offset += 2;
  date_word = GetWordValue(block_data,offset);
  GetProdosDate(date_word,&file_entry->file_modification_date);
  offset += 2;
  time_word = GetWordValue(block_data,offset);
  GetProdosTime(time_word,&file_entry->file_modification_time);
  offset += 2;
  file_entry->header_pointer_block = GetWordValue(block_data,offset);
  offset += 2;

  /* Taille de la structure ODS */
  file_entry->struct_size = offset;

  /* Valeurs Ascii */
  BuildStorageTypeAscii(file_entry->storage_type,file_entry->storage_type_ascii,file_entry->storage_type_ascii_short);
  BuildFileTypeAscii(file_entry->file_type,file_entry->file_aux_type,file_entry->file_type_ascii);
  BuildAccessAscii(file_entry->access,file_entry->access_ascii);

  /* Nom LowerCase */
  BuildLowerCase(file_entry->file_name,file_entry->lowercase,file_entry->file_name_case);

  /* Chemin complet */
  file_entry->file_path = (char *) calloc(strlen(folder_path) + 1 + strlen(file_entry->file_name_case) + 1,sizeof(char));
  if(file_entry->file_path == NULL)
    {
      printf("  Error : Impossible to allocate memory for 'file_path' value.\n");
      mem_free_entry(file_entry);
      return(NULL);
    }
  sprintf(file_entry->file_path,"%s/%s",folder_path,file_entry->file_name_case);

  /** Taille des données + Liste des blocs utilisés **/
  error = GetFileDataResourceSize(current_image,file_entry);
  if(error)
    {
      mem_free_entry(file_entry);
      return(NULL);
    }

  /* Renvoi la structure */
  return(file_entry);
}


/**************************************************************************/
/*  GetAllDirectoryFile() :  Lecture des Directory + SubDirectory + File. */
/**************************************************************************/
static void GetAllDirectoryFile(struct prodos_image *current_image)
{
  int i, offset, nb_file, nb_directory, first_time, block_number;
  struct file_descriptive_entry *current_entry;
  struct file_descriptive_entry *current_directory;
  struct file_descriptive_entry *first_file;
  struct file_descriptive_entry *first_directory;
  unsigned char one_block[BLOCK_SIZE];
  char volume_path[256];

  /* Init */
  first_time = 1;
  nb_file = 0;
  nb_directory = 0;
  first_file = NULL;
  first_directory = NULL;
  sprintf(volume_path,"/%s",current_image->volume_header->volume_name_case);

  /*******************************************/  
  /**  Volume Directory (Block 2+suivants)  **/
  block_number = 2;
  GetBlockData(current_image,block_number,one_block);
  offset = current_image->volume_header->struct_size;
  while(block_number)
    {
      /* On analyse toutes les entrées de ce block */
      for(i=0; i<current_image->volume_header->entries_per_block-first_time; i++, offset += current_image->volume_header->entry_length)
        {
          /* Récupère l'entrée */
          current_entry = ODSReadFileDescriptiveEntry(current_image,volume_path,&one_block[offset]);
          if(current_entry == NULL)
            continue;
          current_entry->depth = 1;
          current_entry->parent_directory = NULL;
          /* Positionnement de cette entrée dans l'image */
          current_entry->block_location = block_number;
          current_entry->entry_offset = offset;

          /* On ne va pas enregistrer les entrées Deleted */
          if((current_entry->storage_type & 0x0F) == 0x00)
            {
              mem_free_entry(current_entry);
              continue;
            }

          /* Enregistre cette entrée */
          if((current_entry->storage_type & 0x0F) == 0x0D)
            {
              my_Memory(MEMORY_ADD_DIRECTORY,current_entry,NULL);
              if(nb_directory == 0)
                first_directory = current_entry;
              nb_directory++;
            }
          else
            {
              my_Memory(MEMORY_ADD_ENTRY,current_entry,NULL);
              if(nb_file == 0)
                first_file = current_entry;
              nb_file++;
            }
        }

      /* Bloc suivant */
      first_time = 0;
      block_number = GetWordValue(one_block,2);
      if(block_number == 0)
        break;
      GetBlockData(current_image,block_number,one_block);
      offset = 4;   /* Pointeur Prev + Pointeur Next */
    }

  /** Allocation des tableaux pointant vers les entrées **/
  current_image->nb_file = nb_file;
  if(current_image->nb_file > 0)
    {
      current_image->tab_file = (struct file_descriptive_entry **) calloc(nb_file,sizeof(struct file_descriptive_entry *));
      if(current_image->tab_file != NULL)
        for(i=0,current_entry=first_file; i<nb_file; i++,current_entry=current_entry->next)
          current_image->tab_file[i] = current_entry;
      qsort(current_image->tab_file,nb_file,sizeof(struct file_descriptive_entry *),compare_entry);
    }
  current_image->nb_directory = nb_directory;
  if(current_image->nb_directory > 0)
    {
      current_image->tab_directory = (struct file_descriptive_entry **) calloc(nb_directory,sizeof(struct file_descriptive_entry *));
      if(current_image->tab_directory != NULL)
        for(i=0,current_entry=first_directory; i<nb_directory; i++,current_entry=current_entry->next)
          current_image->tab_directory[i] = current_entry;
      qsort(current_image->tab_directory,nb_directory,sizeof(struct file_descriptive_entry *),compare_entry);
    }

  /************************************/
  /**  Tous les autres Subdirectory  **/
  my_Memory(MEMORY_GET_DIRECTORY_NB,&nb_directory,NULL);
  for(i=1; i<=nb_directory; i++)
    {
      my_Memory(MEMORY_GET_DIRECTORY,&i,&current_directory);
      if(current_directory->processed == 0)
        {
          /** Traite les entrées de ce SubDirectory **/
          GetOneSubDirectoryFile(current_image,current_directory->file_path,current_directory);
          current_directory->processed = 1;

          /* Si de nouveaux SubDir ont été ajoutés */
          my_Memory(MEMORY_GET_DIRECTORY_NB,&nb_directory,NULL);
        }
    }

  /** Tableaux de pointeurs **/
  my_Memory(MEMORY_BUILD_ENTRY_TAB,NULL,NULL);
  my_Memory(MEMORY_BUILD_DIRECTORY_TAB,NULL,NULL);
}


/************************************************************************/
/*  GetOneSubDirectoryFile() :  Récupère les entrées d'un SubDirectory. */
/************************************************************************/
static void GetOneSubDirectoryFile(struct prodos_image *current_image, char *folder_path, struct file_descriptive_entry *current_directory)
{
  int i, offset, first_time, depth, block_number, nb_file, nb_directory;
  struct sub_directory_header *directory_header;
  struct file_descriptive_entry *current_entry;
  struct file_descriptive_entry *first_file;
  struct file_descriptive_entry *first_directory;
  unsigned char one_block[BLOCK_SIZE];

  /* Init */
  first_time = 1;
  nb_file = 0;
  nb_directory = 0;
  first_file = NULL;
  first_directory = NULL;

  /** Volume Directory (Block X+suivants) **/
  depth = current_directory->depth+1;
  block_number = current_directory->key_pointer_block;
  GetBlockData(current_image,block_number,one_block);
  directory_header = ODSReadSubDirectoryHeader(one_block);
  offset = directory_header->struct_size;
  while(block_number)
    {
      /* On analyse toutes les entrées de ce block */
      for(i=0; i<directory_header->entries_per_block-first_time; i++, offset += directory_header->entry_length)
        {
          /* Récupère l'entrée */
          current_entry = ODSReadFileDescriptiveEntry(current_image,folder_path,&one_block[offset]);
          if(current_entry == NULL)
            continue;
          current_entry->depth = depth;
          current_entry->parent_directory = current_directory;
          /* Positionnement de cette entrée dans l'image */
          current_entry->block_location = block_number;
          current_entry->entry_offset = offset;

          /* On ne va pas enregistrer les entrées Deleted */
          if((current_entry->storage_type & 0x0F) == 0x00)
            {
              mem_free_entry(current_entry);
              continue;
            }

          /* Enregistre cette entrée */
          if((current_entry->storage_type & 0x0F) == 0x0D)
            {
              my_Memory(MEMORY_ADD_DIRECTORY,current_entry,NULL);
              if(nb_directory == 0)
                first_directory = current_entry;
              nb_directory++;
            }
          else
            {
              my_Memory(MEMORY_ADD_ENTRY,current_entry,NULL);
              if(nb_file == 0)
                first_file = current_entry;
              nb_file++;
            }
        }

      /* Bloc suivant */
      first_time = 0;
      block_number = GetWordValue(one_block,2);
      if(block_number == 0)
        break;
      GetBlockData(current_image,block_number,one_block);
      offset = 4;   /* Pointeur Prev + Pointeur Next */
    }

  /** Allocation des tableaux pointant vers les entrées **/
  current_directory->nb_file = nb_file;
  if(current_directory->nb_file > 0)
    {
      current_directory->tab_file = (struct file_descriptive_entry **) calloc(nb_file,sizeof(struct file_descriptive_entry *));
      if(current_directory->tab_file != NULL)
        for(i=0,current_entry=first_file; i<nb_file; i++,current_entry=current_entry->next)
          current_directory->tab_file[i] = current_entry;
      qsort(current_directory->tab_file,nb_file,sizeof(struct file_descriptive_entry *),compare_entry);
    }
  current_directory->nb_directory = nb_directory;
  if(current_directory->nb_directory > 0)
    {
      current_directory->tab_directory = (struct file_descriptive_entry **) calloc(nb_directory,sizeof(struct file_descriptive_entry *));
      if(current_directory->tab_directory != NULL)
        for(i=0,current_entry=first_directory; i<nb_directory; i++,current_entry=current_entry->next)
          current_directory->tab_directory[i] = current_entry;
      qsort(current_directory->tab_directory,nb_directory,sizeof(struct file_descriptive_entry *),compare_entry);
    }

  /* Libération du SubDirectory Header */
  mem_free_subdirectory(directory_header);
}


/******************************************************************/
/*  GetBlockData() :  Récupère les données d'un block de l'image. */
/******************************************************************/
void GetBlockData(struct prodos_image *current_image, int block_number, unsigned char *block_data_rtn)
{
  /* Vérifie les limites */
  if(block_number >= current_image->nb_block)
    {
      memset(block_data_rtn,0,BLOCK_SIZE);
      return;
    }

  /* Récupère les data */
  memcpy(block_data_rtn,&current_image->image_data[block_number*BLOCK_SIZE],BLOCK_SIZE);
}


/***************************************************************/
/*  SetBlockData() :  Ecrit les données d'un block de l'image. */
/***************************************************************/
void SetBlockData(struct prodos_image *current_image, int block_number, unsigned char *block_data)
{
  /* Vérifie les limites */
  if(block_number >= current_image->nb_block)
    return;

  /* Ecrit les data */
  memcpy(&current_image->image_data[block_number*BLOCK_SIZE],block_data,BLOCK_SIZE);

  /* Marque le block comme ayant été modifié */
  current_image->block_modified[block_number] = 1;
}


/*************************************************************/
/*  GetProdosDate() :  Décodage d'une date au format Prodos. */
/*************************************************************/
void GetProdosDate(WORD date_word, struct prodos_date *date_rtn)
{
  char *month_ascii[] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec",NULL};

  /* Valeurs numériques */
  date_rtn->year = (date_word & 0xFE00) >> 9;
  date_rtn->month = (date_word & 0x01E0) >> 5;
  date_rtn->day = (date_word & 0x001F);

  /* Valeur Ascii */
  sprintf(date_rtn->ascii,"%02d-%s-%04d",date_rtn->day,(date_rtn->month>0 && date_rtn->month<13)?month_ascii[date_rtn->month-1]:"???",date_rtn->year+((date_rtn->year<70)?2000:1900));
}


/**************************************************************/
/*  GetProdosTime() :  Décodage d'une heure au format Prodos. */
/**************************************************************/
void GetProdosTime(WORD time_word, struct prodos_time *time_rtn)
{
  /* Valeurs numériques */
  time_rtn->hour = (time_word & 0x1F00) >> 8;
  time_rtn->minute = (time_word & 0x003F);

  /* Valeur Ascii */
  sprintf(time_rtn->ascii,"%02d:%02d",time_rtn->hour,time_rtn->minute);
}


/**************************************************************/
/*  BuildProdosDate() :  Construit une Date au format Prodos. */
/**************************************************************/
WORD BuildProdosDate(int day, int month, int year)
{
  WORD prodos_date;
  int prodos_year;
  
  /* Création de la Date */
  prodos_year = year - 1900;
  if(prodos_year > 100)
    prodos_year -= 100;
  prodos_date = ((prodos_year << 9) & 0xFE00) | ((month << 5) & 0x01E0) | (day & 0x001F);
  
  /* Renvoie la date */
  return(prodos_date);
}


/**************************************************************/
/*  BuildProdosTime() :  Construit une Time au format Prodos. */
/**************************************************************/
WORD BuildProdosTime(int minute, int hour)
{
  WORD prodos_time;

  /* Création de l'Heure */
  prodos_time = ((hour << 8) & 0x1F00) | (minute & 0x003F);

  /* Renvoie l'heure */
  return(prodos_time);
}


/***************************************************************/
/*  BuildLowerCase() :  Construit la version LowerCase du nom. */
/***************************************************************/
static void BuildLowerCase(char *file_name, WORD lowercase, char *file_name_case_rtn)
{
  int i;

  /* Init */
  strcpy(file_name_case_rtn,file_name);
  
  /* Vérifie la présence du 0x8000 */
  if((lowercase & 0x8000) == 0x0000)
    return;

  /* On met les lettres en minuscule */
  for(i=0; i< (int) strlen(file_name); i++)
    if((lowercase << (i+1)) & 0x8000)
      file_name_case_rtn[i] = tolower(file_name_case_rtn[i]);
}


/*************************************************************/
/*  BuildProdosCase() :  Construit le code LowerCase du nom. */
/*************************************************************/
WORD BuildProdosCase(char *file_name)
{
  int i;
  WORD name_case;

  /* Init */
  name_case = 0x8000;

  /* Indique les lettres en minuscule */
  for(i=0; i<(int)strlen(file_name); i++)
   if(file_name[i] != toupper(file_name[i]))
      name_case |= (0x8000 >> (i+1));

  /* Renvoie le code */
  return(name_case);
}


/********************************************************************************/
/*  BuildStorageTypeAscii() :  Construction de la valeur Ascii du Storage Type. */
/********************************************************************************/
static void BuildStorageTypeAscii(BYTE storage_type, char *ascii_rtn, char *ascii_short_rtn)
{
  if((storage_type & 0x0F) == 0x00)
    {
      strcpy(ascii_rtn,"Deleted");
      strcpy(ascii_short_rtn,"Del ");
    }
  else if((storage_type & 0x0F) == 0x01)
    {
      strcpy(ascii_rtn,"Seedling (1 data block)");
      strcpy(ascii_short_rtn,"Seed");
    }
  else if((storage_type & 0x0F) == 0x02)
    {
      strcpy(ascii_rtn,"Sapling (2-256 data blocks)");
      strcpy(ascii_short_rtn,"Sapl");
    }
  else if((storage_type & 0x0F) == 0x03)
    {
      strcpy(ascii_rtn,"Tree (257-32768 data blocks)");
      strcpy(ascii_short_rtn,"Tree");
    }
  else if((storage_type & 0x0F) == 0x05)
    {
      strcpy(ascii_rtn,"Extended");
      strcpy(ascii_short_rtn,"Fork");
    }
  else if((storage_type & 0x0F) == 0x0D)
    {
      strcpy(ascii_rtn,"Subdirectory");
      strcpy(ascii_short_rtn,"Dir ");
    }
  else if((storage_type & 0x0F) == 0x0E)
    {
      strcpy(ascii_rtn,"Reserved for Subdirectory Header entry");
      strcpy(ascii_short_rtn,"    ");
    }
  else if((storage_type & 0x0F) == 0x0F)
    {
      strcpy(ascii_rtn,"Reserved for Volume Directory Header entry");
      strcpy(ascii_short_rtn,"    ");
    }
  else
    {
      sprintf(ascii_rtn,"Unkown value (%02X)",storage_type & 0x0F);
      sprintf(ascii_short_rtn,"?%02X?",storage_type & 0x0F);
    }
}


/**************************************************************************/
/*  BuildFileTypeAscii() :  Construction de la valeur Ascii du File Type. */
/**************************************************************************/
static void BuildFileTypeAscii(BYTE file_type, WORD file_auxtype, char *ascii_rtn)
{
  if(file_type == 0x00)
    strcpy(ascii_rtn,"UNK");
  else if(file_type == 0x01)
    strcpy(ascii_rtn,"BAD");
  else if(file_type == 0x04)
    strcpy(ascii_rtn,"TXT");
  else if(file_type == 0x06)
    strcpy(ascii_rtn,"BIN");
  else if(file_type == 0x0F)
    strcpy(ascii_rtn,"DIR");
  else if(file_type == 0x19)
    strcpy(ascii_rtn,"ADB");
  else if(file_type == 0x1A)
    strcpy(ascii_rtn,"AWP");
  else if(file_type == 0x1B)
    strcpy(ascii_rtn,"ASP");
  else if(file_type == 0x42)
    strcpy(ascii_rtn,"FTD");
  else if(file_type == 0x50)
    strcpy(ascii_rtn,"GWP");
  else if(file_type == 0x52)
    strcpy(ascii_rtn,"GDB");
  else if(file_type == 0x5A)
    strcpy(ascii_rtn,"CFG");
  else if(file_type == 0x5E)
    strcpy(ascii_rtn,"DVU");
  else if(file_type == 0xB0)
    strcpy(ascii_rtn,"SRC");
  else if(file_type == 0xB3)
    strcpy(ascii_rtn,"S16");
  else if(file_type == 0xB5)
    strcpy(ascii_rtn,"EXE");
  else if(file_type == 0xB6)
    strcpy(ascii_rtn,"PIF");
  else if(file_type == 0xB7)
    strcpy(ascii_rtn,"TIF");
  else if(file_type == 0xB8)
    strcpy(ascii_rtn,"NDA");
  else if(file_type == 0xB9)
    strcpy(ascii_rtn,"CDA");
  else if(file_type == 0xBA)
    strcpy(ascii_rtn,"TOL");
  else if(file_type == 0xBB)
    strcpy(ascii_rtn,"DVR");
  else if(file_type == 0xBC)
    strcpy(ascii_rtn,"LDF");
  else if(file_type == 0xBD)
    strcpy(ascii_rtn,"FST");
  else if(file_type == 0xBF)
    strcpy(ascii_rtn,"DOC");
  else if(file_type == 0xC0)
    strcpy(ascii_rtn,"PNT");
  else if(file_type == 0xC1)
    strcpy(ascii_rtn,"PIC");
  else if(file_type == 0xC2)
    strcpy(ascii_rtn,"ANI");
  else if(file_type == 0xC7)
    strcpy(ascii_rtn,"CDV");
  else if(file_type == 0xC8)
    strcpy(ascii_rtn,"FON");
  else if(file_type == 0xC9)
    strcpy(ascii_rtn,"FND");
  else if(file_type == 0xCA)
    strcpy(ascii_rtn,"ICN");
  else if(file_type == 0xD5)
    strcpy(ascii_rtn,"MUS");
  else if(file_type == 0xD6)
    strcpy(ascii_rtn,"INS");
  else if(file_type == 0xD8)
    strcpy(ascii_rtn,"SND");
  else if(file_type == 0xE0)
    strcpy(ascii_rtn,"LBR");
  else if(file_type == 0xEF)
    strcpy(ascii_rtn,"PAS");
  else if(file_type == 0xF0)
    strcpy(ascii_rtn,"CMD");
  else if(file_type == 0xF9)
    strcpy(ascii_rtn,"OS ");
  else if(file_type == 0xFC)
    strcpy(ascii_rtn,"BAS");
  else if(file_type == 0xFD)
    strcpy(ascii_rtn,"VAR");
  else if(file_type == 0xFE)
    strcpy(ascii_rtn,"REL");
  else if(file_type == 0xFF)
    strcpy(ascii_rtn,"SYS");
  else
    sprintf(ascii_rtn,"$%02X",file_type);
}


/*********************************************************************/
/*  BuildAccessAscii() :  Construction de la valeur Ascii de Access. */
/*********************************************************************/
static void BuildAccessAscii(BYTE access, char *ascii_rtn)
{
  strcpy(ascii_rtn,"");

  /* Read */
  strcat(ascii_rtn,(access & 0x01) == 0x01 ? "R" : " ");

  /* Write */
  strcat(ascii_rtn,(access & 0x02) == 0x02 ? "W" : " ");

  /* Changed since Last Backup */
  strcat(ascii_rtn,(access & 0x20) == 0x20 ? "B" : " ");

  /* Rename */
  strcat(ascii_rtn,(access & 0x40) == 0x40 ? "N" : " ");

  /* Destroy */
  strcat(ascii_rtn,(access & 0x80) == 0x80 ? "D" : " ");

  /* Hidden (GS/OS) */
  strcat(ascii_rtn,(access & 0x04) == 0x04 ? "H" : " ");
}


/****************************************************************************/
/*  GetFileDataResourceSize() :  Récupère la taille occupée par le fichier. */
/****************************************************************************/
static int GetFileDataResourceSize(struct prodos_image *current_image, struct file_descriptive_entry *file_entry)
{
  int i, nb_block, nb_data_block, nb_resource_block, data_block_used, resource_block_used, index_block, nb_index_block, nb_used_block_data, nb_used_block_resource;
  BYTE data_storage_type, resource_storage_type;
  WORD data_key_block, resource_key_block;
  int *tab_block;
  int *tab_index_block;
  int *tab_used_block_data;
  int *tab_used_block_resource;
  unsigned char extended_block[BLOCK_SIZE];

  if((file_entry->storage_type & 0x0F) == 0x00)
    {
      /* Fichier effacé */
      file_entry->data_block = 0;
      file_entry->data_size = 0;
      file_entry->resource_block = 0;
      file_entry->resource_size = 0;
      file_entry->nb_used_block = 0;

      /* OK */
      return(0);
    }
  else if((file_entry->storage_type & 0x0F) == 0x0D)
    {
      /** Subdirectory **/
      file_entry->data_block = 0;
      file_entry->data_size = 0;
      file_entry->resource_block = 0;
      file_entry->resource_size = 0;

      /** Table des blocs utilisés **/
      file_entry->tab_used_block = BuildDirectoryUsedBlockTable(current_image,file_entry,&file_entry->nb_used_block);
      if(file_entry->tab_used_block == NULL)
        {
          printf("  Error : Impossible to allocate memory for 'tab_used_block' table.\n");
          return(1);
        }

      /* OK */
      return(0);
    }
  else
    {
      /*** Seedling (1 data block) / Sapling (2-256 data blocks) / Tree (257-32768 data blocks) ***/
      if((file_entry->storage_type & 0x0F) == 0x01 || (file_entry->storage_type & 0x0F) == 0x02 || (file_entry->storage_type & 0x0F) == 0x03)
        {
          /** Tailles occupée **/
          file_entry->data_size = file_entry->eof_location;
          file_entry->resource_size = 0;
          file_entry->resource_block = 0;

          /* Nombre Total de Data Block nécessaire pour les données ce fichier */
          nb_data_block = GetContainerNumber(file_entry->eof_location,BLOCK_SIZE);

          /*** Recherche de fichiers Sparse dans la partie Data ***/
          if((file_entry->storage_type & 0x0F) == 0x01)
            {
              /* Pas d'index */
              file_entry->index_block = 0;
              file_entry->data_block =  file_entry->blocks_used;
              tab_block = GetEntryBlock(current_image,TYPE_ENTRY_SEEDLING,file_entry->key_pointer_block,file_entry->eof_location,&nb_block,&tab_index_block,&nb_index_block);
              if(tab_block == NULL || tab_index_block == NULL)
                {
                  if(tab_block)
                    free(tab_block);
                  if(tab_index_block)
                    free(tab_index_block);
                  return(1);
                }
            }
          else if((file_entry->storage_type & 0x0F) == 0x02)
            {
              /* 1 niveau d'index */
              file_entry->index_block = 1;
              file_entry->data_block =  file_entry->blocks_used - file_entry->index_block;
              tab_block = GetEntryBlock(current_image,TYPE_ENTRY_SAPLING,file_entry->key_pointer_block,file_entry->eof_location,&nb_block,&tab_index_block,&nb_index_block);
              if(tab_block == NULL || tab_index_block == NULL)
                {
                  if(tab_block)
                    free(tab_block);
                  if(tab_index_block)
                    free(tab_index_block);
                  return(1);
                }
            }
          else if((file_entry->storage_type & 0x0F) == 0x03)
            {
              /* 2 niveaux d'index */
              file_entry->index_block = 1 + GetContainerNumber(nb_data_block,INDEX_PER_BLOCK);
              file_entry->data_block =  file_entry->blocks_used - file_entry->index_block;
              tab_block = GetEntryBlock(current_image,TYPE_ENTRY_TREE,file_entry->key_pointer_block,file_entry->eof_location,&nb_block,&tab_index_block,&nb_index_block);
              if(tab_block == NULL || tab_index_block == NULL)
                {
                  if(tab_block)
                    free(tab_block);
                  if(tab_index_block)
                    free(tab_index_block);
                  return(1);
                }
            }

          /* On compte tous les blocs à 0 = Sparse */
          for(i=0; i<nb_block; i++)
            if(tab_block[i] == 0)
              file_entry->nb_sparse++;

          /* Table des blocs utilisés */
          file_entry->tab_used_block = BuildUsedBlockTable(nb_block,tab_block,nb_index_block,tab_index_block,&file_entry->nb_used_block);
          if(file_entry->tab_used_block == NULL)
            {
              free(tab_block);
              free(tab_index_block);
              return(1);
            }

          /* Libération mémoire */
          free(tab_block);
          free(tab_index_block);
        }
      else if((file_entry->storage_type & 0x0F) == 0x05)     /*** Extended : Data + Resource Fork ***/
        {
          /** Extended Block **/
          GetBlockData(current_image,file_entry->key_pointer_block,&extended_block[0]);

          /** Extrait les tailles **/
          file_entry->data_size = extended_block[5] + 256*extended_block[5+1] + 65536*extended_block[5+2];
          data_block_used =  GetWordValue(extended_block,3);
          file_entry->resource_size = extended_block[256+5] + 256*extended_block[256+5+1] + 65536*extended_block[256+5+2];
          resource_block_used = GetWordValue(extended_block,256+3);

          /* Nombre Total de Data+Resource Block nécessaire pour les données ce fichier */
          nb_data_block = GetContainerNumber(file_entry->data_size,BLOCK_SIZE);
          nb_resource_block = GetContainerNumber(file_entry->resource_size,BLOCK_SIZE);

          /********************************************************/
          /*** Recherche de fichiers Sparse dans la partie Data ***/
          data_storage_type = GetByteValue(extended_block,0);
          data_key_block = GetWordValue(extended_block,1);

          if((data_storage_type & 0x0F) == 0x01)
            {
              /* Pas d'index */
              index_block = 0;
              file_entry->data_block = data_block_used;
              tab_block = GetEntryBlock(current_image,TYPE_ENTRY_SEEDLING,data_key_block,file_entry->data_size,&nb_block,&tab_index_block,&nb_index_block);
              if(tab_block == NULL || tab_index_block == NULL)
                {
                  if(tab_block)
                    free(tab_block);
                  if(tab_index_block)
                    free(tab_index_block);
                  return(1);
                }
            }
          else if((data_storage_type & 0x0F) == 0x02)
            {
              /* 1 niveau d'index */
              index_block = 1;
              file_entry->data_block =  data_block_used - index_block;
              tab_block = GetEntryBlock(current_image,TYPE_ENTRY_SAPLING,data_key_block,file_entry->data_size,&nb_block,&tab_index_block,&nb_index_block);
              if(tab_block == NULL || tab_index_block == NULL)
                {
                  if(tab_block)
                    free(tab_block);
                  if(tab_index_block)
                    free(tab_index_block);
                  return(1);
                }
            }
          else if((data_storage_type & 0x0F) == 0x03)
            {
              /* 2 niveaux d'index */
              index_block = 1 + GetContainerNumber(nb_data_block,INDEX_PER_BLOCK);
              file_entry->data_block =  data_block_used - index_block;
              tab_block = GetEntryBlock(current_image,TYPE_ENTRY_TREE,data_key_block,file_entry->data_size,&nb_block,&tab_index_block,&nb_index_block);
              if(tab_block == NULL || tab_index_block == NULL)
                {
                  if(tab_block)
                    free(tab_block);
                  if(tab_index_block)
                    free(tab_index_block);
                  return(1);
                }
            }
          file_entry->index_block += index_block;

          /* Comptabilise les blocs à 0 = Sparse */
          for(i=0; i<nb_block; i++)
            if(tab_block[i] == 0)
              file_entry->nb_sparse++;

          /* Table des blocs utilisés (Data) */
          tab_used_block_data = BuildUsedBlockTable(nb_block,tab_block,nb_index_block,tab_index_block,&nb_used_block_data);
          if(tab_used_block_data == NULL)
            {
              free(tab_block);
              free(tab_index_block);
              return(1);
            }

          /* Libération mémoire */
          free(tab_block);
          free(tab_index_block);

          /************************************************************/
          /*** Recherche de fichiers Sparse dans la partie Resource ***/
          resource_storage_type = GetByteValue(extended_block,256+0);
          resource_key_block = GetWordValue(extended_block,256+1);

          if((resource_storage_type & 0x0F) == 0x01)
            {
              /* Pas d'index */
              index_block = 0;
              file_entry->resource_block =  resource_block_used;
              tab_block = GetEntryBlock(current_image,TYPE_ENTRY_SEEDLING,resource_key_block,file_entry->resource_size,&nb_block,&tab_index_block,&nb_index_block);
              if(tab_block == NULL || tab_index_block == NULL)
                {
                  if(tab_block)
                    free(tab_block);
                  if(tab_index_block)
                    free(tab_index_block);
                  return(1);
                }
            }
          else if((resource_storage_type & 0x0F) == 0x02)
            {
              /* 1 niveau d'index */
              index_block = 1;
              file_entry->resource_block =  resource_block_used - index_block;
              tab_block = GetEntryBlock(current_image,TYPE_ENTRY_SAPLING,resource_key_block,file_entry->resource_size,&nb_block,&tab_index_block,&nb_index_block);
              if(tab_block == NULL || tab_index_block == NULL)
                {
                  if(tab_block)
                    free(tab_block);
                  if(tab_index_block)
                    free(tab_index_block);
                  return(1);
                }
            }
          else if((resource_storage_type & 0x0F) == 0x03)
            {
              /* 2 niveaux d'index */
              index_block = 1 + GetContainerNumber(nb_resource_block,INDEX_PER_BLOCK);
              file_entry->resource_block =  resource_block_used - index_block;
              tab_block = GetEntryBlock(current_image,TYPE_ENTRY_TREE,resource_key_block,file_entry->resource_size,&nb_block,&tab_index_block,&nb_index_block);
              if(tab_block == NULL || tab_index_block == NULL)
                {
                  if(tab_block)
                    free(tab_block);
                  if(tab_index_block)
                    free(tab_index_block);
                  return(1);
                }
            }
          file_entry->index_block += index_block;

          /* Un des blocs est à 0 */
          for(i=0; i<nb_block; i++)
            if(tab_block[i] == 0)
              file_entry->nb_sparse++;

          /* Table des blocs utilisés (Resource) */
          tab_used_block_resource = BuildUsedBlockTable(nb_block,tab_block,nb_index_block,tab_index_block,&nb_used_block_resource);
          if(tab_used_block_resource == NULL)
            {
              free(tab_block);
              free(tab_index_block);
              free(tab_used_block_data);
              return(1);
            }

          /* Libération mémoire */
          free(tab_block);
          free(tab_index_block);

          /** Table des blocs utilisés (Data+Resource+Index) **/
          file_entry->nb_used_block = 0;
          file_entry->tab_used_block = (int *) calloc(1+nb_used_block_data+nb_used_block_resource,sizeof(int));
          if(file_entry->tab_used_block == NULL)
            {
              printf("  Error : Impossible to allocate memory for 'tab_used_block' table.\n");
              free(tab_used_block_data);
              free(tab_used_block_resource);
              return(1);
            }

          /* Extended Block */
          file_entry->tab_used_block[file_entry->nb_used_block++] = file_entry->key_pointer_block;
          /* Data Block */
          for(i=0; i<nb_used_block_data; i++)
            file_entry->tab_used_block[file_entry->nb_used_block++] = tab_used_block_data[i];
          /* Resource Block */
          for(i=0; i<nb_used_block_resource; i++)
            file_entry->tab_used_block[file_entry->nb_used_block++] = tab_used_block_resource[i];

          /* Libération mémoire */
          free(tab_used_block_data);
          free(tab_used_block_resource);
        }
    }

  /* OK */
  return(0);
}


/*************************************************************************************/
/*  BuildUsedBlockTable() :  Création de la table des blocs utilisés par un fichier. */
/*************************************************************************************/
static int *BuildUsedBlockTable(int nb_data_block, int *tab_data_block, int nb_index_block, int *tab_index_block, int *nb_used_block_rtn)
{
  int i, nb_used_block;
  int *tab_used_block;

  /* Allocation de la table */
  tab_used_block = (int *) calloc(nb_data_block+nb_index_block,sizeof(int));
  if(tab_used_block == NULL)
    {
      printf("  Error : Impossible to allocate memory for 'tab_used_block' table.\n");
      return(NULL);
    }

  /** Remplissage de la table **/
  nb_used_block = 0;
  /* Data */
  if(tab_data_block)
    {
      for(i=0; i<nb_data_block; i++)
        if(tab_data_block[i] != 0)
          tab_used_block[nb_used_block++] = tab_data_block[i];
    }
  /* Index */
  if(tab_index_block)
    {
      for(i=0; i<nb_index_block; i++)
        if(tab_index_block[i] != 0)
          tab_used_block[nb_used_block++] = tab_index_block[i];
    }

  /* OK */
  *nb_used_block_rtn = nb_used_block;
  return(tab_used_block);
}


/********************************************************************************************/
/*  BuildDirectoryUsedBlockTable() :  Construit la liste des blocs utilisés par un dossier. */
/********************************************************************************************/
static int *BuildDirectoryUsedBlockTable(struct prodos_image *current_image, struct file_descriptive_entry *file_entry, int *nb_used_block_rtn)
{
  int i, nb_used_block, block_number;
  int *tab_used_block;
  unsigned char block_data[BLOCK_SIZE];

  /* Détermine le nombre de bloc */
  nb_used_block = file_entry->blocks_used;

  /* Allocation mémoire */
  tab_used_block = (int *) calloc(nb_used_block,sizeof(int *));
  if(tab_used_block == NULL)
    {
      printf("  Error : Impossible to allocate memory for 'tab_used_block' table.\n");
      return(NULL);
    }

  /** Remplissage de la table **/
  block_number = file_entry->key_pointer_block;
  for(i=0; i<nb_used_block; i++)
    {
      /* Lecture du block */
      if(block_number == 0)
        break;
      GetBlockData(current_image,block_number,block_data);

      /* On conserve le numéro de block */
      tab_used_block[i] = block_number;

      /* Block suivant */
      block_number = GetWordValue(block_data,0x02);
    }

  /* Renvoie la table */
  *nb_used_block_rtn = i;
  return(tab_used_block);
}


/**********************************************************************/
/*  DecodeExpandBitmapBlock() :  Décode la zone 1-bitmap => 8-bitmap. */
/**********************************************************************/
static void DecodeExpandBitmapBlock(struct prodos_image *current_image)
{
  int i, j, k, l, nb_block, nb_free_block, nb_byte;
  unsigned char block_data[BLOCK_SIZE];

  /* Init */
  nb_free_block = 0;

  /* Nombre de block nécessaires pour stocker la table */
  nb_block = GetContainerNumber(current_image->nb_block,BLOCK_SIZE*8);

  /* Nombre d'octets nécessaires dans le dernier block */
  if((nb_block*BLOCK_SIZE*8) != current_image->nb_block)
    {
      nb_byte = (current_image->nb_block - ((nb_block-1)*BLOCK_SIZE*8)) / 8;
	  if(nb_byte * 8 != (current_image->nb_block - ((nb_block-1)*BLOCK_SIZE*8)))
		nb_byte++;
    }
  else
    nb_byte = BLOCK_SIZE;

  /** Remplissage **/
  for(l=0,i=0; i<nb_block; i++)
    {
      /* Lecture du block */
      GetBlockData(current_image,current_image->volume_header->bitmap_block+i,block_data);

      /* Décodage */
      for(j=0; j<((i == nb_block-1)?nb_byte:BLOCK_SIZE); j++)
	    {
          for(k=7; k>=0; k--)
		    {
              /* Décode les block libres / occupés */
              current_image->block_allocation_table[l] = (block_data[j] >> k) & 0x01;
              nb_free_block += current_image->block_allocation_table[l];
              l++;

              /* Fin des blocks ? */
			  if(l == current_image->nb_block)
                break;
		    }

          /* Fin des blocks ? */
          if(l == current_image->nb_block)
            break;
		}
    }

  /* Nombre de bloc libres */
  current_image->nb_free_block = nb_free_block;
}


/******************************************************************/
/*  compare_entry() : Fonction de comparaison pour le Quick Sort  */
/******************************************************************/
int compare_entry(const void *data_1, const void *data_2)
{
  struct file_descriptive_entry *entry_1;
  struct file_descriptive_entry *entry_2;

  /* Récupération des paramètres */
  entry_1 = *((struct file_descriptive_entry **) data_1);
  entry_2 = *((struct file_descriptive_entry **) data_2);

  /* Comparaison des noms */
  return(my_stricmp(entry_1->file_name,entry_2->file_name));
}


/***************************************************************/
/*  GetProdosFile() :  Recherche l'entrée d'un fichier Prodos. */
/***************************************************************/
struct file_descriptive_entry *GetProdosFile(struct prodos_image *current_image, char *prodos_file_path)
{
  struct file_descriptive_entry *current_entry;
  struct file_descriptive_entry *current_directory_entry;
  int i, is_volume_name, is_root_name;
  char *begin;
  char *end;
  char name[1024];

  /* Init */
  begin = prodos_file_path;
  is_volume_name = 1;
  is_root_name = 0;
  current_entry = NULL;

  /** Recherche le fichier dans l'image Prodos **/
  while(begin)
    {
      /* Isole le nom du dossier */
      end = strchr(begin,'/');
      if(end == NULL)
        strcpy(name,begin);
      else
        {
          memcpy(name,begin,end-begin);
          name[end-begin] = '\0';
        }

      /* Suivant */
      begin = (end == NULL) ? NULL : end+1;

      /* Nom vide : On continue */
      if(strlen(name) == 0)
        continue;

      /* Recherche ce nom */
      if(is_volume_name == 1)
        {
          /* Première partie : Volume Name */
          if(my_stricmp(name,current_image->volume_header->volume_name))
            {
              printf("  Error : Can't get file from Image, Wrong volume name.\n");
              return(NULL);
            }
          is_volume_name = 0;
          is_root_name = 1;
        }
      else if(is_root_name == 1)
        {
          /* Nom du Dossier à la racine */ 
          if(begin != NULL)
            {
              for(i=0; i<current_image->nb_directory; i++)
                if(!my_stricmp(current_image->tab_directory[i]->file_name,name))
                  {
                    current_entry = current_image->tab_directory[i];
                    break;
                  }
            }
          else
            {
              /* Nom du Fichier à la racine */
              for(i=0; i<current_image->nb_file; i++)
                if(!my_stricmp(current_image->tab_file[i]->file_name,name))
                  {
                    current_entry = current_image->tab_file[i];
                    break;
                  }
            }

          /* Rien trouvé : Erreur */
          if(current_entry == NULL)
            {
              printf("  Error : Can't get file from Image, File not found.\n");
              return(NULL);
            }
          is_root_name = 0;
        }
      else
        {
          /* Recherche dans un dossier */
          current_directory_entry = current_entry;
          current_entry = NULL;

          /* Nom du Dossier */
          if(begin != NULL)
            {
              for(i=0; i<current_directory_entry->nb_directory; i++)
                if(!my_stricmp(current_directory_entry->tab_directory[i]->file_name,name))
                  {
                    current_entry = current_directory_entry->tab_directory[i];
                    break;
                  }
            }
          else
            {
              /* Nom du Fichier */
              for(i=0; i<current_directory_entry->nb_file; i++)
                if(!my_stricmp(current_directory_entry->tab_file[i]->file_name,name))
                  {
                    current_entry = current_directory_entry->tab_file[i];
                    break;
                  }
            }

          /* Rien trouvé : Erreur */
          if(current_entry == NULL)
            {
              printf("  Error : Can't get file from Image.\n");
              return(NULL);
            }
        }
    }

  /* Rien trouvé */
  if(current_entry == NULL)
    {
      printf("  Error : Can't get file from Image.\n");
      return(NULL);
    }

  /* Le fichier est un répertoire */
  if((current_entry->storage_type & 0x0F) == 0x0D)
    {
      printf("  Error : Can't get file from Image : Directory name.\n");
      return(NULL);
    }

  /* Renvoi la structure */
  return(current_entry);
}


/*****************************************************************/
/*  GetProdosFolder() :  Recherche l'entrée d'un dossier Prodos. */
/*****************************************************************/
struct file_descriptive_entry *GetProdosFolder(struct prodos_image *current_image, char *prodos_folder_path, int show_error)
{
  struct file_descriptive_entry *current_entry;
  struct file_descriptive_entry *current_directory_entry;
  int i, is_volume_name, is_root_name;
  char *begin;
  char *end;
  char name[1024];

  /* Init */
  begin = prodos_folder_path;
  is_volume_name = 1;
  is_root_name = 0;
  current_entry = NULL;

  /** Recherche le dossier dans l'image Prodos **/
  while(begin)
    {
      /* Isole le nom du dossier */
      end = strchr(begin,'/');
      if(end == NULL)
        strcpy(name,begin);
      else
        {
          memcpy(name,begin,end-begin);
          name[end-begin] = '\0';
        }

      /* Suivant */
      begin = (end == NULL) ? NULL : end+1;

      /* Nom vide : On continue */
      if(strlen(name) == 0)
        continue;

      /* Recherche ce nom */
      if(is_volume_name == 1)
        {
          /* Première partie : Volume Name */
          if(my_stricmp(name,current_image->volume_header->volume_name))
            {
              if(show_error == 1)
                printf("  Error : Can't get folder from Image, Wrong volume name.\n");
              return(NULL);
            }
          is_volume_name = 0;
          is_root_name = 1;
        }
      else if(is_root_name == 1)
        {
          /* Nom du Dossier à la racine */ 
          for(i=0; i<current_image->nb_directory; i++)
            if(!my_stricmp(current_image->tab_directory[i]->file_name,name))
              {
                current_entry = current_image->tab_directory[i];
                break;
              }

          /* Rien trouvé : Erreur */
          if(current_entry == NULL)
            {
              if(show_error == 1)
                printf("  Error : Can't get folder from Image, Folder not found.\n");
              return(NULL);
            }
          is_root_name = 0;
        }
      else
        {
          /* Recherche dans un dossier */
          current_directory_entry = current_entry;
          current_entry = NULL;

          /* Nom du Dossier */
          for(i=0; i<current_directory_entry->nb_directory; i++)
            if(!my_stricmp(current_directory_entry->tab_directory[i]->file_name,name))
              {
                current_entry = current_directory_entry->tab_directory[i];
                break;
              }

          /* Rien trouvé : Erreur */
          if(current_entry == NULL)
            {
              if(show_error == 1)
                printf("  Error : Can't get folder from Image.\n");
              return(NULL);
            }
        }
    }

  /* Rien trouvé */
  if(current_entry == NULL)
    {
      if(show_error == 1)
        printf("  Error : Can't get folder from Image.\n");
      return(NULL);
    }

  /* Le fichier n'est pas un répertoire */
  if((current_entry->storage_type & 0x0F) != 0x0D)
    {
      if(show_error == 1)
        printf("  Error : Can't get folder from Image : File name.\n");
      return(NULL);
    }

  /* Renvoi la structure */
  return(current_entry);
}



/***************************************************************/
/*  GetDataFile() :  Récupère les données d'un fichier Prodos. */
/***************************************************************/
int GetDataFile(struct prodos_image *current_image, struct file_descriptive_entry *current_entry, struct prodos_file *current_file)
{
  int data_storage_type, data_key_block, data_blocks_used, data_eof;
  int resource_storage_type, resource_key_block, resource_blocks_used, resource_eof;
  unsigned char extended_block[BLOCK_SIZE];

  /*** Seedling (1 data block) ***/
  if((current_entry->storage_type & 0x0F) == 0x01)
    {
      /* Récupération des Data */
      current_file->data = GetEntryData(current_image,TYPE_ENTRY_SEEDLING,current_entry->key_pointer_block,current_entry->eof_location);
      if(current_file->data == NULL)
        return(1);
      current_file->data_length = current_entry->eof_location;
      current_file->resource = NULL;
      current_file->resource_length = 0;
    }
  /*** Sapling (2-256 data blocks) ***/
  else if((current_entry->storage_type & 0x0F) == 0x02)
    {
      /* Récupération des Data */
      current_file->data = GetEntryData(current_image,TYPE_ENTRY_SAPLING,current_entry->key_pointer_block,current_entry->eof_location);
      if(current_file->data == NULL)
        return(1);
      current_file->data_length = current_entry->eof_location;
      current_file->resource = NULL;
      current_file->resource_length = 0;
    }
  /*** Tree (257-32768 data blocks) ***/
  else if((current_entry->storage_type & 0x0F) == 0x03)
    {
      /* Récupération des Data */
      current_file->data = GetEntryData(current_image,TYPE_ENTRY_TREE,current_entry->key_pointer_block,current_entry->eof_location);
      if(current_file->data == NULL)
        return(1);
      current_file->data_length = current_entry->eof_location;
      current_file->resource = NULL;
      current_file->resource_length = 0;
    }
  /*** Extended : Data + Resource Fork ***/
  else if((current_entry->storage_type & 0x0F) == 0x05)
    {
      /** Extended Block **/
      GetBlockData(current_image,current_entry->key_pointer_block,&extended_block[0]);

      /** Data Fork : Mini Directory Entry **/
      data_storage_type = GetByteValue(extended_block,0);
      data_key_block = GetWordValue(extended_block,1);
      data_blocks_used = GetWordValue(extended_block,3);
      data_eof = extended_block[5] + 256*extended_block[5+1] + 65536*extended_block[5+2];

      /** Récupération des Data **/
      if(data_eof > 0)
        {
          current_file->data_length = data_eof;
          if((data_storage_type & 0x0F) == 0x01)
            current_file->data = GetEntryData(current_image,TYPE_ENTRY_SEEDLING,data_key_block,data_eof);
          else if((data_storage_type & 0x0F) == 0x02)
            current_file->data = GetEntryData(current_image,TYPE_ENTRY_SAPLING,data_key_block,data_eof);
          else if((data_storage_type & 0x0F) == 0x03)
            current_file->data = GetEntryData(current_image,TYPE_ENTRY_TREE,data_key_block,data_eof);
          if(current_file->data == NULL)
            return(1);
        }

      /** Resource Fork : Mini Directory Entry **/
      resource_storage_type = GetByteValue(extended_block,256+0);
      resource_key_block = GetWordValue(extended_block,256+1);
      resource_blocks_used = GetWordValue(extended_block,256+3);
      resource_eof = extended_block[256+5] + 256*extended_block[256+5+1] + 65536*extended_block[256+5+2];

      /* HFS Finder information */
      memcpy(current_file->resource_finderinfo_1,&extended_block[8],18);
      memcpy(current_file->resource_finderinfo_2,&extended_block[26],18);

      /** Récupération des Resource **/
      if(resource_eof > 0)
        {
          current_file->resource_length = resource_eof;
          if((resource_storage_type & 0x0F) == 0x01)
            current_file->resource = GetEntryData(current_image,TYPE_ENTRY_SEEDLING,resource_key_block,resource_eof);
          else if((resource_storage_type & 0x0F) == 0x02)
            current_file->resource = GetEntryData(current_image,TYPE_ENTRY_SAPLING,resource_key_block,resource_eof);
          else if((resource_storage_type & 0x0F) == 0x03)
            current_file->resource = GetEntryData(current_image,TYPE_ENTRY_TREE,resource_key_block,resource_eof);
          if(current_file->resource == NULL)
            return(1);
        }
    }

  /* OK */
  return(0);
}


/*******************************************************************/
/*  GetEntryData() :  Récupère les Data d'une partie d'un fichier. */
/*******************************************************************/
static unsigned char *GetEntryData(struct prodos_image *current_image, int type_entry, int key_block, int total_data_size)
{
  int i, data_size, offset, nb_data_block, nb_index_block;
  int *tab_data_block;
  int *tab_index_block;
  unsigned char *data;
  unsigned char data_block[BLOCK_SIZE];

  /** Memory Allocation **/
  data = (unsigned char *) calloc(1,total_data_size);
  if(data == NULL)
    return(NULL);

  /* Liste des block du fichier */
  tab_data_block = GetEntryBlock(current_image,type_entry,key_block,total_data_size,&nb_data_block,&tab_index_block,&nb_index_block);
  if(tab_data_block == NULL)
    return(NULL);

  /** Récupération des données **/
  for(i=0, offset=0; i<nb_data_block; i++)
    {
      /* Récupération des données du block (vide si le block = 0) */
      if(tab_data_block[i] == 0)
        memset(&data_block[0],0,512);
      else
        GetBlockData(current_image,tab_data_block[i],&data_block[0]);

      /* Taille valide des Data de ce block (ajustement pour le dernier) */
      data_size = (i==nb_data_block-1)?total_data_size-((nb_data_block-1)*BLOCK_SIZE):BLOCK_SIZE;
      if(data_size > BLOCK_SIZE)
        data_size = BLOCK_SIZE;

      /* Place les données dans la structure */
      memcpy(&data[offset],data_block,data_size);
      offset += data_size;
    }

  /* Libération mémoire */
  free(tab_data_block);
  if(tab_index_block)
    free(tab_index_block);

  /* Renvoi les data */
  return(data);
}


/****************************************************************/
/*  GetEntryBlock() :  Renvoie la liste des blocs d'un fichier. */
/****************************************************************/
int *GetEntryBlock(struct prodos_image *current_image, int type_entry, int key_block, int total_data_size, int *nb_data_block_rtn, int **tab_index_block_rtn, int *nb_index_block_rtn)
{
  int i, j, k, nb_data, nb_index, block_number, nb_data_block, nb_index_block;
  int *tab_data_block;
  int *tab_index_block;
  unsigned char index_block[BLOCK_SIZE];
  unsigned char master_block[BLOCK_SIZE];

  /* Init */
  *nb_data_block_rtn = 0;
  *tab_index_block_rtn = NULL;
  *nb_index_block_rtn = 0;
  nb_index_block = 0;

  /* Nombre de block théorique (Taille fichier / BLOCK_SIZE) */
  nb_data_block = GetContainerNumber(total_data_size,BLOCK_SIZE);
  if(nb_data_block == 0 && key_block !=0 && type_entry == TYPE_ENTRY_SEEDLING)
    nb_data_block = 1;

  /* Allocation mémoire : Nombre de block Index */
  tab_index_block = (int *) calloc(256+1,sizeof(int));
  if(tab_index_block == NULL)
    {
      printf("  Error : Impossible to allocate memory for 'tab_index_block' table.\n");
      return(NULL);
    }

  /* Allocation mémoire : Nombre de block Data */
  tab_data_block = (int *) calloc(nb_data_block+1,sizeof(int));
  if(tab_data_block == NULL)
    {
      printf("  Error : Impossible to allocate memory for 'tab_data_block' table.\n");
      free(tab_index_block);
      return(NULL);
    }
  if(nb_data_block == 0)
    {
      *tab_index_block_rtn = tab_index_block;
      return(tab_data_block);
    }

  /*** Récupère les numero de Block valides ***/
  if(type_entry == TYPE_ENTRY_SEEDLING)
    tab_data_block[0] = key_block;
  else if(type_entry == TYPE_ENTRY_SAPLING)
    {
      /** Index Block **/
      GetBlockData(current_image,key_block,&index_block[0]);
      tab_index_block[nb_index_block++] = key_block;

      /** Extrait les numéros de Block **/
      for(i=0,k=0; i<((nb_data_block>256)?256:nb_data_block); i++)
        tab_data_block[k++] = index_block[i] + 256*index_block[BLOCK_SIZE/2+i];
    }
  else if(type_entry == TYPE_ENTRY_TREE)
    {
      /** Master Index Block **/
      GetBlockData(current_image,key_block,&master_block[0]);
      tab_index_block[nb_index_block++] = key_block;

      /* Nombre de Index Block pour ce fichier */
      nb_index = GetContainerNumber(nb_data_block,INDEX_PER_BLOCK);

      /** Récupère les numéros de Block **/
      for(j=0,k=0; j<nb_index; j++)
        {
          /** Index Block **/
          block_number = master_block[j] + 256*master_block[BLOCK_SIZE/2+j];
          if(block_number == 0)
            memset(&index_block[0],0,BLOCK_SIZE);
          else
            {
              GetBlockData(current_image,block_number,&index_block[0]);
              tab_index_block[nb_index_block++] = block_number;
            }

          /* Nombre de Data block dans cet Index Block */
          nb_data = (j == nb_index-1) ? (nb_data_block - (nb_index-1)*INDEX_PER_BLOCK) : INDEX_PER_BLOCK;

          /** Data Block de cet Index Block **/
          for(i=0; i<nb_data; i++)
            tab_data_block[k++] = index_block[i] + 256*index_block[BLOCK_SIZE/2+i];
        }
    }

  /* Renvoi les tableaux */
  *nb_data_block_rtn = nb_data_block;
  *nb_index_block_rtn = nb_index_block;
  *tab_index_block_rtn = tab_index_block;
  return(tab_data_block);
}


/****************************************************************/
/*  AllocateImageBlock() :  Alloue X block dans l'image Prodos. */
/****************************************************************/
int *AllocateImageBlock(struct prodos_image *current_image, int nb_block)
{
  int i, j, nb_free_block, first_free_block, nb_bitmap_block, modified, total_modified, offset;
  int *tab_block;
  unsigned char mask;
  unsigned char bitmap_block[BLOCK_SIZE];

  /* Init */
  first_free_block = 0;

  /* Pas assez de place ! */
  if(current_image->nb_free_block < nb_block)
    {
      printf("  Error : Impossible to allocate %d blocks. No space left on image.\n",nb_block);  
      return(NULL);
    }

  /* Allocation mémoire */
  tab_block = (int *) calloc(nb_block,sizeof(int));
  if(tab_block == NULL)
    {
      printf("  Error : Impossible to allocate memory.\n");
      return(NULL);
    }

  /** 1ère passe, on recherche les X blocs consécutifs **/
  for(i=0,nb_free_block=0; i<current_image->nb_block-nb_block; i++)
    {
      if(current_image->block_allocation_table[i] == 1)
        {
          if(nb_free_block == 0)
            first_free_block = i;
          nb_free_block++;
          if(nb_free_block == nb_block)
            break;
        }
      else
        {
          nb_free_block = 0;
          first_free_block = 0;
        }
    }

  /* On a trouvé ! */
  if(first_free_block != 0)
    {
      /* Blocs séquentiels */
      for(i=0; i<nb_block; i++)
        tab_block[i] = first_free_block + i;
    }
  else
    {
      /* On prend ce qui est disponible */
        for(i=0,j=0; i<current_image->nb_block-nb_block; i++)
          if(current_image->block_allocation_table[i] == 1)
            {
              tab_block[j++] = i;
              if(j == nb_block)
                break;
            }
    }

  /**********************************************/
  /** On modifie la Table d'allocation mémoire **/
  for(i=0; i<nb_block; i++)
    current_image->block_allocation_table[tab_block[i]] = 0;
  current_image->nb_free_block -= nb_block;

  /****************************************************/
  /** Marque les blocs occupés dans la BitMap disque **/
  nb_bitmap_block = GetContainerNumber(current_image->nb_block,BLOCK_SIZE*8);
  total_modified = 0;
  for(i=0; i<nb_bitmap_block; i++)
    {
      /* Bitmap block */
      GetBlockData(current_image,current_image->volume_header->bitmap_block+i,&bitmap_block[0]);
      modified = 0;

      /** Passe en revue les blocs utilisés **/
      for(j=0; j<nb_block; j++)
        if((tab_block[j] >= i*8*BLOCK_SIZE) && (tab_block[j] < (i+1)*8*BLOCK_SIZE))
          {
            offset = tab_block[j] - i*8*BLOCK_SIZE;
            mask = (0x01 << (7-(offset%8)));
            if((bitmap_block[offset/8] | mask) == bitmap_block[offset/8])
              {
                bitmap_block[offset/8] -= mask;    /* 0 : Occupé */
                modified = 1;
                total_modified++;
              }

            /* Pas besoin de tout parcourir */
            if(total_modified == nb_block)
              break;
          }

      /* Modifie le block Bitmap */
      if(modified)
        SetBlockData(current_image,current_image->volume_header->bitmap_block+i,&bitmap_block[0]);

      /* Pas besoin de tout parcourir */
      if(total_modified == nb_block)
        break;
    }

  /* OK */
  return(tab_block);
}


/******************************************************************************************************/
/*  AllocateFolderEntry() :  Recherche/Crée une entrée vide dans un dossier ou à la racine du volume. */
/******************************************************************************************************/
int AllocateFolderEntry(struct prodos_image *current_image, struct file_descriptive_entry *folder_entry, WORD *directory_block_number_rtn, BYTE *directory_entry_number_rtn, WORD *header_block_number_rtn)
{
  int i, j, offset, nb_entry, entry_length, block_used, eof;
  int current_block_number, previous_block_number, next_block_number, new_block_number, parent_directory_block_number;
  int *tab_block;
  unsigned char storage_type;
  unsigned char directory_block[BLOCK_SIZE];

  /*** Charge les blocs de ce Dossier ***/
  current_block_number = (folder_entry == NULL) ? 2 : folder_entry->key_pointer_block;   /* Bloc 2 pour la racine du Volume */
  *header_block_number_rtn = current_block_number;

  /* Lecture du 1er block de ce Directory ou du volume */
  GetBlockData(current_image,current_block_number,&directory_block[0]);

  /* Nombre d'entrées */
  entry_length = (int) directory_block[(folder_entry == NULL) ? VOLUME_ENTRYLENGTH_OFFSET : DIRECTORY_ENTRYLENGTH_OFFSET];       /* 0x27 */
  nb_entry = (int) directory_block[(folder_entry == NULL) ? VOLUME_ENTRIESPERBLOCK_OFFSET : DIRECTORY_ENTRIESPERBLOCK_OFFSET];   /* 0x0D */

  /** On va passer en revue tous les block de ce Directory **/
  for(i=0; current_block_number != 0; i++)
    {
      /* Récupère les données du bloc */
      if(i > 0)
        GetBlockData(current_image,current_block_number,&directory_block[0]); /* Le 1er bloc a déjà été récupéré */
      previous_block_number = GetWordValue(directory_block,0);
      next_block_number = GetWordValue(directory_block,2);

      /* Recherche une entrée effacée */
      for(j=0; j<nb_entry; j++)
        {
          /* Header */
          if(j == 0 && i == 0)
            continue;

          /* Début de l'entrée */
          offset = 4 + j*entry_length;

          /* Entrée vide ? */
          storage_type = directory_block[offset+FILE_STORAGETYPE_OFFSET];
          if(storage_type == 0x00)
            {
              *directory_block_number_rtn = (WORD) current_block_number;
              *directory_entry_number_rtn = (BYTE) j+1;
              return(0);
            }
        }

      /* Bloc suivant */
      previous_block_number = current_block_number;
      current_block_number = next_block_number;
    }

  /*** On va allouer un bloc de plus à ce Dossier ***/
  /* Si on est sur le Volume Directory, on ne peut plus allouer de bloc supplémentaire */
  if(folder_entry == NULL)
    {
      printf("  Error : Volume Directory is full.\n");
      return(1);
    }

  /** Allocation du bloc **/
  tab_block = AllocateImageBlock(current_image,1);
  if(tab_block == NULL)
    return(1);
  new_block_number = tab_block[0];
  free(tab_block);

  /** Next : Modifie le bloc précédent **/
  SetWordValue(&directory_block[0],0x02,(WORD)new_block_number);     /* current->next = new */
  SetBlockData(current_image,previous_block_number,&directory_block[0]);

  /** Previous : Attache le bloc au Dossier **/
  memset(&directory_block[0],0,BLOCK_SIZE);
  SetWordValue(&directory_block[0],0x00,(WORD)previous_block_number); /* new->previous = current */
  SetBlockData(current_image,new_block_number,&directory_block[0]);

  /** Met à jour BlockUsed dans l'entrée décrivant le Dossier **/
  parent_directory_block_number = folder_entry->block_location;
  offset = folder_entry->entry_offset;

  /* Charge le Directory Bloc contenant l'entrée décrivant ce SubDirectory */
  GetBlockData(current_image,parent_directory_block_number,&directory_block[0]);

  /* Met à jour BlockUsed (+1) */
  block_used = GetWordValue(&directory_block[0],offset+0x13) + 1;
  SetWordValue(&directory_block[0],offset+0x13,(WORD)block_used);

  /* Met à jour EOF (+BLOCK_SIZE) */
  eof = Get24bitValue(&directory_block[0],offset+0x15) + BLOCK_SIZE;
  Set24bitValue(&directory_block[0],offset+0x15,eof);

  /* Enregistre le Bloc */
  SetBlockData(current_image,parent_directory_block_number,&directory_block[0]);

  /* Ok */
  *directory_block_number_rtn = (WORD) new_block_number;
  *directory_entry_number_rtn = (BYTE) 1;
  return(0);
}


/**************************************************************/
/*  CheckProdosName() :  Vérifie si un nom Prodos est valide. */
/**************************************************************/
int CheckProdosName(char *name)
{
  int i;

  /* Nom vide */
  if(strlen(name) == 0)
    return(0);

  /* Trop long */
  if(strlen(name) > 15)
    return(0);

  /* Vérifie la plage des caractères */
  for(i=0; i<(int)strlen(name); i++)
    if(!((name[i] >= 'a' && name[i] <= 'z') || (name[i] >= 'A' && name[i] <= 'Z') || (name[i] >= '0' && name[i] <= '9') || name[i] == '.'))
      return(0);

  /* On ne commence ni par un chiffre ni par un . */
  if(name[0] == '.' || (name[0] >= '0' && name[0] <= '9'))
    return(0);

  /* OK */
  return(1);
}


/*********************************************************************/
/*  GetCurrentDate() :  Récupère la date courante sous forme Prodos. */
/*********************************************************************/
void GetCurrentDate(WORD *now_date_rtn, WORD *now_time_rtn)
{
  time_t clock;
  struct tm *p;
  WORD now_date, now_time;
  WORD year, month, day;
  WORD hour, minute;

  /* Récupère l'heure actuelle */
  time(&clock);
  p = localtime(&clock);
  year = (WORD) p->tm_year;
  if(year > 100)
    year -= 100;
  month = (WORD) p->tm_mon+1;
  day = (WORD) p->tm_mday;
  hour = (WORD) p->tm_hour;
  minute = (WORD) p->tm_min;

  /* Date Prodos */
  now_date = ((year << 9) & 0xFE00) | ((month << 5) & 0x01E0) | (day & 0x001F);

  /* Heure Prodos */
  now_time = ((hour << 8) & 0x1F00) | (minute & 0x003F);

  /* Renvoie les valeurs */
  *now_date_rtn = now_date;
  *now_time_rtn = now_time;
}


/***********************************************************/
/*  UpdateEntryTable() :  Met à jour la table des entrées. */
/***********************************************************/
int UpdateEntryTable(int action, int *nb_entry_rtn, struct file_descriptive_entry ***tab_entry_rtn, struct file_descriptive_entry *current_entry)
{
  int i, nb_entry;
  struct file_descriptive_entry **tab_entry;
  struct file_descriptive_entry **tab_new;

  /* Init */
  nb_entry = *nb_entry_rtn;
  tab_entry = *tab_entry_rtn;

  /** On ajoute une entrée de la table **/
  if(action == UPDATE_ADD)
    {
      /* Allocation mémoire de la nouvelle table */
      tab_new = (struct file_descriptive_entry **) calloc(nb_entry+1,sizeof(struct file_descriptive_entry *));
      if(tab_new == NULL)
        return(1);

      /* Met les valeurs */
      for(i=0; i<nb_entry; i++)
        tab_new[i] = tab_entry[i];
      tab_new[i] = current_entry;

      /* Tri */
      qsort(tab_new,nb_entry+1,sizeof(struct file_descriptive_entry *),compare_entry);

      /* Libération de l'ancienne table */
      free(tab_entry);

      /* Renvoi la nouvelle */
      *tab_entry_rtn = tab_new;
      *nb_entry_rtn = nb_entry+1;
      return(0);
    }
  else
    {
      /** Suppression d'une entrée de la table **/
      if(nb_entry == 0)
        return(0);
      else if(nb_entry == 1 && current_entry == tab_entry[0])
        {
          /* Une seule valeur, la 1ère */
          *nb_entry_rtn = 0;
          return(0);
        }
      else if(current_entry == tab_entry[0])
        {
          /* 1ère valeur */
          memmove(&tab_entry[0],&tab_entry[1],(nb_entry-1)*sizeof(struct file_descriptive_entry *));
          *nb_entry_rtn = nb_entry - 1;
          return(0);
        }
      else if(current_entry == tab_entry[nb_entry-1])
        {
          /* Dernière */
          *nb_entry_rtn = nb_entry - 1;
          return(0);
        }
      else
        {
          /* Au milieu */
          for(i=0; i<nb_entry; i++)
            if(tab_entry[i] == current_entry)
              {
                memmove(&tab_entry[i],&tab_entry[i+1],(nb_entry-i-1)*sizeof(struct file_descriptive_entry *));
                *nb_entry_rtn = nb_entry - 1;
                return(0);
              }
        }
    }

  /* Never Here */
  return(0);
}


/****************************************************************************************/
/*  mem_free_subdirectory() :  Libération mémoire de la structure sub_directory_header. */
/****************************************************************************************/
static void mem_free_subdirectory(struct sub_directory_header *directory_header)
{
  if(directory_header)
    {
      free(directory_header);
    }
}


/*************************************************************************/
/*  mem_free_image() :  Libération mémoire de la structure prodos_image. */
/*************************************************************************/
void mem_free_image(struct prodos_image *current_image)
{
  if(current_image)
    {
      if(current_image->image_file_path)
        free(current_image->image_file_path);

      if(current_image->block_modified)
        free(current_image->block_modified);

      if(current_image->block_allocation_table)
        free(current_image->block_allocation_table);

      if(current_image->block_usage_type)
        free(current_image->block_usage_type);

      if(current_image->block_usage_object)
        free(current_image->block_usage_object);

      free(current_image);
    }
}


/***********************************************************************************/
/*  mem_free_entry() :  Libération mémoire de la structure file_descriptive_entry. */
/***********************************************************************************/
void mem_free_entry(struct file_descriptive_entry *current_entry)
{
  if(current_entry)
    {
      if(current_entry->file_path)
        free(current_entry->file_path);

      if(current_entry->tab_file)
        free(current_entry->tab_file);

      if(current_entry->tab_directory)
        free(current_entry->tab_directory);

      if(current_entry->tab_used_block)
        free(current_entry->tab_used_block);

      free(current_entry);
    }
}


/***********************************************************************/
/*  mem_free_file() :  Libération mémoire de la structure prodos_file. */
/***********************************************************************/
void mem_free_file(struct prodos_file *current_file)
{
  if(current_file)
    {
      if(current_file->data)
        free(current_file->data);

      if(current_file->resource)
        free(current_file->resource);

      if(current_file->file_name)
        free(current_file->file_name);

      if(current_file->file_name_case)
        free(current_file->file_name_case);
        
      if(current_file->tab_data_block)
        free(current_file->tab_data_block);

      if(current_file->tab_resource_block)
        free(current_file->tab_resource_block);

      free(current_file);
    }
}

/**********************************************************************/
