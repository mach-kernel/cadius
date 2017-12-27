/***********************************************************************/
/*                                                                     */
/*   Prodos_Extract.c : Module pour la gestion des commandes EXTRACT.  */
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
#include "Dc_OS.h"
#include "Prodos_Extract.h"

static int CreateOutputFile(struct prodos_file *,char *);
static void SetFileInformation(char *,struct prodos_file *);

/**************************************************************/
/*  ExtractOneFile() :  Extrait un fichier Prodos sur disque. */
/**************************************************************/
void ExtractOneFile(struct prodos_image *current_image, char *prodos_file_path, char *output_directory_path)
{
  int error;
  struct file_descriptive_entry *current_entry;
  struct prodos_file *current_file;

  /** Recherche l'entrée du fichier **/
  current_entry = GetProdosFile(current_image,prodos_file_path);
  if(current_entry == NULL)
    return;

  /** Allocation mémoire **/
  current_file = (struct prodos_file *) calloc(1,sizeof(struct prodos_file));
  if(current_file == NULL)
    {
      printf("  Error : Can't get file from Image : Memory Allocation impossible.\n");
      return;
    }
  current_file->entry = current_entry;

  /** Récupère les data de ce fichier **/
  error = GetDataFile(current_image,current_entry,current_file);
  if(error)
    {
      printf("  Error : Can't get file from Image : Memory Allocation impossible.\n");
      mem_free_file(current_file);
      return;
    }

  /** Création du fichier sur disque **/
  error = CreateOutputFile(current_file,output_directory_path);

  /* Libération mémoire */
  mem_free_file(current_file);
}


/************************************************************************/
/*  ExtractFolderFiles() :  Fonction récursive d'extraction de fichier. */
/************************************************************************/
void ExtractFolderFiles(struct prodos_image *current_image, struct file_descriptive_entry *folder_entry, char *output_directory_path)
{
  int i, error;
  char *windows_folder_path;
  struct file_descriptive_entry *current_entry;
  struct prodos_file *current_file;

  /** Création du dossier sur disque **/
  /* Chemin du dossier */
  windows_folder_path = (char *) calloc(strlen(output_directory_path) + strlen(folder_entry->file_name_case) + 256,sizeof(char));
  if(windows_folder_path == NULL)
    {
      printf("  Error : Can't extract folder files from Image : Memory Allocation impossible.\n");
      current_image->nb_extract_error++;
      return;
    }
  strcpy(windows_folder_path,output_directory_path);
  if(strlen(windows_folder_path) > 0)
    if(windows_folder_path[strlen(windows_folder_path)-1] != '\\' && windows_folder_path[strlen(windows_folder_path)-1] != '/')
      strcat(windows_folder_path,FOLDER_CHARACTER);
  strcat(windows_folder_path,folder_entry->file_name_case);
  strcat(windows_folder_path,FOLDER_CHARACTER);

  /* Création du dossier */
  error = my_CreateDirectory(windows_folder_path);
  if(error)
    {
      printf("  Error : Can't create folder : '%s'.\n",windows_folder_path);
      current_image->nb_extract_error++;
      free(windows_folder_path);
      return;
    }
  current_image->nb_extract_folder++;

  /*****************************************************/
  /**  Traitement de tous les fichiers du répertoire  **/
  for(i=0; i<folder_entry->nb_file; i++)
    {
      /* Entrée du fichier */
      current_entry = folder_entry->tab_file[i];

      /* Information */
      printf("      o Extract File   : %s\n",current_entry->file_path);

      /** Allocation mémoire **/
      current_file = (struct prodos_file *) calloc(1,sizeof(struct prodos_file));
      if(current_file == NULL)
        {
          printf("  Error : Can't get file from Image : Memory Allocation impossible.\n");
          current_image->nb_extract_error++;
          continue;
        }
      current_file->entry = current_entry;

      /** Récupère les data de ce fichier **/
      error = GetDataFile(current_image,current_entry,current_file);
      if(error)
        {
          printf("  Error : Can't get file from Image : Memory Allocation impossible.\n");
          current_image->nb_extract_error++;
          mem_free_file(current_file);
          continue;
        }

      /** Création du fichier sur disque **/
      error = CreateOutputFile(current_file,windows_folder_path);

      /* Libération mémoire */
      mem_free_file(current_file);

      /* Stat */
      if(error)
        current_image->nb_extract_error++;
      else
        current_image->nb_extract_file++;
    }

  /*****************************************************/
  /**  Traitement de tous les dossiers du répertoire  **/
  for(i=0; i<folder_entry->nb_directory; i++)
    {
      /* Entrée du fichier */
      current_entry = folder_entry->tab_directory[i];

      /* Information */
      printf("      + Extract Folder : %s\n",current_entry->file_path);

      /** Récursivité **/
      ExtractFolderFiles(current_image,current_entry,windows_folder_path);
    }

  /** Libération mémoire **/
  free(windows_folder_path);
}


/****************************************************************************/
/*  ExtractVolumeFiles() :  Fonction d'extraction des fichiers d'un volume. */
/****************************************************************************/
void ExtractVolumeFiles(struct prodos_image *current_image, char *output_directory_path)
{
  int i, error;
  char *windows_folder_path;
  struct file_descriptive_entry *current_entry;
  struct prodos_file *current_file;

  /** Création du dossier sur disque **/
  /* Chemin du dossier */
  windows_folder_path = (char *) calloc(strlen(output_directory_path) + strlen(current_image->volume_header->volume_name_case) + 256,sizeof(char));
  if(windows_folder_path == NULL)
    {
      printf("  Error : Can't extract files from Image : Memory Allocation impossible.\n");
      current_image->nb_extract_error++;
      return;
    }
  strcpy(windows_folder_path,output_directory_path);
  if(strlen(windows_folder_path) > 0)
    if(windows_folder_path[strlen(windows_folder_path)-1] != '\\' && windows_folder_path[strlen(windows_folder_path)-1] != '/')
      strcat(windows_folder_path,FOLDER_CHARACTER);
  strcat(windows_folder_path,current_image->volume_header->volume_name_case);
  strcat(windows_folder_path,FOLDER_CHARACTER);

  /* Création du dossier */
  error = my_CreateDirectory(windows_folder_path);
  if(error)
    {
      printf("  Error : Can't create folder : '%s'.\n",windows_folder_path);
      free(windows_folder_path);
      current_image->nb_extract_error++;
      return;
    }

  /****************************************************/
  /**  Traitement de tous les fichiers de la racine  **/
  for(i=0; i<current_image->nb_file; i++)
    {
      /* Entrée du fichier */
      current_entry = current_image->tab_file[i];

      /* Information */
      printf("      o Extract File   : %s\n",current_entry->file_path);

      /** Allocation mémoire **/
      current_file = (struct prodos_file *) calloc(1,sizeof(struct prodos_file));
      if(current_file == NULL)
        {
          printf("  Error : Can't get file from Image : Memory Allocation impossible.\n");
          current_image->nb_extract_error++;
          continue;
        }
      current_file->entry = current_entry;

      /** Récupère les data de ce fichier **/
      error = GetDataFile(current_image,current_entry,current_file);
      if(error)
        {
          printf("  Error : Can't get file from Image : Memory Allocation impossible.\n");
          current_image->nb_extract_error++;
          mem_free_file(current_file);
          continue;
        }

      /** Création du fichier sur disque **/
      error = CreateOutputFile(current_file,windows_folder_path);

      /* Libération mémoire */
      mem_free_file(current_file);

      /* Stat */
      if(error)
        current_image->nb_extract_error++;
      else
        current_image->nb_extract_file++;
    }

  /****************************************************/
  /**  Traitement de tous les dossiers de la racine  **/
  for(i=0; i<current_image->nb_directory; i++)
    {
      /* Entrée du fichier */
      current_entry = current_image->tab_directory[i];

      /* Information */
      printf("      + Extract Folder : %s\n",current_entry->file_path);

      /** Récursivité **/
      ExtractFolderFiles(current_image,current_entry,windows_folder_path);
    }

  /** Libération mémoire **/
  free(windows_folder_path);
}


/************************************************************/
/*  CreateOutputFile() :  Création d'un fichier sur disque. */
/************************************************************/
static int CreateOutputFile(struct prodos_file *current_file, char *output_directory_path)
{
  int error;
  char directory_path[1024];
  char file_data_path[1024];
  char file_resource_path[1024];
  char file_information_path[1024];

  /* Création du répertoire de base */
  error = my_CreateDirectory(output_directory_path);
  if(error)
    {
      printf("  Error : Can't create output folder '%s'.\n",output_directory_path);
      return(1);
    }
  strcpy(directory_path,output_directory_path);
  if(strlen(directory_path) > 0)
    if(directory_path[strlen(directory_path)-1] != '\\' && directory_path[strlen(directory_path)-1] != '/')
      strcat(directory_path,FOLDER_CHARACTER);

  /* Chemin du fichier : Data */
  strcpy(file_data_path,directory_path);
  strcat(file_data_path,current_file->entry->file_name_case);

  /* Chemin du fichier : Resource */
  strcpy(file_resource_path,file_data_path);
  strcat(file_resource_path,"_ResourceFork.bin");

  /**********************************/
  /**  Création du Fichier : Data  **/
  /**********************************/
  error = CreateBinaryFile(file_data_path,current_file->data,current_file->data_length);
  if(error)
    {
      printf("  Error : Can't create file '%s' on disk at location '%s'.\n",current_file->entry->file_name_case,file_data_path);
      return(1);
    }

  /** Ajustement des Dates **/
  my_SetFileCreationModificationDate(file_data_path,current_file->entry);

  /** Change la visibilité du fichier **/
  my_SetFileAttribute(file_data_path,((current_file->entry->access&0x04)==0x04)?SET_FILE_HIDDEN:SET_FILE_VISIBLE);

  /** Ajoute des informations du fichier dans le fichier FileInformation.txt **/
  strcpy(file_information_path,directory_path);
  strcat(file_information_path,"_FileInformation.txt");
  SetFileInformation(file_information_path,current_file);

  /**************************************/
  /**  Création du Fichier : Resource  **/
  /**************************************/
  if(current_file->resource_length > 0)
    {
      error = CreateBinaryFile(file_resource_path,current_file->resource,current_file->resource_length);
      if(error)
        {
          printf("  Error : Can't create resource file '%s' on disk at location '%s'.\n",current_file->entry->file_name_case,file_resource_path);
          return(1);
        }

      /** Ajustement des Dates **/
      my_SetFileCreationModificationDate(file_resource_path,current_file->entry);

      /** Change la visibilité du fichier **/
      my_SetFileAttribute(file_resource_path,((current_file->entry->access&0x04)==0x04)?SET_FILE_HIDDEN:SET_FILE_VISIBLE);
    }

  /* OK */
  return(0);
}


/********************************************************************/
/*  SetFileInformation() :  Place les informations dans un fichier. */ 
/********************************************************************/
static void SetFileInformation(char *file_information_path, struct prodos_file *current_file)
{
  FILE *fd;
  char *next_sep;
  int i, nb_line;
  char **line_tab;
  char file_name[1024];
  char local_buffer[1024];
  char folder_info1[256];
  char folder_info2[256];
  
  /* Folder Info */
  for(i=0; i<18; i++)
    {
      sprintf(&folder_info1[2*i],"%02X",current_file->resource_finderinfo_1[i]);
      sprintf(&folder_info2[2*i],"%02X",current_file->resource_finderinfo_2[i]);
    }

  /** Prépare la ligne du fichier **/
  sprintf(local_buffer,"%s=Type(%02X),AuxType(%04X),VersionCreate(%02X),MinVersion(%02X),Access(%02X),FolderInfo1(%s),FolderInfo2(%s)",current_file->entry->file_name_case,
          current_file->entry->file_type,current_file->entry->file_aux_type,current_file->entry->version_created,current_file->entry->min_version,
          current_file->entry->access,folder_info1,folder_info2);

  /** Charge en mémoire le fichier **/
  line_tab = BuildUniqueListFromFile(file_information_path,&nb_line);
  if(line_tab == NULL)
    {
      /* Créer le fichier FileInformation */
      CreateBinaryFile(file_information_path,(unsigned char *)local_buffer,(int)strlen(local_buffer));

      /* Rendre le fichier invisible */
      my_SetFileAttribute(file_information_path,SET_FILE_HIDDEN);
      return;
    }

  /* Rendre le fichier visible */
  my_SetFileAttribute(file_information_path,SET_FILE_VISIBLE);

  /** Création du fichier **/
  fd = fopen(file_information_path,"w");
  if(fd == NULL)
    {
      mem_free_list(nb_line,line_tab);
      return;
    }

  /** Ajouts des lignes existantes **/
  for(i=0; i<nb_line; i++)
    {
      /* Isole le nom du fichier */
      next_sep = strchr(line_tab[i],'=');
      if(next_sep == NULL)
        continue;

      /* Recherche le fichier actuel */
      memcpy(file_name,line_tab[i],next_sep-line_tab[i]);
      file_name[next_sep-line_tab[i]] = '\0';

      /* On ne recopie pas la ligne du fichier */
      if(my_stricmp(file_name,current_file->entry->file_name_case))
        fprintf(fd,"%s\n",line_tab[i]);
    }
        
  /* Nouvelle ligne */
  fprintf(fd,"%s\n",local_buffer);

  /* Fermeture */
  fclose(fd);

  /* Libération mémoire */
  mem_free_list(nb_line,line_tab);

  /* Rendre le fichier invisible */
  my_SetFileAttribute(file_information_path,SET_FILE_HIDDEN);
}

/***********************************************************************/
