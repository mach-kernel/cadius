/**********************************************************************/
/*                                                                    */
/*  Prodos_Create.c : Module pour la gestion des commandes CREATE.    */
/*                                                                    */
/**********************************************************************/
/*  Auteur : Olivier ZARDINI  *  Brutal Deluxe Software  *  Mar 2012  */
/**********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <malloc.h>

#include "Dc_Shared.h"
#include "Dc_Memory.h"
#include "Dc_Prodos.h"
#include "Dc_OS.h"
#include "Prodos_Create.h"

unsigned char block_zero[BLOCK_SIZE] =
{
  0x01,0x38,0xb0,0x03,0x4c,0x1c,0x09,0x78,0x86,0x43,0xc9,0x03,0x08,0x8a,0x29,0x70,0x4a,0x4a,0x4a,0x4a,0x09,0xc0,0x85,0x49,0xa0,0xff,0x84,0x48,0x28,0xc8,0xb1,0x48,
  0xd0,0x3a,0xb0,0x0e,0xa9,0x03,0x8d,0x00,0x08,0xe6,0x3d,0xa5,0x49,0x48,0xa9,0x5b,0x48,0x60,0x85,0x40,0x85,0x48,0xa0,0x5e,0xb1,0x48,0x99,0x94,0x09,0xc8,0xc0,0xeb,
  0xd0,0xf6,0xa2,0x06,0xbc,0x32,0x09,0xbd,0x39,0x09,0x99,0xf2,0x09,0xbd,0x40,0x09,0x9d,0x7f,0x0a,0xca,0x10,0xee,0xa9,0x09,0x85,0x49,0xa9,0x86,0xa0,0x00,0xc9,0xf9,
  0xb0,0x2f,0x85,0x48,0x84,0x60,0x84,0x4a,0x84,0x4c,0x84,0x4e,0x84,0x47,0xc8,0x84,0x42,0xc8,0x84,0x46,0xa9,0x0c,0x85,0x61,0x85,0x4b,0x20,0x27,0x09,0xb0,0x66,0xe6,
  0x61,0xe6,0x61,0xe6,0x46,0xa5,0x46,0xc9,0x06,0x90,0xef,0xad,0x00,0x0c,0x0d,0x01,0x0c,0xd0,0x52,0xa9,0x04,0xd0,0x02,0xa5,0x4a,0x18,0x6d,0x23,0x0c,0xa8,0x90,0x0d,
  0xe6,0x4b,0xa5,0x4b,0x4a,0xb0,0x06,0xc9,0x0a,0xf0,0x71,0xa0,0x04,0x84,0x4a,0xad,0x20,0x09,0x29,0x0f,0xa8,0xb1,0x4a,0xd9,0x20,0x09,0xd0,0xdb,0x88,0x10,0xf6,0xa0,
  0x16,0xb1,0x4a,0x4a,0x6d,0x1f,0x09,0x8d,0x1f,0x09,0xa0,0x11,0xb1,0x4a,0x85,0x46,0xc8,0xb1,0x4a,0x85,0x47,0xa9,0x00,0x85,0x4a,0xa0,0x1e,0x84,0x4b,0x84,0x61,0xc8,
  0x84,0x4d,0x20,0x27,0x09,0xb0,0x35,0xe6,0x61,0xe6,0x61,0xa4,0x4e,0xe6,0x4e,0xb1,0x4a,0x85,0x46,0xb1,0x4c,0x85,0x47,0x11,0x4a,0xd0,0x18,0xa2,0x01,0xa9,0x00,0xa8,
  0x91,0x60,0xc8,0xd0,0xfb,0xe6,0x61,0xea,0xea,0xca,0x10,0xf4,0xce,0x1f,0x09,0xf0,0x07,0xd0,0xd8,0xce,0x1f,0x09,0xd0,0xca,0x58,0x4c,0x00,0x20,0x4c,0x47,0x09,0x02,
  0x26,0x50,0x52,0x4f,0x44,0x4f,0x53,0xa5,0x60,0x85,0x44,0xa5,0x61,0x85,0x45,0x6c,0x48,0x00,0x08,0x1e,0x24,0x3f,0x45,0x47,0x76,0xf4,0xd7,0xd1,0xb6,0x4b,0xb4,0xac,
  0xa6,0x2b,0x18,0x60,0x4c,0xbc,0x09,0x20,0x58,0xfc,0xa0,0x14,0xb9,0x58,0x09,0x99,0xb1,0x05,0x88,0x10,0xf7,0x4c,0x55,0x09,0xd5,0xce,0xc1,0xc2,0xcc,0xc5,0xa0,0xd4,
  0xcf,0xa0,0xcc,0xcf,0xc1,0xc4,0xa0,0xd0,0xd2,0xcf,0xc4,0xcf,0xd3,0xa5,0x53,0x29,0x03,0x2a,0x05,0x2b,0xaa,0xbd,0x80,0xc0,0xa9,0x2c,0xa2,0x11,0xca,0xd0,0xfd,0xe9,
  0x01,0xd0,0xf7,0xa6,0x2b,0x60,0xa5,0x46,0x29,0x07,0xc9,0x04,0x29,0x03,0x08,0x0a,0x28,0x2a,0x85,0x3d,0xa5,0x47,0x4a,0xa5,0x46,0x6a,0x4a,0x4a,0x85,0x41,0x0a,0x85,
  0x51,0xa5,0x45,0x85,0x27,0xa6,0x2b,0xbd,0x89,0xc0,0x20,0xbc,0x09,0xe6,0x27,0xe6,0x3d,0xe6,0x3d,0xb0,0x03,0x20,0xbc,0x09,0xbc,0x88,0xc0,0x60,0xa5,0x40,0x0a,0x85,
  0x53,0xa9,0x00,0x85,0x54,0xa5,0x53,0x85,0x50,0x38,0xe5,0x51,0xf0,0x14,0xb0,0x04,0xe6,0x53,0x90,0x02,0xc6,0x53,0x38,0x20,0x6d,0x09,0xa5,0x50,0x18,0x20,0x6f,0x09,
  0xd0,0xe3,0xa0,0x7f,0x84,0x52,0x08,0x28,0x38,0xc6,0x52,0xf0,0xce,0x18,0x08,0x88,0xf0,0xf5,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

unsigned char img_header[IMG_HEADER_SIZE] =
{
  0x32,0x49,0x4D,0x47,0x3E,0x42,0x44,0x3C,0x40,0x00,0x01,0x00,0x01,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x40,0x06,0x00,0x00,0x40,0x00,0x00,0x00,0x00,0x80,0xC0,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

unsigned char bitmap_mask[8] = 
{
  0x7F,0xBF,0xDF,0xEF,0xF7,0xFB,0xFD,0xFE
};


/***********************************************************/
/*  CreateProdosVolume() :  Création d'un nouveau Dossier. */
/***********************************************************/
void CreateProdosFolder(struct prodos_image *current_image, char *prodos_folder_path)
{
  int error, is_volume_header;
  struct file_descriptive_entry *new_folder;

  /** Création du Dossier **/
  new_folder = BuildProdosFolderPath(current_image,prodos_folder_path,&is_volume_header,0);
  if(new_folder == NULL)
    return;

  /** Ecrit le fichier Image **/
  error = UpdateProdosImage(current_image);
}


/********************************************************************/
/*  CreateProdosVolume() :  Création d'une image Prodos 2mg/hdv/po. */
/********************************************************************/
struct prodos_image *CreateProdosVolume(char *image_file_path, char *volume_name, int volume_size_kb)
{
  int i, error, is_valid, nb_image_block, nb_bitmap_block, image_format, image_header_size;
  DWORD nb_block, nb_byte;
  WORD prev_block, next_block, word_value;
  WORD name_case, now_date, now_time;
  BYTE storage_length;
  char upper_case[256];
  unsigned char *image_data;
  struct prodos_image *current_image;

  /** Type d'image **/
  image_format = IMAGE_UNKNOWN;
  for(i=strlen(image_file_path); i>=0; i--)
    if(image_file_path[i] == '.')
      {
        if(!my_stricmp(&image_file_path[i],".2MG"))
          {
            image_format = IMAGE_2MG;
            image_header_size = IMG_HEADER_SIZE;
          }
        else if(!my_stricmp(&image_file_path[i],".HDV"))
          {
            image_format = IMAGE_HDV;
            image_header_size = HDV_HEADER_SIZE;
          }
        else if(!my_stricmp(&image_file_path[i],".PO"))
          {
            image_format = IMAGE_PO;
            image_header_size = PO_HEADER_SIZE;
          }
        break;
      }
  if(image_format == IMAGE_UNKNOWN)
    {
      printf("  Error, Unknown image file format : '%s'.\n",image_file_path);
      return(NULL);
    }

  /* Vérification du nouveau nom */
  is_valid = CheckProdosName(volume_name);
  if(is_valid == 0)
    {
      printf("  Error : Invalid Prodos name '%s'.\n",volume_name);
      return(NULL);
    }

  /* Nom en majuscule */
  strcpy(upper_case,volume_name);
  for(i=0; i<(int)strlen(upper_case); i++)
    upper_case[i] = toupper(upper_case[i]);

  /* 16 bit décrivant la case */
  name_case = BuildProdosCase(volume_name);

  /* Date actuelle */
  GetCurrentDate(&now_date,&now_time);

  /* Nombre de block de l'image */
  nb_image_block = 2*volume_size_kb;

  /* Allocation mémoire */
  image_data = (unsigned char *) calloc(1,1024*volume_size_kb + image_header_size);
  if(image_data == NULL)
    {
      printf("  Error : Impossible to allocate memory.\n");
      return(NULL);
    }

  /*****************************************/
  /*** Remplissage du contenu de l'image ***/
  if(image_format == IMAGE_2MG)
    {
      /** 2mg header **/
      memcpy(&image_data[0],img_header,IMG_HEADER_SIZE);  

      /* Number of Block */
      nb_block = (DWORD) nb_image_block;
      SetDWordValue(image_data,0x14,nb_block);

      /* Number of Byte */
      nb_byte = 1024*volume_size_kb;
      SetDWordValue(image_data,0x1C,nb_byte);
    }
    
  /** Block 0 : Boot **/
  memcpy(&image_data[image_header_size],block_zero,BLOCK_SIZE);

  /** Block 2 : Volume Header **/
  /* Previous / Next block number */
  prev_block = 0;
  next_block = 3;
  SetWordValue(image_data,image_header_size+2*BLOCK_SIZE,prev_block);
  SetWordValue(image_data,image_header_size+2*BLOCK_SIZE+0x02,next_block);

  /* Name Length / Storage Type */
  storage_length = 0xF0 | (unsigned char) strlen(volume_name);
  image_data[image_header_size+2*BLOCK_SIZE+0x04] = storage_length;

  /* Volume Name */
  memcpy(&image_data[image_header_size+2*BLOCK_SIZE+0x05],upper_case,strlen(upper_case));

  /* Modification Date */
  SetWordValue(image_data,image_header_size+2*BLOCK_SIZE+0x16,now_date);
  SetWordValue(image_data,image_header_size+2*BLOCK_SIZE+0x18,now_time);

  /* Lower case */
  SetWordValue(image_data,image_header_size+2*BLOCK_SIZE+0x1A,name_case);

  /* Creation Date */
  SetWordValue(image_data,image_header_size+2*BLOCK_SIZE+0x1C,now_date);
  SetWordValue(image_data,image_header_size+2*BLOCK_SIZE+0x1E,now_time);

  /* Version */
  image_data[image_header_size+2*BLOCK_SIZE+0x20] = 0x05;

  /* Min Version */
  image_data[image_header_size+2*BLOCK_SIZE+0x21] = 0x00;

  /* Access */
  image_data[image_header_size+2*BLOCK_SIZE+0x22] = 0xC3;

  /* Entry Length */
  image_data[image_header_size+2*BLOCK_SIZE+0x23] = 0x27;

  /* Entries per block */
  image_data[image_header_size+2*BLOCK_SIZE+0x24] = 0x0D;

  /* File Count */
  word_value = 0;
  SetWordValue(image_data,image_header_size+2*BLOCK_SIZE+0x25,word_value);

  /* Bitmap block */
  word_value = 0x0006;
  SetWordValue(image_data,image_header_size+2*BLOCK_SIZE+0x27,word_value);

  /* Total Blocks (on ne dépasse pas 65535 !) */
  word_value = (WORD) (nb_image_block == 65536 ? 65535 : nb_image_block);
  SetWordValue(image_data,image_header_size+2*BLOCK_SIZE+0x29,word_value);

  /** Block 3-5 : Volume Directory **/
  /* Previous / Next block number */
  prev_block = 2;
  next_block = 4;
  SetWordValue(image_data,image_header_size+3*BLOCK_SIZE,prev_block);
  SetWordValue(image_data,image_header_size+3*BLOCK_SIZE+0x02,next_block);
  prev_block = 3;
  next_block = 5;
  SetWordValue(image_data,image_header_size+4*BLOCK_SIZE,prev_block);
  SetWordValue(image_data,image_header_size+4*BLOCK_SIZE+0x02,next_block);
  prev_block = 4;
  next_block = 0;
  SetWordValue(image_data,image_header_size+5*BLOCK_SIZE,prev_block);
  SetWordValue(image_data,image_header_size+5*BLOCK_SIZE+0x02,next_block);

  /** Block 6+ : Bitmap **/
  /* Nombre de blocs nécessaires pour stocker la table */
  nb_bitmap_block = GetContainerNumber(nb_image_block,BLOCK_SIZE*8);

  /* Indique les zones libres : Bloc 6+nb_bitmap_block+1 -> Nb Bloc */
  for(i=6+nb_bitmap_block; i<nb_image_block; i++)
    image_data[image_header_size+6*BLOCK_SIZE+i/8] |= (0x01<<(7-i%8));

  /** Création du fichier sur disque **/
  error = CreateBinaryFile(image_file_path,image_data,1024*volume_size_kb + image_header_size);
  if(error)
    {
      free(image_data);
      printf("  Error : Impossible to create file '%s' on disk.\n",image_file_path);
      return(NULL);
    }

  /* Libération mmoire */
  free(image_data);

  /** Chargement de l'image **/
  current_image = LoadProdosImage(image_file_path);

  return(current_image);
}


/********************************************************************************************/
/*  BuildProdosFolderPath() :  Création des dossiers nécessaires pour y ajouter le fichier. */
/********************************************************************************************/
struct file_descriptive_entry *BuildProdosFolderPath(struct prodos_image *current_image, char *target_folder_path_param, int *is_volume_header_rtn, int verbose)
{
  int first_one, name_length;
  char *begin;
  char *next_sep;
  char volume_name[256];
  char folder_path[2048];
  char target_folder_path[2048];
  struct file_descriptive_entry *current_folder;
  struct file_descriptive_entry *next_folder;
  struct file_descriptive_entry *new_folder;

  /* Init */
  *is_volume_header_rtn = 0;
  first_one = 1;
  current_folder = NULL;
  next_folder = NULL;

  /* Nettoyage du chemin */
  if(target_folder_path_param[0] == '/')
    strcpy(target_folder_path,target_folder_path_param);
  else
    sprintf(target_folder_path,"/%s",target_folder_path_param);
  if(target_folder_path[strlen(target_folder_path)-1] != '/')
    strcat(target_folder_path,"/");

  /** On vérifie que le nom de volume est correct **/
  next_sep = strchr(target_folder_path+1,'/');
  name_length = next_sep-(target_folder_path+1);
  memcpy(volume_name,target_folder_path+1,name_length);
  volume_name[name_length] = '\0';
  if(my_stricmp(current_image->volume_header->volume_name,volume_name))
    {
      printf("  Error : Invalid Prodos Volume name : %s\n",volume_name);
      return(NULL);
    }

  /** On veut placer le fichier à la racine du volume **/
  if(strlen(current_image->volume_header->volume_name) + 2 == strlen(target_folder_path))
    {
      *is_volume_header_rtn = 1;
      return(NULL);
    }

  /** On va rechercher le dossier valide dans l'arborescence **/
  next_sep = strchr(target_folder_path+1,'/');
  while(next_sep)
    {
      /* Isole le chemin */
      memcpy(folder_path,target_folder_path,next_sep-target_folder_path+1);
      folder_path[next_sep-target_folder_path+1] = '\0';

      /* Le premier, c'est le Volume Name, on passe */
      if(first_one == 1)
        {
          first_one = 0;
          next_sep = strchr(next_sep+1,'/');
          continue;
        }

      /* Vérifie ce chemin */
      next_folder = GetProdosFolder(current_image,folder_path,0);
      if(next_folder == NULL)
        break;

      /* Suivant */
      current_folder = next_folder;
      next_sep = strchr(next_sep+1,'/');
    }

  /** Création de tous les Dossiers **/
  strcpy(folder_path,&target_folder_path[(current_folder == NULL) ? strlen(current_image->volume_header->volume_name)+2: strlen(current_folder->file_path)+1]);
  begin = folder_path;
  while(begin)
    {
      /* Isole le nom du répertoire */
      next_sep = strchr(begin,'/');
      if(next_sep)
        *next_sep = '\0';

      /* Fini */
      if(strlen(begin) == 0)
        break;

      /* Création d'un répertoire vide dans le Dossier ou à la Racine du volume */
      new_folder = CreateOneProdosFolder(current_image,current_folder,begin,verbose);
      if(new_folder == NULL)
        return(NULL);

      /* Répertoire suivant */
      current_folder = new_folder;
      begin = (next_sep == NULL) ? NULL : next_sep+1;
    }

  /** Tableaux de pointeurs **/
  my_Memory(MEMORY_BUILD_DIRECTORY_TAB,NULL,NULL);

  /* Renvoi le dernier répertoire */
  return(current_folder);
}


/******************************************************************************************************/
/*  CreateOneProdosFolder() :  Création d'un dossier vide dans un Directory ou à la Racine du volume. */
/******************************************************************************************************/
struct file_descriptive_entry *CreateOneProdosFolder(struct prodos_image *current_image, struct file_descriptive_entry *current_folder, char *folder_name, int verbose)
{
  char upper_case[256];
  WORD name_case, now_date, now_time, file_count;
  WORD prev_block_number, next_block_number, directory_block_number, subdirectory_block_number, header_block_number;
  BYTE storage_length, directory_entry_number;
  int i, is_valid, error, offset, entry_length, nb_directory;
  int *tab_block;
  unsigned char name_length;
  struct file_descriptive_entry *new_folder = NULL;
  struct file_descriptive_entry **new_tab_directory = NULL;
  char volume_path[256];
  unsigned char directory_block[BLOCK_SIZE];
  unsigned char subdirectory_block[BLOCK_SIZE];

  /* Vérifie le nom du Dossier */
  is_valid = CheckProdosName(folder_name);
  if(is_valid == 0)
    {
      printf("  Error : Invalid Prodos Folder name '%s'.\n",folder_name);
      return(NULL);
    }

  /** Vérifie que ce nom de Dossier ne correspond pas déjà à un nom de fichier **/
  if(current_folder == NULL)
    {
      /* Vérification à la racine du Volume */
      for(i=0; i<current_image->nb_file; i++)
        if(!my_stricmp(folder_name,current_image->tab_file[i]->file_name))
          {
            printf("  Error : Invalid Prodos Folder name. The name is already used by a File '%s'.\n",folder_name);
            return(NULL);
          }
    }
  else
    {
      /* Vérification dans le dossier */
      for(i=0; i<current_folder->nb_file; i++)
        if(!my_stricmp(folder_name,current_folder->tab_file[i]->file_name))
          {
            printf("  Error : Invalid Prodos Folder name. The name is already used by a File '%s'.\n",folder_name);
            return(NULL);
          }
    }

  /* Nom en majuscule */
  strcpy(upper_case,folder_name);
  for(i=0; i<(int)strlen(upper_case); i++)
    upper_case[i] = toupper(upper_case[i]);

  /* 16 bit décrivant la case */
  name_case = BuildProdosCase(folder_name);

  /* Longueur du nom */
  name_length = (unsigned char) strlen(folder_name);

  /* Date actuelle */
  GetCurrentDate(&now_date,&now_time);

  /* Entry Length */
  entry_length = 0x27;

  /*****************************************************/
  /** Recherche d'une entrée libre dans le répertoire **/
  error = AllocateFolderEntry(current_image,current_folder,&directory_block_number,&directory_entry_number,&header_block_number);
  if(error)
    return(NULL);

  /*************************************/
  /** Création d'un SubDirectory Bloc **/
  /* Init */
  memset(subdirectory_block,0,BLOCK_SIZE);

  /* Previous Block / Next Block */
  prev_block_number = 0;
  next_block_number = 0;
  SetWordValue(subdirectory_block,0x00,prev_block_number);
  SetWordValue(subdirectory_block,0x02,next_block_number);

  /* Storage Type / Name Length */
  storage_length = 0xE0 | name_length;
  subdirectory_block[DIRECTORY_STORAGETYPE_OFFSET] = storage_length;

  /* SubDir Name */
  memcpy(&subdirectory_block[DIRECTORY_NAME_OFFSET],upper_case,strlen(upper_case));

  /* Constant */
  subdirectory_block[0x14] = 0x76;

  /* Creation Date */
  SetWordValue(subdirectory_block,0x1C,now_date);
  SetWordValue(subdirectory_block,0x1E,now_time);

  /* Lower case (Version + Minimum Version)*/
  SetWordValue(subdirectory_block,DIRECTORY_LOWERCASE_OFFSET,name_case);

  /* Access */
  subdirectory_block[0x22] = 0xC3;

  /* Entry Length */
  subdirectory_block[DIRECTORY_ENTRYLENGTH_OFFSET] = 0x27;

  /* Entry per Block */
  subdirectory_block[DIRECTORY_ENTRIESPERBLOCK_OFFSET] = 0x0D;

  /* File Count */
  SetWordValue(subdirectory_block,DIRECTORY_FILECOUNT_OFFSET,0);

  /* Parent Block */
  SetWordValue(subdirectory_block,DIRECTORY_PARENTPOINTERBLOCK_OFFSET,directory_block_number);

  /* Parent Entry */
  subdirectory_block[0x29] = directory_entry_number;   /* 1->N */

  /* Parent Entry Length */
  subdirectory_block[0x2A] = 0x27;

  /*******************************************/
  /*** Stockage sur disque du subDirectory ***/
  /** Allocation d'un block pour le SubDirectory **/
  tab_block = AllocateImageBlock(current_image,1);
  if(tab_block == NULL)
    return(NULL);
  subdirectory_block_number = tab_block[0];
  free(tab_block);

  /** Ecriture du bloc **/
  SetBlockData(current_image,subdirectory_block_number,&subdirectory_block[0]);

  /***********************************************************/
  /*** Modification du Directory ou de la Racine du volume ***/
  /* Lecture : Block contenant l'entrée */
  GetBlockData(current_image,directory_block_number,&directory_block[0]);

  /** Création de l'entrée SubDirectory **/
  offset = 4 + (directory_entry_number-1)*entry_length;

  /* Storage Type / Name length */
  storage_length = 0xD0 | name_length;
  directory_block[offset+0x00] = storage_length;

  /* File Name */
  memcpy(&directory_block[offset+0x01],upper_case,strlen(upper_case));

  /* File Type */
  directory_block[offset+0x10] = 0x0F;

  /* Key Pointer */
  SetWordValue(directory_block,offset+0x11,subdirectory_block_number);

  /* Block Used */
  SetWordValue(directory_block,offset+0x13,1);

  /* EOF */
  Set24bitValue(directory_block,offset+0x15,BLOCK_SIZE);   /* Taille d'un dossier vide = 1 Bloc */

  /* Creation */
  SetWordValue(directory_block,offset+0x18,now_date);
  SetWordValue(directory_block,offset+0x1A,now_time);

  /* Lower case (Version + Minimum Version)*/
  SetWordValue(directory_block,offset+0x1C,name_case);

  /* Access */
  directory_block[offset+0x1E] = 0xE3;

  /* Aux Type */
  SetWordValue(directory_block,offset+0x1F,0);

  /* Last Modification */
  SetWordValue(directory_block,offset+0x21,now_date);
  SetWordValue(directory_block,offset+0x23,now_time);

  /* Header Pointer */
  SetWordValue(directory_block,offset+0x25,(WORD)((current_folder == NULL) ? 2 : current_folder->key_pointer_block));

  /* Ecriture : Block contenant l'entrée */
  SetBlockData(current_image,directory_block_number,&directory_block[0]);

  /*********************************************************/
  /***  Allocation Mémoire pour ce nouveau SubDirectory  ***/
  /** Lecture de l'entrée **/
  sprintf(volume_path,"/%s",current_image->volume_header->volume_name_case);
  new_folder = ODSReadFileDescriptiveEntry(current_image,(current_folder==NULL)?volume_path:current_folder->file_path,&directory_block[offset]);
  if(new_folder == NULL)
    return(NULL);
  /* Complète l'entrée */
  new_folder->depth = (current_folder==NULL) ? 1 : current_folder->depth + 1;   /* ? */
  new_folder->parent_directory = current_folder;
  new_folder->block_location = directory_block_number;   /* ? */
  new_folder->entry_offset = offset;
  new_folder->nb_file = 0;
  new_folder->tab_file = NULL;
  new_folder->nb_directory = 0;
  new_folder->tab_directory = NULL;
  /* Ajoute ce Dossier */
  my_Memory(MEMORY_ADD_DIRECTORY,new_folder,NULL);

  /*** Modification du Directory Header ***/
  /* Lecture : Bloc Header du Directory */
  GetBlockData(current_image,(current_folder == NULL) ? 2 : current_folder->key_pointer_block,&directory_block[0]);

  /* Modification du FileCount (+1) */
  file_count = GetWordValue(directory_block,0x25);
  SetWordValue(directory_block,0x25,(WORD)(file_count+1));

  /* Ecriture Bloc Header du Directory */
  SetBlockData(current_image,(current_folder == NULL) ? 2 : current_folder->key_pointer_block,&directory_block[0]);

  /***************************************************/
  /*** Ajoute ce SubDirectory au Folder en mémoire ***/
  /** Création du nouveau Tableau des Dossiers **/
  nb_directory = 1 + ((current_folder==NULL) ? current_image->nb_directory : current_folder->nb_directory);
  new_tab_directory = (struct file_descriptive_entry **) calloc(nb_directory,sizeof(struct file_descriptive_entry *));
  if(new_tab_directory == NULL)
    {
      printf("  Error : Impossible to allocate memory.\n");
      return(NULL);
    }
  for(i=0; i<nb_directory-1; i++)
    new_tab_directory[i] = (current_folder==NULL) ? current_image->tab_directory[i] : current_folder->tab_directory[i];
  new_tab_directory[nb_directory-1] = new_folder;
  qsort(new_tab_directory,nb_directory,sizeof(struct file_descriptive_entry *),compare_entry);

  /** Met à jour les informations mémoire du Dossier parent **/
  if(current_folder == NULL)
    {
      /* Ajoute ce dossier à la Racine du Volume */
      if(current_image->tab_directory)
        free(current_image->tab_directory);
      current_image->tab_directory = new_tab_directory;
      current_image->nb_directory = nb_directory;
    }
  else
    {
      /* Ajoute ce dossier dans le Dossier parent */
      if(current_folder->tab_directory)
        free(current_folder->tab_directory);
      current_folder->tab_directory = new_tab_directory;
      current_folder->nb_directory = nb_directory;
    }

  /* Stat */
  if(verbose)
    printf("      + Add Folder : %s\n",new_folder->file_path);
  current_image->nb_add_folder++;

  /* Renvoie la nouvelle structure */
  return(new_folder);
}

/***********************************************************************/