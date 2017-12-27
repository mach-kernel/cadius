/***********************************************************************/
/*                                                                     */
/* Prodos_Check.c : Module pour la gestion de la commande CHECKVOLUME. */
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
#include "Prodos_Check.h"

static char *GetObjectInfo(int,struct file_descriptive_entry *);

/*****************************************************************/
/*  CheckProdosImage() :  Vérifie le contenu d'une image Prodos. */
/*****************************************************************/
void CheckProdosImage(struct prodos_image *current_image, int verbose)
{
  int i, j, nb_directory, nb_file, nb_error, nb_bitmap_block, first_block_number;
  struct file_descriptive_entry *current_directory;
  struct file_descriptive_entry *current_file;
  struct error *current_error;
  char *object_info;
  char current_block_info[2048];
  char first_block_info[2048];
  char error_message[2048];

  /** Blocs de Boot **/
  if(verbose)
    {
      printf("; ---------------------  Boot  ----------------------\n");
      printf("Boot;0000\n");
      printf("Boot;0001\n");
    }
  current_image->block_usage_type[0x0000] = BLOCK_TYPE_BOOT;
  current_image->block_usage_type[0x0001] = BLOCK_TYPE_BOOT;

  /** Volume directory Blocs **/
  if(verbose)
    {
      printf("; ---------------  Volume Directory  ----------------\n");
      printf("Volume;0002\n");
      printf("Volume;0003\n");
      printf("Volume;0004\n");
      printf("Volume;0005\n");
    }
  current_image->block_usage_type[0x0002] = BLOCK_TYPE_VOLUME;
  current_image->block_usage_type[0x0003] = BLOCK_TYPE_VOLUME;
  current_image->block_usage_type[0x0004] = BLOCK_TYPE_VOLUME;
  current_image->block_usage_type[0x0005] = BLOCK_TYPE_VOLUME;

  /** Bitmap Blocs **/
  if(verbose)
    printf("; --------------------  Bitmap  ---------------------\n");
  nb_bitmap_block = GetContainerNumber(current_image->nb_block,BLOCK_SIZE*8);
  for(i=0; i<nb_bitmap_block; i++)
    {
      if(verbose)
        printf("Bitmap;%04X;\n",0x0006+i);
      current_image->block_usage_type[0x0006+i] = BLOCK_TYPE_BITMAP;
    }

  /** Liste des Folders **/
  if(verbose)
    printf("; ------------------  Folder List  ------------------\n");
  my_Memory(MEMORY_GET_DIRECTORY_NB,&nb_directory,NULL);
  for(i=1; i<=nb_directory; i++)
    {
      my_Memory(MEMORY_GET_DIRECTORY,&i,&current_directory);
      if(verbose)
        printf("Folder;%s",current_directory->file_path);
      for(j=0; j<current_directory->nb_used_block; j++)
        if(current_directory->tab_used_block[j] != 0)
          {
            /* A qui ce block appartient */
            if(current_image->block_usage_type[current_directory->tab_used_block[j]] != BLOCK_TYPE_EMPTY)
              {
                /* Déjà occupé ! */
                object_info = GetObjectInfo(current_image->block_usage_type[current_directory->tab_used_block[j]],(struct file_descriptive_entry *)current_image->block_usage_object[current_directory->tab_used_block[j]]);
                sprintf(error_message,"Block %04X is claimed by Folder %s but it is already used by %s",current_directory->tab_used_block[j],current_directory->file_path,object_info);
                my_Memory(MEMORY_ADD_ERROR,error_message,NULL);
              }
            else
              {
                current_image->block_usage_type[current_directory->tab_used_block[j]] = BLOCK_TYPE_FOLDER;
                current_image->block_usage_object[current_directory->tab_used_block[j]] = current_directory;
              }
            /* Numéro du block */
            if(verbose)
              printf(";%04X",current_directory->tab_used_block[j]);
          }
      if(verbose)
        printf("\n");
    }

  /** Liste des Fichiers **/
  if(verbose)
    printf("; -------------------  File List  -------------------\n");
  my_Memory(MEMORY_GET_ENTRY_NB,&nb_file,NULL);
  for(i=1; i<=nb_file; i++)
    {
      my_Memory(MEMORY_GET_ENTRY,&i,&current_file);
      if(verbose)
        printf("File;%s",current_file->file_path);
      for(j=0; j<current_file->nb_used_block; j++)
        if(current_file->tab_used_block[j] != 0)
          {
            /* A qui ce block appartient */
            if(current_image->block_usage_type[current_file->tab_used_block[j]] != BLOCK_TYPE_EMPTY)
              {
                /* Déjà occupé ! */
                object_info = GetObjectInfo(current_image->block_usage_type[current_file->tab_used_block[j]],(struct file_descriptive_entry *)current_image->block_usage_object[current_file->tab_used_block[j]]);
                sprintf(error_message,"Block %04X is claimed by File %s but it is already used by %s",current_file->tab_used_block[j],current_file->file_path,object_info);
                my_Memory(MEMORY_ADD_ERROR,error_message,NULL);
              }
            else
              {
                current_image->block_usage_type[current_file->tab_used_block[j]] = BLOCK_TYPE_FILE;
                current_image->block_usage_object[current_file->tab_used_block[j]] = current_file;
              }

            /* Numéro du block */
            if(verbose)
              printf(";%04X",current_file->tab_used_block[j]);
          }
      if(verbose)
        printf("\n");
    }
  
  /** Liste des Blocks **/
  if(verbose)
    printf("; ------------------  Block List  -------------------\n");
  for(i=0, first_block_number=-1; i<current_image->nb_block; i++)
    {
      /** Décode le block **/
      if(current_image->block_usage_type[i] == BLOCK_TYPE_BOOT)
        sprintf(current_block_info,"Boot");
      else if(current_image->block_usage_type[i] == BLOCK_TYPE_VOLUME)
        sprintf(current_block_info,"Volume");
      else if(current_image->block_usage_type[i] == BLOCK_TYPE_BITMAP)
        sprintf(current_block_info,"Bitmap");
      else if(current_image->block_usage_type[i] == BLOCK_TYPE_FILE)
        sprintf(current_block_info,"File;%s",((struct file_descriptive_entry *)(current_image->block_usage_object[i]))->file_path);
      else if(current_image->block_usage_type[i] == BLOCK_TYPE_FOLDER)
        sprintf(current_block_info,"Folder;%s",((struct file_descriptive_entry *)(current_image->block_usage_object[i]))->file_path);
      else
        sprintf(current_block_info,"Free");

      /** Vérifie ce qui est déclaré dans la Bitmap (0=occupé, 1=libre) **/
      if(current_image->block_allocation_table[i] == 1 && current_image->block_usage_type[i] != 0)
        {
          sprintf(error_message,"Block %04X is declared FREE in the Bitmap, but used by %s",i,current_block_info);
          my_Memory(MEMORY_ADD_ERROR,error_message,NULL);
        }
      else if(current_image->block_allocation_table[i] == 0 && current_image->block_usage_type[i] == 0)
        {  
          sprintf(error_message,"Block %04X is declared IN USE in the Bitmap, but it not referenced by any object",i);
          my_Memory(MEMORY_ADD_ERROR,error_message,NULL);
        }

      /** Teste une continuité **/
      if(first_block_number == -1)
        {
          first_block_number = i;
          strcpy(first_block_info,current_block_info);
        }
      else
        {
          /* Fin d'une série */
          if(my_stricmp(first_block_info,current_block_info))
            {
              /* On produit la série précédente */
              if(verbose)
                {
                  if(i == first_block_number+1)
                    printf("%04X     ;%s\n",first_block_number,first_block_info);
                  else
                    printf("%04X-%04X;%s\n",first_block_number,i-1,first_block_info);
                }

              /* On stocke les infos pour la série suivante */
              first_block_number = i;
              strcpy(first_block_info,current_block_info);
            }
        }
    }
  /* Fin */
  if(verbose)
    {
      if(current_image->nb_block == first_block_number+1)
        printf("%04X     ;%s\n",first_block_number,first_block_info);
      else
        printf("%04X-%04X;%s\n",first_block_number,current_image->nb_block-1,first_block_info);
    }

  /** Liste des erreurs **/
  if(verbose)
    printf("; ------------------  Error List  -------------------\n");
  my_Memory(MEMORY_GET_ERROR_NB,&nb_error,NULL);
  for(i=1; i<=nb_error; i++)
    {
      my_Memory(MEMORY_GET_ERROR,&i,&current_error);
      printf("    => %s\n",current_error->message);
    }
}


/************************************************************/
/*  GetObjectInfo() :  Crée la chaine identifiant un objet. */
/************************************************************/
static char *GetObjectInfo(int type, struct file_descriptive_entry *current_entry)
{
  static char object_info[2048];

  if(type == BLOCK_TYPE_BOOT)
    sprintf(object_info,"Boot");
  else if(type == BLOCK_TYPE_VOLUME)
    sprintf(object_info,"Volume");
  else if(type == BLOCK_TYPE_BITMAP)
    sprintf(object_info,"Bitmap");
  else if(type == BLOCK_TYPE_FILE)
    sprintf(object_info,"File;%s",current_entry->file_path);
  else if(type == BLOCK_TYPE_FOLDER)
    sprintf(object_info,"Folder;%s",current_entry->file_path);
  else
    strcpy(object_info,"Unknown");

  /* Renvoi la description */
  return(&object_info[0]);
}

/***********************************************************************/