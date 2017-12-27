/***********************************************************************/
/*                                                                     */
/*   Prodos_Dump.c : Module pour la gestion de la commande CATALOG.    */
/*                                                                     */
/***********************************************************************/
/*  Auteur : Olivier ZARDINI  *  Brutal Deluxe Software  *  Dec 2011   */
/***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

#include "Dc_Shared.h"
#include "Dc_Prodos.h"
#include "Dc_Memory.h"
#include "Dc_OS.h"
#include "Prodos_Dump.h"

static void DumpVolumeFooter(struct prodos_image *,int);
static void DumpOneDirectory(struct file_descriptive_entry *,int);
static void DumpOneFile(struct file_descriptive_entry *,int);
static void DumpDirectoryEntries(struct prodos_image *,struct file_descriptive_entry *);
static void DumpOneEntry(struct prodos_image *,struct file_descriptive_entry *);

/*************************************************************/
/*  DumpProdosImage() :  Dump le contenu d'une image Prodos. */
/*************************************************************/
void DumpProdosImage(struct prodos_image *current_image, int dump_structure)
{
  int i, max_depth, nb_directory;
  struct file_descriptive_entry *current_directory;

  /** Calcule la profondeur max **/
  max_depth = 1;
  my_Memory(MEMORY_GET_DIRECTORY_NB,&nb_directory,NULL);
  for(i=1; i<=nb_directory; i++)
    {
      my_Memory(MEMORY_GET_DIRECTORY,&i,&current_directory);
      if(current_directory->depth > max_depth)
        max_depth = current_directory->depth;
    }

  /* Ligne de Label */
  printf("  Name");
  for(i=strlen("  Name"); i<max_depth*2+20; i++)
    printf(" ");
  printf("Type   Aux      Size     Data     Res  Data   Res  Sparse Index  Struct  Access    Creation Date     Modification Date\n");

  /* Volume Name */
  printf("/%s/\n",current_image->volume_header->volume_name);

  /** Volume : Files **/
  for(i=0; i<current_image->nb_file; i++)
    DumpOneFile(current_image->tab_file[i],max_depth);

  /** Volume : Sub Directory **/
  for(i=0; i<current_image->nb_directory; i++)
    DumpOneDirectory(current_image->tab_directory[i],max_depth);

  /** Volume Footer **/
  DumpVolumeFooter(current_image,max_depth);

  /*** On Dump la structure interne des entrées ***/
  if(dump_structure == 1)
    {
      /* Début */
      printf("----------------------------------------------------------------------\n");

      /** All Entries **/
      for(i=0; i<current_image->nb_file; i++)
        DumpOneEntry(current_image,current_image->tab_file[i]);

      /** All Directory (recursivity) **/
      for(i=0; i<current_image->nb_directory; i++)
        DumpDirectoryEntries(current_image,current_image->tab_directory[i]);
    }
}


/***********************************************************************/
/*  DumpOneDirectory() :  Dump les infomations d'une entrée Directory. */
/***********************************************************************/
static void DumpOneDirectory(struct file_descriptive_entry *current_file, int max_depth)
{
  int i;

  /* Profondeur */
  for(i=0; i<current_file->depth; i++)
    printf("  ");

  /* Directory Name */
  printf("/%s/\n",current_file->file_name);

  /** All File **/
  for(i=0; i<current_file->nb_file; i++)
    DumpOneFile(current_file->tab_file[i],max_depth);

  /** All Directory (recursivity) **/
  for(i=0; i<current_file->nb_directory; i++)
    DumpOneDirectory(current_file->tab_directory[i],max_depth);
}


/*************************************************************/
/*  DumpOneFile() :  Dump les infomations d'une entrée File. */
/*************************************************************/
static void DumpOneFile(struct file_descriptive_entry *current_file, int max_depth)
{
  int i;
  char buffer[8192];

  /* Init */
  buffer[0] = '\0';

  /* Profondeur */
  for(i=0; i<current_file->depth; i++)
    strcat(buffer,"  ");

  /* Nom Fichier */
  strcat(buffer,current_file->file_name_case);

  /* Max Depth */
  for(i=2*current_file->depth+strlen(current_file->file_name_case); i<max_depth*2+20; i++)
    strcat(buffer," ");

  /** Information fichier **/
  /* Type */
  sprintf(&buffer[strlen(buffer)],"%s%s  ",current_file->file_type_ascii,!my_stricmp(current_file->storage_type_ascii_short,"Fork")?"+":" ");
  /* Aux Type */
  sprintf(&buffer[strlen(buffer)],"$%04X  ",current_file->file_aux_type);
  /* Taille Data+Resource */
  sprintf(&buffer[strlen(buffer)],"%7d  ",current_file->data_size+current_file->resource_size);
  /* Taille Data */
  sprintf(&buffer[strlen(buffer)],"%7d  ",current_file->data_size);
  /* Taille Resource */
  sprintf(&buffer[strlen(buffer)],"%6d  ",current_file->resource_size);
  /* Block Data */
  sprintf(&buffer[strlen(buffer)],"%4d ",current_file->data_block);
  /* Block Resource */
  sprintf(&buffer[strlen(buffer)],"%4d   ",current_file->resource_block);
  /* Sparse Block */
  sprintf(&buffer[strlen(buffer)],"%4d  ",current_file->nb_sparse);
  /* Index Block */
  sprintf(&buffer[strlen(buffer)],"%4d     ",current_file->index_block);
  /* Encodage */
  sprintf(&buffer[strlen(buffer)],"%s   ",current_file->storage_type_ascii_short);
  /* Access */
  sprintf(&buffer[strlen(buffer)],"%4s   ",current_file->access_ascii);
  /* Creation Date */
  sprintf(&buffer[strlen(buffer)],"%s %s   ",current_file->file_creation_date.ascii,current_file->file_creation_time.ascii);
  /* Modification Date */
  sprintf(&buffer[strlen(buffer)],"%s %s  ",current_file->file_modification_date.ascii,current_file->file_modification_time.ascii);

  printf("%s\n",buffer);
}


/*************************************************************/
/*  DumpVolumeFooter() :  Dump les informations d'un Volume. */
/*************************************************************/
static void DumpVolumeFooter(struct prodos_image *current_image, int max_depth)
{
  int i, nb_directory, nb_file;
  char buffer[8192] = "";

  my_Memory(MEMORY_GET_DIRECTORY_NB,&nb_directory,NULL);
  my_Memory(MEMORY_GET_ENTRY_NB,&nb_file,NULL);

  /* Max Depth */
  for(i=0; i<max_depth*2+10; i++)
    strcat(buffer," ");

  /** Information volume **/
  sprintf(&buffer[strlen(buffer)],"Block : %d     ",current_image->nb_block);
  sprintf(&buffer[strlen(buffer)],"Free : %d     ",current_image->nb_free_block);
  sprintf(&buffer[strlen(buffer)],"File : %d     ",nb_file);
  sprintf(&buffer[strlen(buffer)],"Directory : %d",nb_directory);

  /* Affichage */
  printf("%s\n",buffer);
}


/********************************************************************************/
/*  DumpDirectoryEntries() :  Dump les infomations des entrées d'un répertoire. */
/********************************************************************************/
static void DumpDirectoryEntries(struct prodos_image *current_image, struct file_descriptive_entry *current_file)
{
  int i;

  /* Current SubDirectory entry */
  DumpOneEntry(current_image,current_file);

  /** All Entries **/
  for(i=0; i<current_file->nb_file; i++)
    DumpOneEntry(current_image,current_file->tab_file[i]);

  /** All Directory (recursivity) **/
  for(i=0; i<current_file->nb_directory; i++)
    DumpDirectoryEntries(current_image,current_file->tab_directory[i]);
}


/*********************************************************/
/*  DumpOneEntry() :  Dump les infomations d'une entrée. */
/*********************************************************/
static void DumpOneEntry(struct prodos_image *current_image, struct file_descriptive_entry *current_file)
{
  int i;
  char prefix[1024];

  /** On utilise la profondeur du fichier comme décalage **/
  for(i=0; i<2*current_file->depth; i++)
    prefix[i] = ' ';
  prefix[2*current_file->depth] = '\0';

  /* Path */
  printf("%sFile Path                  : %s\n",prefix,current_file->file_path);
  printf("%sName Length                : %d\n",prefix,current_file->name_length);
  printf("%sFile Name                  : %s\n",prefix,current_file->file_name);
  printf("%sFile Name Case             : %s\n",prefix,current_file->file_name_case);
  printf("%sLower Case                 : %04X\n",prefix,current_file->lowercase);
  printf("-----\n");

  /* Storage Type */
  printf("%sStorage Type               : %02X\n",prefix,current_file->storage_type);
  printf("%sStorage Type Ascii         : %s\n",prefix,current_file->storage_type_ascii);
  printf("%sStorage Type Ascii Short   : %s\n",prefix,current_file->storage_type_ascii_short);
  printf("-----\n");

  /* File Type */
  printf("%sFile Type                  : %02X\n",prefix,current_file->file_type);
  printf("%sFile Aux Type              : %04X\n",prefix,current_file->file_aux_type);
  printf("%sFile Type Ascii            : %s\n",prefix,current_file->file_type_ascii);
  printf("-----\n");

  /* Création / Modification Date + Version */
  printf("%sFile Creation Date         : %s\n",prefix,current_file->file_creation_date.ascii);
  printf("%sFile Creation Time         : %s\n",prefix,current_file->file_creation_time.ascii);
  printf("%sFile Modification Date     : %s\n",prefix,current_file->file_modification_date.ascii);
  printf("%sFile Modification Time     : %s\n",prefix,current_file->file_modification_time.ascii);
  printf("%sVersion Created            : %d\n",prefix,current_file->version_created);
  printf("%sMin Version                : %d\n",prefix,current_file->min_version);
  printf("-----\n");

  /* Access */
  printf("%sAccess                     : %02X\n",prefix,current_file->access);
  printf("%sAccess Ascii               : %s\n",prefix,current_file->access_ascii);
  printf("-----\n");

  /* Size */
  printf("%sBlocks Used                : %d\n",prefix,current_file->blocks_used);
  printf("%sEOF Location               : %d\n",prefix,current_file->eof_location);
  printf("%sData Block                 : %d\n",prefix,current_file->data_block);
  printf("%sData Size                  : %d\n",prefix,current_file->data_size);
  printf("%sResource Block             : %d\n",prefix,current_file->resource_block);
  printf("%sResource Size              : %d\n",prefix,current_file->resource_size);
  printf("%sSparse Block               : %d\n",prefix,current_file->nb_sparse);
  printf("%sIndex Block                : %d\n",prefix,current_file->index_block);
  printf("-----\n");

  /* Divers */
  printf("%sDepth                      : %d\n",prefix,current_file->depth);
  printf("%sStruct Size                : %d\n",prefix,current_file->struct_size);
  printf("%sEntry Offset [this block]  : %d\n",prefix,current_file->entry_offset);
  printf("%sKey Pointer Block [Data]   : %d  %04X\n",prefix,current_file->key_pointer_block,current_file->key_pointer_block);
  printf("%sHeader Pointer Block [Dir] : %d  %04X\n",prefix,current_file->header_pointer_block,current_file->header_pointer_block);
  printf("%sBlock Location [Entry]     : %d  %04X\n",prefix,current_file->block_location,current_file->block_location);
  if(current_file->parent_directory == NULL)
    printf("%sParent Directory           : /%s\n",prefix,current_image->volume_header->volume_name_case);
  else
    printf("%sParent Directory           : %s\n",prefix,current_file->parent_directory->file_path);

  /* Fin */
  printf("----------------------------------------------------------------------\n");
}

/*************************************************************************/
