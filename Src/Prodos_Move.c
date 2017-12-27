/***********************************************************************/
/*                                                                     */
/*   Prodos_Move.c : Module pour la gestion des commandes MOVE.        */
/*                                                                     */
/***********************************************************************/
/*  Auteur : Olivier ZARDINI  *  Brutal Deluxe Software  *  Mar 2012   */
/***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <malloc.h>

#include "Dc_Shared.h"
#include "Dc_Prodos.h"
#include "Dc_Memory.h"
#include "Dc_OS.h"
#include "Prodos_Create.h"
#include "Prodos_Move.h"

static int MoveProdosFileToFolder(struct prodos_image *,struct file_descriptive_entry *,struct file_descriptive_entry *);
static int MoveProdosFolderToFolder(struct prodos_image *,struct file_descriptive_entry *,struct file_descriptive_entry *);
static void ChangeDirectoryEntriesDepth(struct file_descriptive_entry *,int);
static void ChangeDirectoryEntriesPath(struct file_descriptive_entry *,char *,char *);
static char *ChangeEntryPath(char *,char *,char *);


/*********************************************************/
/*  MoveProdosFile() :  Déplacement d'un fichier Prodos. */
/*********************************************************/
void MoveProdosFile(struct prodos_image *current_image, char *prodos_file_path, char *target_folder_path)
{
  int is_volume_header, error;
  struct file_descriptive_entry *current_entry;
  struct file_descriptive_entry *target_folder;

  /* Recherche l'entrée Prodos */
  current_entry = GetProdosFile(current_image,prodos_file_path);
  if(current_entry == NULL)
    {
      printf("  Error : Invalid Prodos File path '%s'.\n",prodos_file_path);
      return;
    }

  /** Recherche le dossier Prodos Cible où déplacer le fichier **/
  target_folder = BuildProdosFolderPath(current_image,target_folder_path,&is_volume_header,0);
  if(target_folder == NULL && is_volume_header == 0)
    return;

  /** Déplace le fichier dans un Dossier existant ou à la Racine du volume **/
  error = MoveProdosFileToFolder(current_image,target_folder,current_entry);
  if(error)
    return;

  /** Ecrit le fichier Image **/
  error = UpdateProdosImage(current_image);
}


/***********************************************************/
/*  MoveProdosFolder() :  Déplacement d'un dossier Prodos. */
/***********************************************************/
void MoveProdosFolder(struct prodos_image *current_image, char *prodos_folder_path, char *target_folder_path)
{
  int is_volume_header, error;
  struct file_descriptive_entry *current_entry;
  struct file_descriptive_entry *target_folder;

  /* Recherche le dossier Prodos (on interdit le Nom du Volume) */
  current_entry = GetProdosFolder(current_image,prodos_folder_path,0);
  if(current_entry == NULL)
    {
      printf("  Error : Invalid Prodos Folder path '%s'.\n",prodos_folder_path);
      return;
    }

  /** Recherche le dossier Prodos Cible où déplacer le dossier **/
  target_folder = BuildProdosFolderPath(current_image,target_folder_path,&is_volume_header,0);
  if(target_folder == NULL && is_volume_header == 0)
    return;

  /** Déplace le dossier dans un Dossier existant ou à la Racine du volume **/
  error = MoveProdosFolderToFolder(current_image,target_folder,current_entry);
  if(error)
    return;

  /** Ecrit le fichier Image **/
  error = UpdateProdosImage(current_image);
}


/**************************************************************************************/
/*  MoveProdosFileToFolder() :  Déplace un fichier dans un répertoire ou à la racine. */
/**************************************************************************************/
int MoveProdosFileToFolder(struct prodos_image *current_image, struct file_descriptive_entry *target_folder, struct file_descriptive_entry *current_file)
{
  int i, error, target_offset, file_block_number, file_block_offset, entry_length, file_header_pointer, file_count;
  WORD directory_block_number, directory_header_pointer;
  BYTE directory_entry_number;
  char *new_path;
  unsigned char directory_block[BLOCK_SIZE];
  unsigned char file_entry_block[BLOCK_SIZE];

  /** Vérifie que ce nom de fichier ne correspond pas déjà à un nom de fichier/dossier **/
  if(target_folder == NULL)
    {
      /* Vérification à la racine du Volume */
      for(i=0; i<current_image->nb_file; i++)
        if(!my_stricmp(current_file->file_name_case,current_image->tab_file[i]->file_name))
          {
            printf("  Error : Invalid target location. A file already exist with the same name '%s'.\n",current_image->tab_file[i]->file_name);
            return(1);
          }
      for(i=0; i<current_image->nb_directory; i++)
        if(!my_stricmp(current_file->file_name_case,current_image->tab_directory[i]->file_name))
          {
            printf("  Error : Invalid target location. A folder already exist with the same name '%s'.\n",current_image->tab_directory[i]->file_name);
            return(1);
          }
    }
  else
    {
      /* Vérification dans le dossier */
      for(i=0; i<target_folder->nb_file; i++)
        if(!my_stricmp(current_file->file_name_case,target_folder->tab_file[i]->file_name))
          {
            printf("  Error : Invalid target location. A file already exist with the same name '%s'.\n",target_folder->tab_file[i]->file_name);
            return(1);
          }
      for(i=0; i<target_folder->nb_directory; i++)
        if(!my_stricmp(current_file->file_name_case,target_folder->tab_directory[i]->file_name))
          {
            printf("  Error : Invalid target location. A folder already exist with the same name '%s'.\n",target_folder->tab_directory[i]->file_name);
            return(1);
          }
    }

  /** Recherche d'une entrée libre dans le répertoire **/
  error = AllocateFolderEntry(current_image,target_folder,&directory_block_number,&directory_entry_number,&directory_header_pointer);
  if(error)
    return(2);

  /*** Recopie les données de l'entrée du Fichier dans cette entrée ***/
  entry_length = 0x27;

  /* Récupère les données de l'entrée */
  file_block_number = current_file->block_location;
  file_block_offset = current_file->entry_offset;
  GetBlockData(current_image,file_block_number,&file_entry_block[0]);
  file_header_pointer = GetWordValue(file_entry_block,file_block_offset+0x25);

  /* Récupère les données du dossier cible */
  target_offset = 4 + (directory_entry_number-1)*entry_length;
  GetBlockData(current_image,directory_block_number,&directory_block[0]);
  
  /** Copie les données de l'entrée dans le nouveau dossier **/
  memcpy(&directory_block[target_offset],&file_entry_block[file_block_offset],entry_length);
  /* Modifie le Header Pointer */
  SetWordValue(directory_block,target_offset+0x25,directory_header_pointer);
  /* Ecrit les données */
  SetBlockData(current_image,directory_block_number,&directory_block[0]);

  /** Efface l'entrée de l'ancien Dossier **/
  memset(&file_entry_block[file_block_offset],0,entry_length);
  SetBlockData(current_image,file_block_number,&file_entry_block[0]);

  /* Modifie le nombre d'entrées valides du Target Folder : +1 */
  GetBlockData(current_image,directory_header_pointer,&directory_block[0]);
  file_count = GetWordValue(directory_block,0x25);
  SetWordValue(directory_block,0x25,(WORD)(file_count+1));
  SetBlockData(current_image,directory_header_pointer,&directory_block[0]);

  /* Modifie le nombre d'entrées valides de l'ancien Folder : -1 */
  GetBlockData(current_image,file_header_pointer,&directory_block[0]);
  file_count = GetWordValue(directory_block,0x25);
  SetWordValue(directory_block,0x25,(WORD)(file_count-1));
  SetBlockData(current_image,file_header_pointer,&directory_block[0]);

  /***************************************/
  /*** Met à jour la structure mémoire ***/
  /** Met à jour le Dossier source (-1 fichier) **/
  if(current_file->parent_directory == NULL)
    UpdateEntryTable(UPDATE_REMOVE,&current_image->nb_file,&current_image->tab_file,current_file);
  else
    UpdateEntryTable(UPDATE_REMOVE,&current_file->parent_directory->nb_file,&current_file->parent_directory->tab_file,current_file);

  /** Met à jour le Dossier Cible (+1 fichier) **/
  if(target_folder == NULL)
    error = UpdateEntryTable(UPDATE_ADD,&current_image->nb_file,&current_image->tab_file,current_file);
  else
    error = UpdateEntryTable(UPDATE_ADD,&target_folder->nb_file,&target_folder->tab_file,current_file);
  if(error)
    {
      printf("  Error : Memory allocation impossible.\n");
      return(1);
    }

  /** Met à jour l'entrée en mémoire **/
  /* Nouveau Path */
  new_path = (char *) calloc((target_folder==NULL)?1+strlen(current_image->volume_header->volume_name_case)+1+strlen(current_file->file_name_case)+1:
                                                   strlen(target_folder->file_path)+1+strlen(current_file->file_name_case)+1,
                                                   sizeof(char));
  if(new_path == NULL)
    {
      printf("  Error : Memory allocation impossible.\n");
      return(1);
    }
  if(target_folder == NULL)
    sprintf(new_path,"/%s/%s",current_image->volume_header->volume_name_case,current_file->file_name_case);
  else
    sprintf(new_path,"%s/%s",target_folder->file_path,current_file->file_name_case);
  if(current_file->file_path)
    free(current_file->file_path);
  current_file->file_path = new_path;

  /* Nouveau Header Pointer Block */
  current_file->header_pointer_block = directory_header_pointer;

  /* Nouvelle profondeur (1=racine -> N) */
  current_file->depth = (target_folder == NULL) ? 1 : target_folder->depth + 1;

  /* Nouvelle position dans un Folder */
  current_file->block_location = directory_block_number;
  current_file->entry_offset = target_offset;

  /* Nouveau Parent Directory */
  current_file->parent_directory = target_folder;

  /* OK */
  return(0);
}


/****************************************************************************************/
/*  MoveProdosFolderToFolder() :  Déplace un dossier dans un répertoire ou à la racine. */
/****************************************************************************************/
int MoveProdosFolderToFolder(struct prodos_image *current_image, struct file_descriptive_entry *target_folder, struct file_descriptive_entry *current_folder)
{
  int i, error, target_offset, file_block_number, file_block_offset, entry_length, file_header_pointer, file_count, depth_delta;
  WORD directory_block_number, directory_header_pointer;
  BYTE directory_entry_number;
  char old_path[2048];
  char new_path[2048];
  unsigned char directory_block[BLOCK_SIZE];
  unsigned char file_entry_block[BLOCK_SIZE];

  /* Ecart de profondeur (1=racine->N) */
  depth_delta = ((target_folder == NULL) ? 1 : target_folder->depth+1) - (current_folder->depth);

  /* Translation de Chemin */
  strcpy(old_path,current_folder->file_path);
  if(target_folder == NULL)
    sprintf(new_path,"/%s/%s",current_image->volume_header->volume_name_case,current_folder->file_name_case);
  else
    sprintf(new_path,"%s/%s",target_folder->file_path,current_folder->file_name_case);

  /* On ne peut pas déplacer un Dossier au même endroit */
  if(!my_stricmp(old_path,new_path))
    {
      printf("  Error : Invalid target location. The Folder is moved at the same location : '%s'.\n",target_folder->file_name_case);
      return(1);
    }
  /* On ne peut pas déplacer un Dossier sous lui-même */
  if(strlen(new_path) > strlen(old_path))
    if(!my_strnicmp(old_path,new_path,strlen(old_path)))
      {
        printf("  Error : Invalid target location. The Folder is moved under itself : '%s'.\n",target_folder->file_name_case);
        return(1);
      }

  /** Vérifie que ce nom de fichier ne correspond pas déjà à un nom de fichier/dossier **/
  if(target_folder == NULL)
    {
      /* Vérification à la racine du Volume */
      for(i=0; i<current_image->nb_file; i++)
        if(!my_stricmp(current_folder->file_name_case,current_image->tab_file[i]->file_name))
          {
            printf("  Error : Invalid target location. A file already exist with the same name '%s'.\n",current_image->tab_file[i]->file_name);
            return(1);
          }
      for(i=0; i<current_image->nb_directory; i++)
        if(!my_stricmp(current_folder->file_name_case,current_image->tab_directory[i]->file_name))
          {
            printf("  Error : Invalid target location. A folder already exist with the same name '%s'.\n",current_image->tab_directory[i]->file_name);
            return(1);
          }
    }
  else
    {
      /* Vérification dans le dossier */
      for(i=0; i<target_folder->nb_file; i++)
        if(!my_stricmp(current_folder->file_name_case,target_folder->tab_file[i]->file_name))
          {
            printf("  Error : Invalid target location. A file already exist with the same name '%s'.\n",target_folder->tab_file[i]->file_name);
            return(1);
          }
      for(i=0; i<target_folder->nb_directory; i++)
        if(!my_stricmp(current_folder->file_name_case,target_folder->tab_directory[i]->file_name))
          {
            printf("  Error : Invalid target location. A folder already exist with the same name '%s'.\n",target_folder->tab_directory[i]->file_name);
            return(1);
          }
    }

  /** Recherche d'une entrée libre dans le répertoire **/
  error = AllocateFolderEntry(current_image,target_folder,&directory_block_number,&directory_entry_number,&directory_header_pointer);
  if(error)
    return(2);

  /*** Recopie les données de l'entrée du Fichier dans cette entrée ***/
  entry_length = 0x27;

  /* Récupère les données de l'entrée */
  file_block_number = current_folder->block_location;
  file_block_offset = current_folder->entry_offset;
  GetBlockData(current_image,file_block_number,&file_entry_block[0]);
  file_header_pointer = GetWordValue(file_entry_block,file_block_offset+FILE_HEADERPOINTER_OFFSET);

  /* Récupère les données du dossier cible */
  target_offset = 4 + (directory_entry_number-1)*entry_length;
  GetBlockData(current_image,directory_block_number,&directory_block[0]);
  
  /** Copie les données de l'entrée dans le nouveau dossier **/
  memcpy(&directory_block[target_offset],&file_entry_block[file_block_offset],entry_length);
  /* Modifie le Header Pointer */
  SetWordValue(directory_block,target_offset+FILE_HEADERPOINTER_OFFSET,directory_header_pointer);
  /* Ecrit les données */
  SetBlockData(current_image,directory_block_number,&directory_block[0]);

  /** Efface l'entrée de l'ancien Dossier **/
  memset(&file_entry_block[file_block_offset],0,entry_length);
  SetBlockData(current_image,file_block_number,&file_entry_block[0]);

  /* Modifie le nombre d'entrées valides du Target Folder : +1 */
  GetBlockData(current_image,directory_header_pointer,&directory_block[0]);
  file_count = GetWordValue(directory_block,DIRECTORY_FILECOUNT_OFFSET);
  SetWordValue(directory_block,DIRECTORY_FILECOUNT_OFFSET,(WORD)(file_count+1));
  SetBlockData(current_image,directory_header_pointer,&directory_block[0]);

  /* Modifie le nombre d'entrées valides de l'ancien Folder : -1 */
  GetBlockData(current_image,file_header_pointer,&directory_block[0]);
  file_count = GetWordValue(directory_block,DIRECTORY_FILECOUNT_OFFSET);
  SetWordValue(directory_block,DIRECTORY_FILECOUNT_OFFSET,(WORD)(file_count-1));
  SetBlockData(current_image,file_header_pointer,&directory_block[0]);

  /** Modifie le Parent Pointer Block et le Parent Entry du dossier que l'on déplace **/
  GetBlockData(current_image,current_folder->key_pointer_block,&directory_block[0]);
  SetWordValue(directory_block,DIRECTORY_PARENTPOINTERBLOCK_OFFSET,(WORD)directory_block_number);
  directory_block[DIRECTORY_PARENTENTRY_OFFSET] = (BYTE) directory_entry_number;
  SetBlockData(current_image,current_folder->key_pointer_block,&directory_block[0]);

  /***************************************/
  /*** Met à jour la structure mémoire ***/
  /** Met à jour le Dossier source (-1 fichier) **/
  if(current_folder->parent_directory == NULL)
    UpdateEntryTable(UPDATE_REMOVE,&current_image->nb_file,&current_image->tab_file,current_folder);
  else
    UpdateEntryTable(UPDATE_REMOVE,&current_folder->parent_directory->nb_file,&current_folder->parent_directory->tab_file,current_folder);

  /** Met à jour le Dossier Cible (+1 fichier) **/
  if(target_folder == NULL)
    error = UpdateEntryTable(UPDATE_ADD,&current_image->nb_file,&current_image->tab_file,current_folder);
  else
    error = UpdateEntryTable(UPDATE_ADD,&target_folder->nb_file,&target_folder->tab_file,current_folder);
  if(error)
    {
      printf("  Error : Memory allocation impossible.\n");
      return(1);
    }

  /** Met à jour l'entrée en mémoire **/

  /* Nouveau Header Pointer Block */
  current_folder->header_pointer_block = directory_header_pointer;

  /* Nouvelle position dans un Folder */
  current_folder->block_location = directory_block_number;
  current_folder->entry_offset = target_offset;

  /* Nouveau Parent Directory */
  current_folder->parent_directory = target_folder;

  /** Modifie toutes les entrées de ce Répertoire **/
  /* Depth */
  ChangeDirectoryEntriesDepth(current_folder,depth_delta);

  /* Path */
  ChangeDirectoryEntriesPath(current_folder,old_path,new_path);

  /* OK */
  return(0);
}


/*********************************************************************************/
/*  ChangeDirectoryEntriesDepth() :  Change le niveau de profondeur des entrées. */
/*********************************************************************************/
static void ChangeDirectoryEntriesDepth(struct file_descriptive_entry *current_folder, int depth_delta)
{
  int i;

  /* Le dossier lui même */
  current_folder->depth += depth_delta;

  /* Les File du dossier */
  for(i=0; i<current_folder->nb_file; i++)
    current_folder->tab_file[i]->depth += depth_delta;

  /* Les Folder du dossier (récursivité) */
  for(i=0; i<current_folder->nb_directory; i++)
    ChangeDirectoryEntriesDepth(current_folder->tab_directory[i],depth_delta);
}


/******************************************************************/
/*  ChangeDirectoryEntriesPath() :  Change le chemin des entrées. */
/******************************************************************/
static void ChangeDirectoryEntriesPath(struct file_descriptive_entry *current_folder, char *old_path, char *new_path)
{
  int i;

  /* Le dossier lui même */
  current_folder->file_path = ChangeEntryPath(current_folder->file_path,old_path,new_path);

  /* Les File du dossier */
  for(i=0; i<current_folder->nb_file; i++)
    current_folder->tab_file[i]->file_path = ChangeEntryPath(current_folder->tab_file[i]->file_path,old_path,new_path);

  /* Les Folder du dossier (récursivité) */
  for(i=0; i<current_folder->nb_directory; i++)
    ChangeDirectoryEntriesPath(current_folder->tab_directory[i],old_path,new_path);
}


/*****************************************************/
/*  ChangeEntryPath() :  Création du nouveau chemin. */
/*****************************************************/
static char *ChangeEntryPath(char *current_file_path, char *old_path, char *new_path)
{
  int length;
  char *new_file_path;

  /* Taille du chemin */
  length = strlen(new_path) + strlen(&current_file_path[strlen(old_path)]);

  /* Allocation mémoire */
  new_file_path = (char *) calloc(length+1,sizeof(char));
  if(new_file_path == NULL)
    {
      printf("  Error : Memory allocation impossible.\n");
      return(current_file_path);
    }

  /* Nouveau chemin */
  strcpy(new_file_path,new_path);
  strcat(new_file_path,&current_file_path[strlen(old_path)]);

  /* Libératioon mémoire ancien chemin */
  free(current_file_path);

  /* Renvoi le chemin */
  return(new_file_path);
}

/***********************************************************************/
