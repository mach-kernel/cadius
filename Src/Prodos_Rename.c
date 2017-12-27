/***********************************************************************/
/*                                                                     */
/*   Prodos_Rename.c : Module pour la gestion des commandes RENAME.    */
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
#include "Prodos_Rename.h"

/********************************************************/
/*  RenameProdosFile() :  Renomage d'un fichier Prodos. */
/********************************************************/
void RenameProdosFile(struct prodos_image *current_image, char *prodos_file_path, char *new_file_name)
{
  char upper_case[256];
  WORD name_case, now_date, now_time;
  BYTE storage_length;
  int i, is_valid, error;
  unsigned char name_length;
  struct file_descriptive_entry *current_entry;
  unsigned char directory_block[BLOCK_SIZE];

  /* Recherche l'entrée Prodos */
  current_entry = GetProdosFile(current_image,prodos_file_path);
  if(current_entry == NULL)
    {
      printf("  Error : Invalid Prodos File path '%s'.\n",prodos_file_path);
      return;
    }

  /* Vérification du nouveau nom */
  is_valid = CheckProdosName(new_file_name);
  if(is_valid == 0)
    {
      printf("  Error : Invalid Prodos name '%s'.\n",new_file_name);
      return;
    }

  /* On vérifie si ce n'est pas le même nom */
  if(!strcmp(current_entry->file_name_case,new_file_name))
    return;

  /* Nom en majuscule */
  strcpy(upper_case,new_file_name);
  for(i=0; i<(int)strlen(upper_case); i++)
    upper_case[i] = toupper(upper_case[i]);

  /* 16 bit décrivant la case */
  name_case = BuildProdosCase(new_file_name);

  /* Longueur du nom */
  name_length = (unsigned char) strlen(new_file_name);

  /* Storage Type + Name length */
  storage_length = (current_entry->storage_type << 4) | name_length;

  /* Date actuelle */
  GetCurrentDate(&now_date,&now_time);

  /*****************************************************/
  /** On va modifier le nom dans la structure mémoire **/
  current_entry->name_length = (int) name_length;
  strcpy(current_entry->file_name,upper_case);
  strcpy(current_entry->file_name_case,new_file_name);
  current_entry->lowercase = name_case;

  /***********************************/
  /** On va modifier l'image disque **/
  /* Directory Block */
  GetBlockData(current_image,current_entry->block_location,&directory_block[0]);

  /** Place les informations dans le Directory **/
  memcpy(&directory_block[current_entry->entry_offset+FILE_STORAGETYPE_OFFSET],&storage_length,sizeof(BYTE));
  memcpy(&directory_block[current_entry->entry_offset+FILE_NAME_OFFSET],upper_case,strlen(upper_case));
  memcpy(&directory_block[current_entry->entry_offset+FILE_LOWERCASE_OFFSET],&name_case,sizeof(WORD));

  /* Modifie le block Directory */
  SetBlockData(current_image,current_entry->block_location,&directory_block[0]);

  /** Ecrit le fichier **/
  error = UpdateProdosImage(current_image);
}


/**********************************************************/
/*  RenameProdosFolder() :  Renomage d'un dossier Prodos. */
/**********************************************************/
void RenameProdosFolder(struct prodos_image *current_image, char *prodos_folder_path, char *new_folder_name)
{
  char upper_case[256];
  WORD name_case, now_date, now_time;
  BYTE storage_length;
  int i, is_valid, error;
  unsigned char name_length;
  struct file_descriptive_entry *current_entry;
  unsigned char directory_block[BLOCK_SIZE];

  /* Recherche le dossier Prodos */
  current_entry = GetProdosFolder(current_image,prodos_folder_path,0);
  if(current_entry == NULL)
    {
      printf("  Error : Invalid Prodos Folder path '%s'.\n",prodos_folder_path);
      return;
    }

  /* Vérification du nouveau nom */
  is_valid = CheckProdosName(new_folder_name);
  if(is_valid == 0)
    {
      printf("  Error : Invalid Prodos name '%s'.\n",new_folder_name);
      return;
    }

  /* On vérifie si ce n'est pas le même nom */
  if(!strcmp(current_entry->file_name_case,new_folder_name))
    return;

  /* Nom en majuscule */
  strcpy(upper_case,new_folder_name);
  for(i=0; i<(int)strlen(upper_case); i++)
    upper_case[i] = toupper(upper_case[i]);

  /* 16 bit décrivant la case */
  name_case = BuildProdosCase(new_folder_name);

  /* Longueur du nom */
  name_length = (unsigned char) strlen(new_folder_name);

  /* Storage Type + Name length */
  storage_length = (current_entry->storage_type << 4) | name_length;

  /* Date actuelle */
  GetCurrentDate(&now_date,&now_time);

  /*****************************************************/
  /** On va modifier le nom dans la structure mémoire **/
  current_entry->name_length = (int) name_length;
  strcpy(current_entry->file_name,upper_case);
  strcpy(current_entry->file_name_case,new_folder_name);
  current_entry->lowercase = name_case;

  /***********************************/
  /** On va modifier l'image disque **/
  /** Directory Block **/
  GetBlockData(current_image,current_entry->block_location,&directory_block[0]);

  /** Place les informations dans le Directory **/
  memcpy(&directory_block[current_entry->entry_offset+FILE_STORAGETYPE_OFFSET],&storage_length,sizeof(BYTE));
  memcpy(&directory_block[current_entry->entry_offset+FILE_NAME_OFFSET],upper_case,strlen(upper_case));
  memcpy(&directory_block[current_entry->entry_offset+FILE_LOWERCASE_OFFSET],&name_case,sizeof(WORD));

  /* Modifie le block Directory */
  SetBlockData(current_image,current_entry->block_location,&directory_block[0]);

  /** Sub-Directory Block **/
  GetBlockData(current_image,current_entry->key_pointer_block,&directory_block[0]);

  /** Place les informations dans le Sub-Directory **/
  memcpy(&directory_block[DIRECTORY_STORAGETYPE_OFFSET],&storage_length,sizeof(BYTE));
  memcpy(&directory_block[DIRECTORY_NAME_OFFSET],upper_case,strlen(upper_case));
  memcpy(&directory_block[DIRECTORY_LOWERCASE_OFFSET],&name_case,sizeof(WORD));

  /* Modifie le block Sub-Directory */
  SetBlockData(current_image,current_entry->key_pointer_block,&directory_block[0]);

  /** Ecrit le fichier **/
  error = UpdateProdosImage(current_image);
}


/*******************************************************/
/*  RenameProdosVolume() :  Renomage du volume Prodos. */
/*******************************************************/
void RenameProdosVolume(struct prodos_image *current_image, char *new_volume_name)
{
  char upper_case[256];
  WORD name_case, now_date, now_time;
  BYTE storage_length;
  int i, is_valid, error;
  unsigned char name_length;
  unsigned char volume_block[BLOCK_SIZE];

  /* Vérification du nouveau nom */
  is_valid = CheckProdosName(new_volume_name);
  if(is_valid == 0)
    {
      printf("  Error : Invalid Prodos name '%s'.\n",new_volume_name);
      return;
    }

  /* On vérifie si ce n'est pas le même nom */
  if(!strcmp(current_image->volume_header->volume_name_case,new_volume_name))
    return;

  /* Nom en majuscule */
  strcpy(upper_case,new_volume_name);
  for(i=0; i<(int)strlen(upper_case); i++)
    upper_case[i] = toupper(upper_case[i]);

  /* 16 bit décrivant la case */
  name_case = BuildProdosCase(new_volume_name);

  /* Longueur du nom */
  name_length = (unsigned char) strlen(new_volume_name);

  /* Storage Type + Name length */
  storage_length = (current_image->volume_header->storage_type << 4) | name_length;

  /* Date actuelle */
  GetCurrentDate(&now_date,&now_time);

  /*****************************************************/
  /** On va modifier le nom dans la structure mémoire **/
  current_image->volume_header->name_length = (int) name_length;
  strcpy(current_image->volume_header->volume_name,upper_case);
  strcpy(current_image->volume_header->volume_name_case,new_volume_name);
  current_image->volume_header->lowercase = name_case;
  GetProdosDate(now_date,&current_image->volume_header->volume_modification_date);
  GetProdosTime(now_time,&current_image->volume_header->volume_modification_time);

  /***********************************/
  /** On va modifier l'image disque **/
  /* Volume Block */
  GetBlockData(current_image,2,&volume_block[0]);

  /** Place les informations dans le Volume Header **/
  memcpy(&volume_block[VOLUME_STORAGETYPE_OFFSET],&storage_length,sizeof(BYTE));
  memcpy(&volume_block[VOLUME_NAME_OFFSET],upper_case,strlen(upper_case));
  memcpy(&volume_block[VOLUME_DATEMODIF_OFFSET],&now_date,sizeof(WORD));
  memcpy(&volume_block[VOLUME_TIMEMODIF_OFFSET],&now_time,sizeof(WORD));
  memcpy(&volume_block[VOLUME_LOWERCASE_OFFSET],&name_case,sizeof(WORD));

  /* Modifie le block */
  SetBlockData(current_image,2,&volume_block[0]);

  /** Ecrit le fichier **/
  error = UpdateProdosImage(current_image);
}

/***********************************************************************/
