/**********************************************************************/
/*                                                                    */
/*  Prodos_Add.c : Module pour la gestion des commandes ADD.          */
/*                                                                    */
/**********************************************************************/
/*  Auteur : Olivier ZARDINI  *  Brutal Deluxe Software  *  Mar 2012  */
/**********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <malloc.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "Dc_Shared.h"
#include "Dc_Memory.h"
#include "Dc_Prodos.h"
#include "Dc_OS.h"
#include "Prodos_Create.h"
#include "Prodos_Add.h"

static struct prodos_file *LoadFile(char *);
static int GetFileInformation(char *,char *,struct prodos_file *);
static void GetLineValue(char *,char *,char *);
static void ComputeFileBlockUsage(struct prodos_file *);
static WORD CreateFileContent(struct prodos_image *,struct prodos_file *);
static void CreateFileEntry(struct prodos_image *,struct prodos_file *,WORD,struct file_descriptive_entry *,WORD,BYTE,WORD);
static int CreateMemoryEntry(struct prodos_image *,struct file_descriptive_entry *,WORD,BYTE);
static WORD CreateSeedlingContent(struct prodos_image *,struct prodos_file *,unsigned char *,int,int,int);
static WORD CreateSaplingContent(struct prodos_image *,struct prodos_file *,unsigned char *,int,int,int);
static WORD CreateTreeContent(struct prodos_image *,struct prodos_file *,unsigned char *,int,int,int,int);

/***************************************************************/
/*  AddFile() :  Ajoute un fichier Windows à l'archive Prodos. */
/***************************************************************/
int AddFile(struct prodos_image *current_image, char *file_path, char *target_folder_path, int update_image)
{
  int i, is_volume_header, error, is_valid;
  WORD file_block_number, directory_block_number, directory_header_pointer;
  BYTE directory_entry_number;
  struct file_descriptive_entry *target_folder;
  struct prodos_file *current_file;

  /** Charge le fichier depuis le disque **/
  current_file = LoadFile(file_path);
  if(current_file == NULL)
    return(1);

  /** On vérifie si ce fichier est compatible Prodos **/
  /* Nom */
  is_valid = CheckProdosName(current_file->file_name);
  if(is_valid == 0)
    {
      printf("  Error : Invalid Prodos File name '%s'.\n",current_file->file_name);
      current_image->nb_add_error++;
      mem_free_file(current_file);
      return(1);
    }
  /* Taille */
  if(current_file->data_length > (16*1024*1024) || current_file->resource_length > (16*1024*1024) || (current_file->data_length+current_file->resource_length) > (16*1024*1024))
    {
      printf("  Error : Invalid Prodos File size '%d' bytes (limit is 16 MB).\n",current_file->data_length+current_file->resource_length);
      current_image->nb_add_error++;
      mem_free_file(current_file);
      return(1);
    } 

  /** Calcule le nombre de block nécessaire pour stocker le fichier (gestion du Sparse) **/
  ComputeFileBlockUsage(current_file);
  if(current_file->tab_data_block == NULL || current_file->tab_resource_block == NULL)
    {
      printf("  Error : Impossible to allocate memory.\n");
      current_image->nb_add_error++;
      mem_free_file(current_file);
      return(1);
    }

  /** Vérifie qu'il reste suffisament de place pour stocker le fichier **/
  if(current_file->entry_disk_block > current_image->nb_free_block)
    {
      printf("  Error : No enough space in the image : '%d' bytes required ('%d' bytes available).\n",BLOCK_SIZE*current_file->entry_disk_block,BLOCK_SIZE*current_image->nb_free_block);
      current_image->nb_add_error++;
      mem_free_file(current_file);
      return(1);
    }

  /** Recherche/Construit le dossier Prodos Cible où placer le fichier **/
  target_folder = BuildProdosFolderPath(current_image,target_folder_path,&is_volume_header,1);
  if(target_folder == NULL && is_volume_header == 0)
    {
      mem_free_file(current_file);
      current_image->nb_add_error++;
      return(1);
    }

  /** Vérifie que le nom de fichier ne correspond pas déjà à un nom de fichier/dossier pris **/
  if(target_folder == NULL)
    {
      /* Vérification à la racine du Volume */
      for(i=0; i<current_image->nb_file; i++)
        if(!my_stricmp(current_file->file_name_case,current_image->tab_file[i]->file_name))
          {
            printf("  Error : Invalid target location. A file already exist with the same name '%s'.\n",current_image->tab_file[i]->file_name);
            current_image->nb_add_error++;
            mem_free_file(current_file);
            return(1);
          }
      for(i=0; i<current_image->nb_directory; i++)
        if(!my_stricmp(current_file->file_name_case,current_image->tab_directory[i]->file_name))
          {
            printf("  Error : Invalid target location. A folder already exist with the same name '%s'.\n",current_image->tab_directory[i]->file_name);
            current_image->nb_add_error++;
            mem_free_file(current_file);
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
            current_image->nb_add_error++;
            mem_free_file(current_file);
            return(1);
          }
      for(i=0; i<target_folder->nb_directory; i++)
        if(!my_stricmp(current_file->file_name_case,target_folder->tab_directory[i]->file_name))
          {
            printf("  Error : Invalid target location. A folder already exist with the same name '%s'.\n",target_folder->tab_directory[i]->file_name);
            current_image->nb_add_error++;
            mem_free_file(current_file);
            return(1);
          }
    }

  /** Recherche d'une entrée libre dans le répertoire **/
  error = AllocateFolderEntry(current_image,target_folder,&directory_block_number,&directory_entry_number,&directory_header_pointer);
  if(error)
    {
      mem_free_file(current_file);
      current_image->nb_add_error++;
      return(1);
    }

  /** Vérifie qu'il reste suffisament de place pour stocker le fichier (la réservation du nom dans le directory a peut être consommé 1 block **/
  if(current_file->entry_disk_block > current_image->nb_free_block)
    {
      printf("  Error : No enough space in the image : '%d' bytes required ('%d' bytes available).\n",BLOCK_SIZE*current_file->entry_disk_block,BLOCK_SIZE*current_image->nb_free_block);
      current_image->nb_add_error++;
      mem_free_file(current_file);
      return(1);
    }

  /*** Création du contenu fichier (index+data+resource) ***/
  file_block_number = CreateFileContent(current_image,current_file);  
  if(file_block_number == 0)
    {
      current_image->nb_add_error++;
      mem_free_file(current_file);
      return(1);
    }

  /** Remplissage de l'entrée du fichier dans le répertoire **/
  CreateFileEntry(current_image,current_file,file_block_number,target_folder,directory_block_number,directory_entry_number,directory_header_pointer);

  /** Création de l'entrée en mémoire **/
  error = CreateMemoryEntry(current_image,target_folder,directory_block_number,directory_entry_number);
  if(error)
    {
      current_image->nb_add_error++;
      printf("  Error : Impossible to allocate memory.\n");
      mem_free_file(current_file);
      return(1);
    }

  /* Libération mémoire */
  mem_free_file(current_file);
    
  /** Ecrit le fichier Image **/
  if(update_image)
    error = UpdateProdosImage(current_image);

  /* OK */
  current_image->nb_add_file++;
  return(0);
}


/********************************************************************************/
/*  AddFolder() :  Ajoute les fichiers Windows d'un dossier à l'archive Prodos. */
/********************************************************************************/
void AddFolder(struct prodos_image *current_image, char *folder_path, char *target_folder_path)
{
  int i, j, error, is_volume_header;
  int nb_file;
  char **tab_file;
  struct file_descriptive_entry *target_folder;  
  char full_folder_path[1024];
  char prodos_folder_path[1024];

  /** Recherche/Construit le dossier Prodos Cible où placer les fichiers **/
  target_folder = BuildProdosFolderPath(current_image,target_folder_path,&is_volume_header,1);
  if(target_folder == NULL && is_volume_header == 0)
    {
      current_image->nb_add_error++;
      return;
    }

  /* Prépare le chemin */
  strcpy(full_folder_path,folder_path);
  if(strlen(full_folder_path) > 0)
    if(full_folder_path[strlen(full_folder_path)-1] != '\\' && full_folder_path[strlen(full_folder_path)-1] != '/')
      strcat(full_folder_path,FOLDER_CHARACTER);
  strcat(full_folder_path,"*");

  /** Récupère la liste des fichiers du répertoire **/
  tab_file = BuildFileList(full_folder_path,&nb_file);
  if(tab_file == NULL || nb_file == 0)
    {
      printf("  Error : Impossible to get files list from location '%s'.\n",folder_path);
      mem_free_list(nb_file,tab_file);
      current_image->nb_add_error++;
      return;
    }

  /** Création des fichiers dans l'image **/
  for(i=0; i<nb_file; i++)
    {
      /* On ne copie pas les fichiers *_ResourceFork.bin */
      if(strlen(tab_file[i]) > strlen("_ResourceFork.bin"))
        if(!my_stricmp(&tab_file[i][strlen(tab_file[i])-strlen("_ResourceFork.bin")],"_ResourceFork.bin"))
          continue;

      /* On ne copie pas les fichiers *_FileInformation.txt */
      if(strlen(tab_file[i]) > strlen("_FileInformation.txt"))
        if(!my_stricmp(&tab_file[i][strlen(tab_file[i])-strlen("_FileInformation.txt")],"_FileInformation.txt"))
          continue;
      
      /** Chemin Prodos du fichier dans l'image **/
      strcpy(prodos_folder_path,target_folder_path);
      if(prodos_folder_path[strlen(prodos_folder_path)-1] != '/')
        strcat(prodos_folder_path,"/");
      strcat(prodos_folder_path,&tab_file[i][strlen(full_folder_path)-strlen("*")]);
      /* Conversion des \ en / */
      for(j=0; j<(int)strlen(prodos_folder_path); j++)
        if(prodos_folder_path[j] == '\\')
          prodos_folder_path[j] = '/';

      /* Information */
      printf("      o Add File   : %s\n",prodos_folder_path);

      /* Supprime le nom du fichier final */
      for(j=(int)strlen(prodos_folder_path); j>=0; j--)
        if(prodos_folder_path[j] == '/')
          {
            prodos_folder_path[j+1] = '\0';
            break;
          }
      
      /** Ajoute ce fichier à l'archive **/
      error = AddFile(current_image,tab_file[i],prodos_folder_path,0);
    }

  /* Libération table des fichiers */
  mem_free_list(nb_file,tab_file);

  /** Ecrit le fichier Image **/
  error = UpdateProdosImage(current_image);  
}


/********************************************************************/
/*  LoadFile() :  Charge un fichier depuis le disque de la machine. */
/********************************************************************/
static struct prodos_file *LoadFile(char *file_path_data)
{
  int i, found;
  struct prodos_file *current_file;
  char file_name[1024];
  char folder_path[2048];
  char file_path[2048];

  /* Allocation mémoire */
  current_file = (struct prodos_file *) calloc(1,sizeof(struct prodos_file));
  if(current_file == NULL)
    {
      printf("  Error : Impossible to allocate memory.\n");
      return(NULL);
    }
  
  /* Extrait le répertoire du nom de fichier */
  strcpy(folder_path,file_path_data);
  for(i=strlen(folder_path); i>=0; i--)
    if(folder_path[i] == '\\' || folder_path[i] == '/')
      {
        folder_path[i+1] = '\0';
        break;
      }

  /** Extrait le nom de fichier **/
  strcpy(file_name,file_path_data);
  for(i=strlen(file_path_data); i>=0; i--)
    if(file_path_data[i] == '\\' || file_path_data[i] == '/')
      {
        strcpy(file_name,&file_path_data[i+1]);
        break;
      }
  current_file->file_name = strdup(file_name);
  current_file->file_name_case = strdup(file_name);  
  if(current_file->file_name == NULL || current_file->file_name_case == NULL)
    {
      free(current_file);
      printf("  Error : Impossible to allocate memory.\n");
      return(NULL);
    }
  for(i=0; i<(int)strlen(current_file->file_name); i++)
    current_file->file_name[i] = toupper(current_file->file_name[i]);
  /* Proper Case */
  current_file->name_case = BuildProdosCase(current_file->file_name_case);
    
  /*** Chargement des Data ***/
  current_file->data = LoadBinaryFile(file_path_data,&current_file->data_length);
  if(current_file->data != NULL && current_file->data_length == 0)
    {
      free(current_file->data);
      current_file->data = NULL;
    }

  /*** Chargement des Resources ***/
  sprintf(file_path,"%s_ResourceFork.bin",file_path_data);
  current_file->resource = LoadBinaryFile(file_path,&current_file->resource_length);
  current_file->has_resource = (current_file->resource == NULL) ? 0 : 1;
  if(current_file->resource != NULL && current_file->resource_length == 0)
    {
      free(current_file->resource);
      current_file->resource = NULL;
    }

  /** Chargement des Informations du fichier contenue dans _FileInformation.txt **/
  sprintf(file_path,"%s_FileInformation.txt",folder_path);
  found = GetFileInformation(file_path,file_name,current_file);
  if(!found)
    {
      /* Valeurs par défaut */
      current_file->type = 0x00;
      current_file->aux_type = 0x0000;
      current_file->version_create = 0x00;
      current_file->min_version = 0x00;
      current_file->access = 0xE3;
    }

  /** Récupération des Propriétés Date/Time du fichier **/
  my_GetFileCreationModificationDate(file_path_data,current_file);

  /* Renvoie la structure */
  return(current_file);
}


/********************************************************************/
/*  GetFileInformation() :  Récupère les informations d'un fichier. */ 
/********************************************************************/
static int GetFileInformation(char *file_information_path, char *file_name, struct prodos_file *current_file)
{
  DWORD value;
  char *next_sep;
  int i, j, nb_line;
  char **line_tab;
  char line_file_name[1024];
  char local_buffer[1024];

  /** Charge en mémoire le fichier **/
  line_tab = BuildUniqueListFromFile(file_information_path,&nb_line);
  if(line_tab == NULL)
    return(0);

  /*** Recherche la ligne du fichier ***/
  for(i=0; i<nb_line; i++)
    {
      /* Isole le nom du fichier */
      next_sep = strchr(line_tab[i],'=');
      if(next_sep == NULL)
        continue;

      /* Recherche le fichier actuel */
      memcpy(line_file_name,line_tab[i],next_sep-line_tab[i]);
      line_file_name[next_sep-line_tab[i]] = '\0';

      /** On récupère les informations **/
      if(!my_stricmp(line_file_name,file_name))
        {
          GetLineValue(line_tab[i],"Type",local_buffer);
          if(strlen(local_buffer) == 2)
            {
              sscanf(local_buffer,"%02X",&value);
              current_file->type = (unsigned char) value;
            }
          GetLineValue(line_tab[i],"AuxType",local_buffer);
          if(strlen(local_buffer) == 4)
            {
              sscanf(local_buffer,"%04X",&value);
              current_file->aux_type = (WORD) value;
            }
          GetLineValue(line_tab[i],"VersionCreate",local_buffer);
          if(strlen(local_buffer) == 2)
            {
              sscanf(local_buffer,"%02X",&value);
              current_file->version_create = (unsigned char) value;
            }
          GetLineValue(line_tab[i],"MinVersion",local_buffer);
          if(strlen(local_buffer) == 2)
            {
              sscanf(local_buffer,"%02X",&value);
              current_file->min_version = (unsigned char) value;
            }
          GetLineValue(line_tab[i],"Access",local_buffer);
          if(strlen(local_buffer) == 2)
            {
              sscanf(local_buffer,"%02X",&value);
              current_file->access = (unsigned char) value;
            }
          GetLineValue(line_tab[i],"FolderInfo1",local_buffer);
          if(strlen(local_buffer) == 36)
            for(j=0; j<18; j++)
              {
                sscanf(&local_buffer[2*j],"%02X",&value);
                current_file->resource_finderinfo_1[j] = (unsigned char) value;
              }
          GetLineValue(line_tab[i],"FolderInfo2",local_buffer);
          if(strlen(local_buffer) == 36)
            for(j=0; j<18; j++)
              {
                sscanf(&local_buffer[2*j],"%02X",&value);
                current_file->resource_finderinfo_2[j] = (unsigned char) value;
              }
            
          /* Trouvé */
          mem_free_list(nb_line,line_tab);
          return(1);
        }
    }

  /* Pas trouvé */
  mem_free_list(nb_line,line_tab);
  return(0);
}


/*********************************************************************/
/*  GetLineValue() :  Récupère la valeur d'une variable de la ligne. */
/*********************************************************************/
static void GetLineValue(char *line, char *variable, char *value_rtn)
{
  int i;
  char *next_sep;

  /* Init */
  strcpy(value_rtn,"");
  if(strlen(line) <= strlen(variable))
    return;

  /* Recherche la variable */
  for(i=1; i<(int)(strlen(line)-strlen(variable)); i++)
    if(!my_strnicmp(&line[i],variable,strlen(variable)))
      if((line[i-1] == '=' || line[i-1] == ',') && line[i+strlen(variable)] == '(')
        {
          /* Récupère la valeur */
          strcpy(value_rtn,&line[i+strlen(variable)+1]);
          next_sep = strchr(value_rtn,')');
          if(next_sep)
            *next_sep = '\0';
          else
             strcpy(value_rtn,"");    /* Erreur, il manque la dernière ) */
          return;
        }
}


/********************************************************************************************/
/*  ComputeFileBlockUsage() :  Détermine le nombre de block utiles pour stocker le fichier. */
/********************************************************************************************/
static void ComputeFileBlockUsage(struct prodos_file *current_file)
{
  int i, result;
  unsigned char empty_block[BLOCK_SIZE];
  
  /* Init */
  memset(empty_block,0x00,BLOCK_SIZE);
  current_file->block_disk_data = 0;      /* Nb de blocks disk utilisés pour les data */
  current_file->empty_data = 0;           /* Tout est à zéro */
  current_file->block_disk_resource = 0;  /* Nb de blocks disk utilisés pour les resource */
  current_file->empty_resource = 0;       /* Tout est à zéro */
  
  /** Nombre de block pour les Data **/
  current_file->block_data = GetContainerNumber(current_file->data_length,BLOCK_SIZE);
  for(i=0; i<current_file->data_length; i+=BLOCK_SIZE)
    {
      /* Recherche les plages de 0 */
      if(i+BLOCK_SIZE <= current_file->data_length)
        result = memcmp(&current_file->data[i],empty_block,BLOCK_SIZE);
      else
        result = memcmp(&current_file->data[i],empty_block,current_file->data_length-i);
        
      /* Block à réserver ? */
      current_file->block_disk_data += (result == 0) ? 0 : 1;
    }
  if(current_file->block_disk_data == 0)
    {
      current_file->block_disk_data = 1;      /* Même pour les fichiers vides, on réserve 1 block de Data */
      current_file->empty_data = 1;
    }

  /** Nombre de block pour les Resource **/
  if(current_file->has_resource == 1)
    {
      current_file->block_resource = GetContainerNumber(current_file->resource_length,BLOCK_SIZE);
      for(i=0; i<current_file->resource_length; i+=BLOCK_SIZE)
        {
          /* Recherche les plages de 0 */
          if(i+BLOCK_SIZE <= current_file->resource_length)
            result = memcmp(&current_file->resource[i],empty_block,BLOCK_SIZE);
          else
            result = memcmp(&current_file->resource[i],empty_block,current_file->resource_length-i);
            
          /* Block à réserver ? */
          current_file->block_disk_resource += (result == 0) ? 0 : 1;
        }
      if(current_file->block_disk_resource == 0)
        {
          current_file->block_disk_resource = 1;      /* Même pour les fichiers vides, on réserve 1 block de Resources */
          current_file->empty_resource = 1;
        }
    }
  else
    current_file->block_resource = 0;

  /*** Type de Stockage + Nb Block Index ***/
  if(current_file->has_resource == 0)
    {
      /* Data */
      if(current_file->block_data == 0 || current_file->block_data == 1 || current_file->empty_data == 1)
        {
          current_file->type_data = TYPE_ENTRY_SEEDLING;
          current_file->index_data = 0;
        }
      else if(current_file->block_data < 257)
        {
          current_file->type_data = TYPE_ENTRY_SAPLING;
          current_file->index_data = 1;
        }
      else
        {
          current_file->type_data = TYPE_ENTRY_TREE;
          current_file->index_data = 1 + GetContainerNumber(current_file->block_data,INDEX_PER_BLOCK);
        }

      /* Type Entry = Type Data */
      current_file->entry_type = current_file->type_data;
      current_file->index_resource = 0;

      /* Taille totale en block */
      current_file->entry_disk_block = current_file->index_data + current_file->block_disk_data;
    }
  else
    {
      /* Data */
      if(current_file->block_data == 0 || current_file->block_data == 1 || current_file->empty_data == 1)
        {
          current_file->type_data = TYPE_ENTRY_SEEDLING;
          current_file->index_data = 0;
        }
      else if(current_file->block_data < 257)
        {
          current_file->type_data = TYPE_ENTRY_SAPLING;
          current_file->index_data = 1;
        }
      else
        {
          current_file->type_data = TYPE_ENTRY_TREE;
          current_file->index_data = 1 + GetContainerNumber(current_file->block_data,INDEX_PER_BLOCK);
        }

      /* Resource */
      if(current_file->block_resource == 0 || current_file->block_resource == 1 || current_file->empty_resource == 1)
        {
          current_file->type_resource = TYPE_ENTRY_SEEDLING;
          current_file->index_resource = 0;
        }
      else if(current_file->block_resource < 257)
        {
          current_file->type_resource = TYPE_ENTRY_SAPLING;
          current_file->index_resource = 1;
        }
      else
        {
          current_file->type_resource = TYPE_ENTRY_TREE;
          current_file->index_resource = 1 + GetContainerNumber(current_file->block_resource,INDEX_PER_BLOCK);
        }

      /* Type Entry = Type Data */
      current_file->entry_type = TYPE_ENTRY_EXTENDED;

      /* Taille totale en block */
      current_file->entry_disk_block = 1 + (current_file->index_data + current_file->index_resource) + (current_file->block_disk_data + current_file->block_disk_resource);
    }

  /* Allocation mémoire pour la tableau de block */
  current_file->tab_data_block = (int *) calloc(current_file->index_data + current_file->block_disk_data + 1,sizeof(int));
  current_file->tab_resource_block = (int *) calloc(current_file->index_resource + current_file->block_disk_resource + 1,sizeof(int));
}


/*********************************************************************************/
/*  CreateFileContent() :  Création du contenu du fichier (index+data+resource). */
/*********************************************************************************/
static WORD CreateFileContent(struct prodos_image *current_image, struct prodos_file *current_file)
{
  int *tab_block;
  WORD file_block_number, data_block_number, resource_block_number;
  unsigned char extended_block[BLOCK_SIZE];

  /** Fichier Data **/
  if(current_file->has_resource == 0)
    {
      if(current_file->type_data == TYPE_ENTRY_SEEDLING)
        {
          file_block_number = CreateSeedlingContent(current_image,current_file,current_file->data,current_file->data_length,current_file->empty_data,1);
          if(file_block_number == 0)
            return(0);
        }
      else if(current_file->type_data == TYPE_ENTRY_SAPLING)
        {
          file_block_number = CreateSaplingContent(current_image,current_file,current_file->data,current_file->data_length,current_file->block_disk_data,1);
          if(file_block_number == 0)
            return(0);
        }
      else if(current_file->type_data == TYPE_ENTRY_TREE)
        {
          file_block_number = CreateTreeContent(current_image,current_file,current_file->data,current_file->data_length,current_file->block_disk_data,current_file->index_data,1);
          if(file_block_number == 0)
            return(0);
        }
    }
  else  /** Fichier Resource **/
    {
      /* Allocation du block Extended */
      tab_block = AllocateImageBlock(current_image,1);
      if(tab_block == NULL)
        return(0);
      file_block_number = tab_block[0];
      free(tab_block);

      /** Data **/
      if(current_file->type_data == TYPE_ENTRY_SEEDLING)
        {
          data_block_number = CreateSeedlingContent(current_image,current_file,current_file->data,current_file->data_length,current_file->empty_data,1);
          if(data_block_number == 0)
            return(0);
        }
      else if(current_file->type_data == TYPE_ENTRY_SAPLING)
        {
          data_block_number = CreateSaplingContent(current_image,current_file,current_file->data,current_file->data_length,current_file->block_disk_data,1);
          if(data_block_number == 0)
            return(0);
        }
      else if(current_file->type_data == TYPE_ENTRY_TREE)
        {
          data_block_number = CreateTreeContent(current_image,current_file,current_file->data,current_file->data_length,current_file->block_disk_data,current_file->index_data,1);
          if(data_block_number == 0)
            return(0);
        }

      /** Resource **/
      if(current_file->type_resource == TYPE_ENTRY_SEEDLING)
        {
          resource_block_number = CreateSeedlingContent(current_image,current_file,current_file->resource,current_file->resource_length,current_file->empty_resource,0);
          if(resource_block_number == 0)
            return(0);
        }
      else if(current_file->type_resource == TYPE_ENTRY_SAPLING)
        {
          resource_block_number = CreateSaplingContent(current_image,current_file,current_file->resource,current_file->resource_length,current_file->block_disk_resource,0);
          if(resource_block_number == 0)
            return(0);
        }
      else if(current_file->type_resource == TYPE_ENTRY_TREE)
        {
          resource_block_number = CreateTreeContent(current_image,current_file,current_file->resource,current_file->resource_length,current_file->block_disk_resource,current_file->index_resource,0);
          if(resource_block_number == 0)
            return(0);
        }

      /** Remplissage de l'Extended block **/
      memset(extended_block,0,BLOCK_SIZE);
      /* Data */
      extended_block[0] = (BYTE) current_file->type_data;
      SetWordValue(extended_block,0x01,(WORD)data_block_number);
      SetWordValue(extended_block,0x03,(WORD)current_file->index_data+current_file->block_disk_data);
      Set24bitValue(extended_block,0x05,current_file->data_length);
      memcpy(&extended_block[0x08],current_file->resource_finderinfo_1,18);
      memcpy(&extended_block[0x1A],current_file->resource_finderinfo_2,18);
      /* Resource */
      extended_block[BLOCK_SIZE/2+0] = (BYTE) current_file->type_resource;
      SetWordValue(extended_block,BLOCK_SIZE/2+0x01,(WORD)resource_block_number);
      SetWordValue(extended_block,BLOCK_SIZE/2+0x03,(WORD)current_file->index_resource+current_file->block_disk_resource);
      Set24bitValue(extended_block,BLOCK_SIZE/2+0x05,current_file->resource_length);
      /* Enregistre */
      SetBlockData(current_image,file_block_number,&extended_block[0]);
    }

  /* Renvoi le numéro de block du contenu du fichier */
  return(file_block_number);
}


/***************************************************************/
/*  CreateSeedlingContent() :  Création d'une entrée Seedling. */
/***************************************************************/
static WORD CreateSeedlingContent(struct prodos_image *current_image, struct prodos_file *current_file, unsigned char *data, int data_length, int empty_data, int is_data)
{
  WORD file_block_number;
  int *tab_block;
  unsigned char data_block[BLOCK_SIZE];

  /* Allocation d'un block */
  tab_block = AllocateImageBlock(current_image,1);
  if(tab_block == NULL)
    return(0);
  file_block_number = tab_block[0];
  free(tab_block);
  if(is_data)
    {
      current_file->nb_data_block = 1;
      current_file->tab_data_block[0] = (int) file_block_number;
    }
  else
    {
      current_file->nb_resource_block = 1;
      current_file->tab_resource_block[0] = (int) file_block_number;
    }

  /* Remplissage du block */
  memset(data_block,0,BLOCK_SIZE);
  if(empty_data == 0)
    memcpy(&data_block[0],data,data_length);
  SetBlockData(current_image,file_block_number,&data_block[0]);

  /* Renvoi le block */
  return(file_block_number);
}


/*************************************************************/
/*  CreateSaplingContent() :  Création d'une entrée Sapling. */
/*************************************************************/
static WORD CreateSaplingContent(struct prodos_image *current_image, struct prodos_file *current_file, unsigned char *data, int data_length, int block_disk_data, int is_data)
{
  int i,j,k,is_empty;
  WORD file_block_number, data_block_number;
  int *tab_block;
  unsigned char data_block[BLOCK_SIZE];
  unsigned char index_block[BLOCK_SIZE];
  unsigned char empty_block[BLOCK_SIZE];

  /* Init */
  memset(empty_block,0x00,BLOCK_SIZE);

  /* Allocation du block Index */
  tab_block = AllocateImageBlock(current_image,1);
  if(tab_block == NULL)
    return(0);
  file_block_number = tab_block[0];
  free(tab_block);
  if(is_data)
    {
      current_file->tab_data_block[0] = (int) file_block_number;
      current_file->nb_data_block = 1;
    }
  else
    {
      current_file->tab_resource_block[0] = (int) file_block_number;
      current_file->nb_resource_block = 1;
    }
  memset(index_block,0x00,BLOCK_SIZE);

  /* Allocation des block data */
  tab_block = AllocateImageBlock(current_image,block_disk_data);
  if(tab_block == NULL)
    return(0);
  for(i=0; i<block_disk_data; i++)
    {
      if(is_data)
        {
          current_file->tab_data_block[1+i] = tab_block[i];
          current_file->nb_data_block++;
        }
      else
        {
          current_file->tab_resource_block[1+i] = tab_block[i];
          current_file->nb_resource_block++;
        }
    }
  free(tab_block);

  /** Remplissage des block data **/
  for(i=0,j=1,k=0; i<data_length; i+=BLOCK_SIZE,k++)
    {
      /* Recherche les plages de 0 */
      if(i+BLOCK_SIZE <= data_length)
        is_empty = !memcmp(&data[i],empty_block,BLOCK_SIZE);
      else
        is_empty = !memcmp(&data[i],empty_block,data_length-i);
        
      /* Numéro du block */
      if(is_data)
        data_block_number = (is_empty == 1) ? 0 : current_file->tab_data_block[j++];
      else
        data_block_number = (is_empty == 1) ? 0 : current_file->tab_resource_block[j++];

      /* Place dans l'index */
      index_block[k] = (BYTE) (data_block_number & 0x00FF);
      index_block[BLOCK_SIZE/2+k] = (BYTE) ((data_block_number & 0xFF00) >> 8);

      /* Data du block */
      if(is_empty == 0)
        {
          memset(data_block,0,BLOCK_SIZE);
          if(i+BLOCK_SIZE <= data_length)
            memcpy(&data_block[0],&data[i],BLOCK_SIZE);
          else
            memcpy(&data_block[0],&data[i],data_length-i);
          SetBlockData(current_image,data_block_number,&data_block[0]);
        }
    }

  /* Stockage du block index */
  SetBlockData(current_image,file_block_number,&index_block[0]);

  /* Renvoie le block index */
  return(file_block_number);
}


/*******************************************************/
/*  CreateTreeContent() :  Création d'une entrée Tree. */
/*******************************************************/
static WORD CreateTreeContent(struct prodos_image *current_image, struct prodos_file *current_file, unsigned char *data, int data_length, int block_disk_data, int index_data, int is_data)
{
  WORD file_block_number, index_block_number, data_block_number;
  int i, j, k, l, is_empty;
  int *tab_block;
  unsigned char master_block[BLOCK_SIZE];
  unsigned char index_block[BLOCK_SIZE];
  unsigned char empty_block[BLOCK_SIZE];
  unsigned char data_block[BLOCK_SIZE];

  /* Init */
  memset(empty_block,0x00,BLOCK_SIZE);

  /* Allocation du block Master Index */
  tab_block = AllocateImageBlock(current_image,1);
  if(tab_block == NULL)
    return(0);
  file_block_number = tab_block[0];
  free(tab_block);
  if(is_data)
    {
      current_file->tab_data_block[0] = (int) file_block_number;
      current_file->nb_data_block = 1;
    }
  else
    {
      current_file->tab_resource_block[0] = (int) file_block_number;
      current_file->nb_resource_block = 1;
    }

  /* Allocation des block Index */
  tab_block = AllocateImageBlock(current_image,index_data-1);
  if(tab_block == NULL)
    return(0);
  for(i=0; i<index_data-1; i++)
    {
      if(is_data)
        {
          current_file->tab_data_block[1+i] = tab_block[i];
          current_file->nb_data_block++;
        }
      else
        {
          current_file->tab_resource_block[1+i] = tab_block[i];
          current_file->nb_resource_block++;
        }
    }
  free(tab_block);

  /** Remplissage du Master Index **/
  memset(master_block,0x00,BLOCK_SIZE);
  for(i=0; i<index_data-1; i++)
    {
      index_block_number = (is_data) ? current_file->tab_data_block[1+i] : current_file->tab_resource_block[1+i];
      master_block[i] = (BYTE) (index_block_number & 0x00FF);
      master_block[BLOCK_SIZE/2+i] = (BYTE) ((index_block_number & 0xFF00) >> 8);
    }

  /* Ecriture du Master Index */
  SetBlockData(current_image,file_block_number,&master_block[0]);

  /** Allocation des block data **/
  tab_block = AllocateImageBlock(current_image,block_disk_data);
  if(tab_block == NULL)
    return(0);
  for(i=0; i<block_disk_data; i++)
    {
      if(is_data)
        {
          current_file->tab_data_block[index_data+i] = tab_block[i];
          current_file->nb_data_block++;
        }
      else
        {
          current_file->tab_resource_block[index_data+i] = tab_block[i];
          current_file->nb_resource_block++;
        }
    }
  free(tab_block);

  /** Remplissage des block data **/
  memset(index_block,0x00,BLOCK_SIZE);
  for(i=0,j=index_data,k=0,l=0; i<data_length; i+=BLOCK_SIZE,k++)
    {
      /* Recherche les plages de 0 */
      if(i+BLOCK_SIZE <= data_length)
        is_empty = !memcmp(&data[i],empty_block,BLOCK_SIZE);
      else
        is_empty = !memcmp(&data[i],empty_block,data_length-i);
        
      /* Numéro du block */
      data_block_number = (is_empty == 1) ? 0 : ((is_data) ? current_file->tab_data_block[j++] : current_file->tab_resource_block[j++]);

      /* Place dans l'index */
      index_block[k] = (BYTE) (data_block_number & 0x00FF);
      index_block[BLOCK_SIZE/2+k] = (BYTE) ((data_block_number & 0xFF00) >> 8);

      /* Data du block */
      if(is_empty == 0)
        {
          memset(data_block,0,BLOCK_SIZE);
          if(i+BLOCK_SIZE <= data_length)
            memcpy(&data_block[0],&data[i],BLOCK_SIZE);
          else
            memcpy(&data_block[0],&data[i],data_length-i);
          SetBlockData(current_image,data_block_number,&data_block[0]);
        }

      /* Fin d'utilisation de l'index block (plein) */
      if(k+1 == BLOCK_SIZE/2)
        {
          /* Ecriture de l'index */
          index_block_number = (is_data) ? current_file->tab_data_block[1+l] : current_file->tab_resource_block[1+l];
          SetBlockData(current_image,index_block_number,&index_block[0]);
          l++;

          /* k=0 */
          memset(index_block,0x00,BLOCK_SIZE);
          k = -1; /* k++ */
        }
    }

  /* Dernier index block */
  if(k > 0)
    {
      /* Ecriture de l'index */
      index_block_number = (is_data) ? current_file->tab_data_block[1+l] : current_file->tab_resource_block[1+l];
      SetBlockData(current_image,index_block_number,&index_block[0]);
    }

  /* Renvoie le Master block */
  return(file_block_number);
}


/****************************************************************************/
/*  CreateFileEntry() :  Création de l'entrée du fichier dans le Directory. */
/****************************************************************************/
static void CreateFileEntry(struct prodos_image *current_image, struct prodos_file *current_file, WORD file_block_number,
                            struct file_descriptive_entry *target_folder, WORD directory_block_number, BYTE directory_entry_number, WORD directory_header_pointer)
{
  BYTE storage_type, storage_length;
  int offset, entry_length, file_count;
  unsigned char directory_block[BLOCK_SIZE];

  /* Entry Length */
  entry_length = 0x27;

  /* Lecture : Block contenant l'entrée */
  GetBlockData(current_image,directory_block_number,&directory_block[0]);

  /** Création de l'entrée SubDirectory **/
  offset = 4 + (directory_entry_number-1)*entry_length;

  /* Storage Type / Name length */
  storage_type = (BYTE) (0xF0 & (current_file->entry_type << 4));
  storage_length = storage_type | ((BYTE) strlen(current_file->file_name_case));
  directory_block[offset+0x00] = storage_length;

  /* File Name */
  memcpy(&directory_block[offset+0x01],current_file->file_name,strlen(current_file->file_name));

  /* File Type */
  directory_block[offset+0x10] = current_file->type;

  /* Key Pointer : File Index + Data + Resource */
  SetWordValue(directory_block,offset+0x11,file_block_number);

  /* Block Used (index+data+resource) */
  SetWordValue(directory_block,offset+0x13,(WORD)current_file->entry_disk_block);

  /* EOF : Taille du fichier  */
  if(current_file->has_resource == 0)
    Set24bitValue(directory_block,offset+0x15,current_file->data_length);   /* Taille des data */
  else
    Set24bitValue(directory_block,offset+0x15,BLOCK_SIZE);                  /* Taille du block d'index */

  /* Creation */
  SetWordValue(directory_block,offset+0x18,current_file->file_creation_date);
  SetWordValue(directory_block,offset+0x1A,current_file->file_creation_time);

  /* Lower case (Version + Minimum Version)*/
  SetWordValue(directory_block,offset+0x1C,current_file->name_case);

  /* Access */
  directory_block[offset+0x1E] = current_file->access;

  /* Aux Type */
  SetWordValue(directory_block,offset+0x1F,current_file->aux_type);

  /* Last Modification */
  SetWordValue(directory_block,offset+0x21,current_file->file_modification_date);
  SetWordValue(directory_block,offset+0x23,current_file->file_modification_time);

  /* Header Pointer */
  SetWordValue(directory_block,offset+0x25,(WORD)((target_folder == NULL) ? 2 : target_folder->key_pointer_block));

  /* Ecrit les données */
  SetBlockData(current_image,directory_block_number,&directory_block[0]);

  /** Modifie le nombre d'entrées valides du Target Folder : +1 **/
  GetBlockData(current_image,directory_header_pointer,&directory_block[0]);
  file_count = GetWordValue(directory_block,0x25);
  SetWordValue(directory_block,0x25,(WORD)(file_count+1));
  SetBlockData(current_image,directory_header_pointer,&directory_block[0]);
}


/***********************************************************************/
/*  CreateMemoryEntry() :  Création de l'entrée du fichier en mémoire. */
/***********************************************************************/
static int CreateMemoryEntry(struct prodos_image *current_image, struct file_descriptive_entry *target_folder, WORD directory_block_number, BYTE directory_entry_number)
{
  int offset, entry_length, error;
  struct file_descriptive_entry *current_entry;
  unsigned char directory_block[BLOCK_SIZE];
  char folder_path[1024];

  /* Lecture du block où est positionnée l'entrée */
  GetBlockData(current_image,directory_block_number,&directory_block[0]);
  entry_length = 0x27;;
  offset = 4 + (directory_entry_number-1)*entry_length;

  /* Chemin du dossier */
  if(target_folder == NULL)
    sprintf(folder_path,"/%s",current_image->volume_header->volume_name_case);
  else
    strcpy(folder_path,target_folder->file_path);

  /* Récupère l'entrée */
  current_entry = ODSReadFileDescriptiveEntry(current_image,folder_path,&directory_block[offset]);
  if(current_entry == NULL)
    return(1);

  /* Positionnement de cette entrée dans l'image */
  current_entry->depth = (target_folder == NULL) ? 1 : target_folder->depth+1;
  current_entry->parent_directory = target_folder;
  current_entry->block_location = directory_block_number;
  current_entry->entry_offset = offset;

  /* Ajoute cette entrée en mémoire */
  my_Memory(MEMORY_ADD_ENTRY,current_entry,NULL);
  my_Memory(MEMORY_BUILD_ENTRY_TAB,NULL,NULL);

  /** Met à jour le Dossier Cible (+1 fichier) **/
  if(target_folder == NULL)
    error = UpdateEntryTable(UPDATE_ADD,&current_image->nb_file,&current_image->tab_file,current_entry);
  else
    error = UpdateEntryTable(UPDATE_ADD,&target_folder->nb_file,&target_folder->tab_file,current_entry);
  if(error)
    return(1);

  /* OK */
  return(0);
}

/**********************************************************************/
