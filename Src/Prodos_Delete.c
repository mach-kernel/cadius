/***********************************************************************/
/*                                                                     */
/*   Prodos_Delete.c : Module pour la gestion des commandes DELETE.    */
/*                                                                     */
/***********************************************************************/
/*  Auteur : Olivier ZARDINI  *  Brutal Deluxe Software  *  Mar 2012   */
/***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

#include "Dc_Shared.h"
#include "Dc_Memory.h"
#include "Dc_Prodos.h"
#include "Prodos_Delete.h"

static void DeleteEntryFile(struct prodos_image *,struct file_descriptive_entry *);
static int EmptyEntryFolder(struct prodos_image *,struct file_descriptive_entry *,int);
static void DeleteEmptyFolder(struct prodos_image *,struct file_descriptive_entry *);
static int compare_folder(const void *,const void *);

/***********************************************************/
/*  DeleteProdosFile() :  Suppression d'un fichier Prodos. */
/***********************************************************/
void DeleteProdosFile(struct prodos_image *current_image, char *prodos_file_path)
{
  int error;
  struct file_descriptive_entry *current_entry;

  /* Recherche l'entrée Prodos */
  current_entry = GetProdosFile(current_image,prodos_file_path);
  if(current_entry == NULL)
    {
      printf("  Error : Invalid Prodos File path '%s'.\n",prodos_file_path);
      return;
    }

  /** Supprime une entrée Fichier **/
  DeleteEntryFile(current_image,current_entry);

  /** Ecrit le fichier **/
  error = UpdateProdosImage(current_image);
}


/******************************************************************/
/*  DeleteEntryFile() :  Suppression d'une entrée Fichier Prodos. */
/******************************************************************/
static void DeleteEntryFile(struct prodos_image *current_image, struct file_descriptive_entry *current_entry)
{
  WORD now_date, now_time, file_count;
  BYTE storage_type;
  int i, j, block_number, nb_bitmap_block, modified, total_modified, offset, subdirectory_block, current_block;
  struct file_descriptive_entry *current_directory;
  unsigned char directory_block[BLOCK_SIZE];
  unsigned char bitmap_block[BLOCK_SIZE];

  /* Date actuelle */
  GetCurrentDate(&now_date,&now_time);

  /* Nombre de block nécessaires pour stocker la table */
  nb_bitmap_block = GetContainerNumber(current_image->nb_block,BLOCK_SIZE*8);

  /**********************************************************/
  /** On va supprimer cette entrée de la structure mémoire **/

  /* Supprime cette entrée de la liste des entrées */
  my_Memory(MEMORY_REMOVE_ENTRY,current_entry,NULL);

  /** Supprime cette entrée du Répertoire dans lequel elle est enregistrée **/
  current_directory = current_entry->parent_directory;
  if(current_directory == NULL)
    {
      /** L'entrée est à la racine du volume **/
      UpdateEntryTable(UPDATE_REMOVE,&current_image->nb_file,&current_image->tab_file,current_entry);

      /** Last Modification date : Volume Header **/
      GetProdosDate(now_date,&current_image->volume_header->volume_modification_date);
      GetProdosTime(now_time,&current_image->volume_header->volume_modification_time);
    }
  else
    {
      /** L'entrée est dans un sous répertoire **/
      UpdateEntryTable(UPDATE_REMOVE,&current_directory->nb_file,&current_directory->tab_file,current_entry);

      /** Last Modification date : Directory **/
      GetProdosDate(now_date,&current_directory->file_modification_date);
      GetProdosTime(now_time,&current_directory->file_modification_time);
    }

  /** Marque les blocs occupés par le fichier comme libres **/
  for(i=0; i<current_entry->nb_used_block; i++)
    {
      block_number = current_entry->tab_used_block[i];
      current_image->block_allocation_table[block_number] = 1;    /* Libre */
    }
  current_image->nb_free_block += current_entry->nb_used_block;

  /***********************************/
  /** On va modifier l'image disque **/
  /** Directory Block **/
  GetBlockData(current_image,current_entry->block_location,&directory_block[0]);

  /** Place les informations dans le Directory : Marque l'entrée comme supprimée **/
  storage_type = 0x00;
  memcpy(&directory_block[current_entry->entry_offset+FILE_STORAGETYPE_OFFSET],&storage_type,sizeof(BYTE));

  /* Modifie le block Directory */
  SetBlockData(current_image,current_entry->block_location,&directory_block[0]);

  /** Sub-Directory Block **/
  current_block = current_entry->block_location;
  subdirectory_block = GetWordValue(&directory_block[0],0);
  while(subdirectory_block != 0)
    {
      current_block = subdirectory_block;
      GetBlockData(current_image,subdirectory_block,&directory_block[0]);
      subdirectory_block = GetWordValue(&directory_block[0],0);
    }

  /* Une entrée en moins dans ce Directory */
  file_count = GetWordValue(&directory_block[0],DIRECTORY_FILECOUNT_OFFSET);
  if(file_count > 0)
    file_count--;
  SetWordValue(&directory_block[0],DIRECTORY_FILECOUNT_OFFSET,file_count);

  /* Modifie le block Sub-Directory */
  SetBlockData(current_image,current_block,&directory_block[0]);

  /** Marque les blocs occupés par le fichier comme libres **/
  total_modified = 0;
  for(i=0; i<nb_bitmap_block; i++)
    {
      /* Bitmap block */
      GetBlockData(current_image,current_image->volume_header->bitmap_block+i,&bitmap_block[0]);
      modified = 0;

      /** Passe en revue les blocs utilisés du fichier **/
      for(j=0; j<current_entry->nb_used_block; j++)
        if((current_entry->tab_used_block[j] >= i*8*BLOCK_SIZE) && (current_entry->tab_used_block[j] < (i+1)*8*BLOCK_SIZE))
          {
            offset = current_entry->tab_used_block[j] - i*8*BLOCK_SIZE;
            bitmap_block[offset/8] |= (0x01 << (7-(offset%8)));    /* 1 : Libre */
            modified = 1;
            total_modified++;

            /* Pas besoin de tout parcourir */
            if(total_modified == current_entry->nb_used_block)
              break;
          }

      /* Modifie le block Bitmap */
      if(modified)
        SetBlockData(current_image,current_image->volume_header->bitmap_block+i,&bitmap_block[0]);

      /* Pas besoin de tout parcourir */
      if(total_modified == current_entry->nb_used_block)
        break;
    }

  /* Libération mémoire de la structure */
  mem_free_entry(current_entry);
}


/*************************************************************/
/*  DeleteProdosFolder() :  Suppression d'un dossier Prodos. */
/*************************************************************/
void DeleteProdosFolder(struct prodos_image *current_image, char *prodos_folder_path)
{
  int i, j, error, nb_folder, nb_directory;
  struct file_descriptive_entry **tab_folder;
  struct file_descriptive_entry *current_entry;
  struct file_descriptive_entry *current_directory;

  /* Recherche le dossier Prodos */
  current_entry = GetProdosFolder(current_image,prodos_folder_path,0);
  if(current_entry == NULL)
    {
      printf("  Error : Invalid Prodos Folder path '%s'.\n",prodos_folder_path);
      return;
    }

  /** Supprime tous les fichiers (récursivité dans les sous-répertoires) **/
  nb_folder = EmptyEntryFolder(current_image,current_entry,1);

  /** Construit la liste des répertoires à supprimer **/
  tab_folder = (struct file_descriptive_entry **) calloc(nb_folder,sizeof(struct file_descriptive_entry *));
  if(tab_folder == NULL)
    {
      printf("  Error : Impossible to allocate memory for table 'tab_folder'.\n");
      return;
    }
  my_Memory(MEMORY_GET_DIRECTORY_NB,&nb_directory,NULL);
  for(i=1, j=0; i<=nb_directory; i++)
    {
      my_Memory(MEMORY_GET_DIRECTORY,&i,&current_directory);
      if(current_directory->delete_folder_depth > 0)
        tab_folder[j++] = current_directory;
    }
  qsort(tab_folder,nb_folder,sizeof(struct file_descriptive_entry *),compare_folder);

  /** Supprime tous les Sous-répertoires vides, par ordre de niveau **/
  for(i=0; i<nb_folder; i++)
    DeleteEmptyFolder(current_image,tab_folder[i]);

  /* Libération mémoire */
  free(tab_folder);

  /** Ecrit le fichier **/
  error = UpdateProdosImage(current_image);
}


/*****************************************************************/
/*  EmptyEntryFolder() :  Suppression des fichiers d'un dossier. */
/*****************************************************************/
static int EmptyEntryFolder(struct prodos_image *current_image, struct file_descriptive_entry *current_entry, int depth)
{
  int i, nb_folder;

  /* Marque ce répertoire comme devant être supprimé */
  nb_folder = 1;
  current_entry->delete_folder_depth = depth;

  /** Supprime tous les fichiers du réperoire **/
  while(current_entry->nb_file > 0)
    DeleteEntryFile(current_image,current_entry->tab_file[0]);

  /** Vide tous les sous-répertoires de leurs fichiers (récursivité) **/
  for(i=0; i<current_entry->nb_directory; i++)
    nb_folder += EmptyEntryFolder(current_image,current_entry->tab_directory[i],depth+1);

  /* Renvoi le nombre de sous-dossier */
  return(nb_folder);
}


/**********************************************************/
/*  DeleteEmptyFolder() :  Suppression d'un dossier vide. */
/**********************************************************/
void DeleteEmptyFolder(struct prodos_image *current_image, struct file_descriptive_entry *current_entry)
{
  WORD now_date, now_time, file_count;
  int i, j, block_number, modified, offset, current_block, subdirectory_block, nb_bitmap_block, total_modified;
  BYTE storage_type;
  struct file_descriptive_entry *current_directory;
  unsigned char directory_block[BLOCK_SIZE];
  unsigned char bitmap_block[BLOCK_SIZE];

  /* On vérifie que le répertoire est vide */
  if(current_entry->nb_file != 0 || current_entry->nb_directory != 0)
    return;

  /* Date actuelle */
  GetCurrentDate(&now_date,&now_time);

  /* Nombre de block nécessaires pour stocker la table */
  nb_bitmap_block = GetContainerNumber(current_image->nb_block,BLOCK_SIZE*8);

  /**********************************************************/
  /** On va supprimer cette entrée de la structure mémoire **/

  /* Supprime cette entrée de la liste des entrées */
  my_Memory(MEMORY_REMOVE_DIRECTORY,current_entry,NULL);

  /** Supprime cette entrée répertoire du Répertoire dans lequel elle est enregistrée **/
  current_directory = current_entry->parent_directory;
  if(current_directory == NULL)
    {
      /** Le répertoire est à la racine du volume **/
      UpdateEntryTable(UPDATE_REMOVE,&current_image->nb_directory,&current_image->tab_directory,current_entry);

      /** Last Modification date : Volume Header **/
      GetProdosDate(now_date,&current_image->volume_header->volume_modification_date);
      GetProdosTime(now_time,&current_image->volume_header->volume_modification_time);
    }
  else
    {
      /** Le répertoire est dans un sous répertoire **/
      UpdateEntryTable(UPDATE_REMOVE,&current_directory->nb_directory,&current_directory->tab_directory,current_entry);

      /** Last Modification date : Directory **/
      GetProdosDate(now_date,&current_directory->file_modification_date);
      GetProdosTime(now_time,&current_directory->file_modification_time);
    }

  /** Marque les blocs occupés par le répertoire comme libres **/
  for(i=0; i<current_entry->nb_used_block; i++)
    {
      block_number = current_entry->tab_used_block[i];
      current_image->block_allocation_table[block_number] = 1;    /* Libre */
    }
  current_image->nb_free_block += current_entry->nb_used_block;

  /***********************************/
  /** On va modifier l'image disque **/
  /** Directory Block **/
  GetBlockData(current_image,current_entry->block_location,&directory_block[0]);

  /** Place les informations dans le Directory : Marque l'entrée comme supprimée **/
  storage_type = 0x00;
  memcpy(&directory_block[current_entry->entry_offset+FILE_STORAGETYPE_OFFSET],&storage_type,sizeof(BYTE));

  /* Modifie le block Directory */
  SetBlockData(current_image,current_entry->block_location,&directory_block[0]);

  /** Sub-Directory Block **/
  current_block = current_entry->block_location;
  subdirectory_block = GetWordValue(&directory_block[0],0);
  while(subdirectory_block != 0)
    {
      current_block = subdirectory_block;
      GetBlockData(current_image,subdirectory_block,&directory_block[0]);
      subdirectory_block = GetWordValue(&directory_block[0],0);
    }

  /* Une entrée en moins dans ce Directory */
  file_count = GetWordValue(&directory_block[0],DIRECTORY_FILECOUNT_OFFSET);
  if(file_count > 0)
    file_count--;
  SetWordValue(&directory_block[0],DIRECTORY_FILECOUNT_OFFSET,file_count);

  /* Modifie le block Sub-Directory */
  SetBlockData(current_image,current_block,&directory_block[0]);

  /** Marque les blocs occupés par le répertoire comme libres **/
  total_modified = 0;
  for(i=0; i<nb_bitmap_block; i++)
    {
      /* Bitmap block */
      GetBlockData(current_image,current_image->volume_header->bitmap_block+i,&bitmap_block[0]);
      modified = 0;

      /** Passe en revue les blocs utilisés du répertoire **/
      for(j=0; j<current_entry->nb_used_block; j++)
        if((current_entry->tab_used_block[j] >= i*8*BLOCK_SIZE) && (current_entry->tab_used_block[j] < (i+1)*8*BLOCK_SIZE))
          {
            offset = current_entry->tab_used_block[j] - i*8*BLOCK_SIZE;
            bitmap_block[offset/8] |= (0x01 << (7-(offset%8)));    /* 1 : Libre */
            modified = 1;
            total_modified++;

            /* Pas besoin de tout parcourir */
            if(total_modified == current_entry->nb_used_block)
              break;
          }

      /* Modifie le block Bitmap */
      if(modified)
        SetBlockData(current_image,current_image->volume_header->bitmap_block+i,&bitmap_block[0]);

      /* Pas besoin de tout parcourir */
      if(total_modified == current_entry->nb_used_block)
        break;
    }

  /* Libération mémoire de la structure */
  mem_free_entry(current_entry);
}


/**********************************************************/
/*  DeleteProdosVolume() :  Suppression du volume Prodos. */
/**********************************************************/
void DeleteProdosVolume(struct prodos_image *current_image)
{
  WORD now_date, now_time, nb_entry;
  int i, j, error, nb_bitmap_block, block_number;
  unsigned char volume_block[BLOCK_SIZE];
  unsigned char bitmap_block[BLOCK_SIZE];
  unsigned char data_block[BLOCK_SIZE];

  /* Date actuelle */
  GetCurrentDate(&now_date,&now_time);

  /* Nombre de block nécessaires pour stocker la table */
  nb_bitmap_block = GetContainerNumber(current_image->nb_block,BLOCK_SIZE*8);

  /************************************************************/
  /** On va supprimer tous les fichiers la structure mémoire **/
  /* Plus de fichier à la racine */
  current_image->nb_file = 0;
  if(current_image->tab_file)
    free(current_image->tab_file);
  current_image->tab_file = NULL;
  /* Plus de fichier à la racine */
  current_image->nb_directory = 0;
  if(current_image->tab_directory)
    free(current_image->tab_directory);
  current_image->tab_directory = NULL;

  /* Libération mémoire : Plus de fichiers nul part */
  my_Memory(MEMORY_FREE_DIRECTORY,NULL,NULL);
  my_Memory(MEMORY_FREE_ENTRY,NULL,NULL);

  /* Nettoyage de la Bitmap */
  for(i=0; i<current_image->nb_block; i++)
    current_image->block_allocation_table[i] = (i < (2+4+nb_bitmap_block)) ? 0 : 1;      /* 0 : Busy / 1 : Free */
  current_image->nb_free_block = current_image->nb_block - (2 + 4 + nb_bitmap_block);

  /** Volume Header **/
  GetProdosDate(now_date,&current_image->volume_header->volume_modification_date);
  GetProdosTime(now_time,&current_image->volume_header->volume_modification_time);

  /***********************************/
  /** On va modifier l'image disque **/
  
  /** Nettoyage du Volume Directory **/
  for(i=0; i<4; i++)
    {
      /* Volume Block */
      GetBlockData(current_image,2+i,&volume_block[0]);

      /** Place les informations dans le Volume Header **/
      if(i == 0)
        {
          /** Volume Header **/
          /* Nombre d'entrées */
          nb_entry = 0;
          memcpy(&volume_block[VOLUME_FILECOUNT_OFFSET],&nb_entry,sizeof(WORD));

          /* Date de Modif */
          memcpy(&volume_block[VOLUME_DATEMODIF_OFFSET],&now_date,sizeof(WORD));
          memcpy(&volume_block[VOLUME_TIMEMODIF_OFFSET],&now_time,sizeof(WORD));

          /** Entrées Suivantes **/
          memset(&volume_block[4+0x27],0,BLOCK_SIZE-(4+0x27));
        }
      else
        memset(&volume_block[4],0,BLOCK_SIZE-4);

      /* Modifie le block */
      SetBlockData(current_image,2+i,&volume_block[0]);
    }

  /** Nettoyage de la Bitmap **/
  /* On vide tous les block */
  block_number = current_image->volume_header->bitmap_block;
  for(i=0; i<nb_bitmap_block; i++)
    {
      /* Récupère le block */
      GetBlockData(current_image,block_number+i,&bitmap_block[0]);

      /* Nettoyage préliminaire */
      memset(bitmap_block,0,BLOCK_SIZE);

      /* Marque les bloc libres */
      for(j=0; j<(8*BLOCK_SIZE); j++)
        if((i*8*BLOCK_SIZE)+j < current_image->nb_block)
          if(current_image->block_allocation_table[(i*8*BLOCK_SIZE)+j] == 1)
            bitmap_block[j/8] |= (0x01 << (7-(j%8)));


      /* Modifie le block */
      SetBlockData(current_image,block_number+i,&bitmap_block[0]);
    }

  /** Vide tous les blocs **/
  memset(&data_block[0],0,BLOCK_SIZE);
  for(i=(block_number+nb_bitmap_block); i<current_image->nb_block; i++)
    SetBlockData(current_image,i,&data_block[0]);

  /** Ecrit le fichier **/
  error = UpdateProdosImage(current_image);
}


/*******************************************************************/
/*  compare_folder() : Fonction de comparaison pour le Quick Sort. */
/*******************************************************************/
static int compare_folder(const void *data_1, const void *data_2)
{
  struct file_descriptive_entry *entry_1;
  struct file_descriptive_entry *entry_2;

  /* Récupération des paramètres */
  entry_1 = *((struct file_descriptive_entry **) data_1);
  entry_2 = *((struct file_descriptive_entry **) data_2);

  /* Comparaison des noms */
  if(entry_1->delete_folder_depth == entry_2->delete_folder_depth)
    return(0);
  else if(entry_1->delete_folder_depth < entry_2->delete_folder_depth)
    return(1);
  else
    return(-1);
}

/***********************************************************************/
