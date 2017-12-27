/***********************************************************************/
/*                                                                     */
/*  Main.c : Module de gestion des Images disques 2mg Prodos.          */
/*                                                                     */
/***********************************************************************/
/*  Auteur : Olivier ZARDINI  *  Brutal Deluxe Software  *  Janv 2011  */
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
#include "Prodos_Check.h"
#include "Prodos_Extract.h"
#include "Prodos_Rename.h"
#include "Prodos_Move.h"
#include "Prodos_Delete.h"
#include "Prodos_Create.h"
#include "Prodos_Add.h"
#include "Prodos_Source.h"

#define ACTION_CATALOG           10
#define ACTION_CHECK_VOLUME      11

#define ACTION_EXTRACT_FILE      20
#define ACTION_EXTRACT_FOLDER    21
#define ACTION_EXTRACT_VOLUME    22

#define ACTION_RENAME_FILE       30
#define ACTION_RENAME_FOLDER     31
#define ACTION_RENAME_VOLUME     32

#define ACTION_MOVE_FILE         40
#define ACTION_MOVE_FOLDER       41

#define ACTION_DELETE_FILE       50
#define ACTION_DELETE_FOLDER     51
#define ACTION_DELETE_VOLUME     52

#define ACTION_ADD_FILE          60
#define ACTION_ADD_FOLDER        61

#define ACTION_CREATE_FOLDER     70
#define ACTION_CREATE_VOLUME     71

#define ACTION_CLEAR_HIGH_BIT    80
#define ACTION_SET_HIGH_BIT      81
#define ACTION_INDENT_FILE       82
#define ACTION_OUTDENT_FILE      83

void usage(char *);
struct parameter *GetParamLine(int,char *[]);

// EXTRACTFILE C:\AppleIIgs\D3.2mg /D3/Divers/Merlin/Sources/Chaine.s c:\AppleIIgs\D3\

/****************************************************/
/*  main() :  Fonction principale de l'application. */
/****************************************************/
int main(int argc, char *argv[])
{
  int i, nb_filepath, verbose;
  char **filepath_tab;
  struct parameter *param;
  struct prodos_image *current_image;
  struct file_descriptive_entry *folder_entry;

  /* Init */
  verbose = 0;

  /* Message Information */
  printf("%s v 1.1, (c) Brutal Deluxe 2011-2013.\n",argv[0]);

  /* Vérification des paramètres */
  if(argc < 3)
    {
      usage(argv[0]);
      return(1);
    }

  /* Initialisation */
  my_Memory(MEMORY_INIT,NULL,NULL);

  /* Verbose */
  if(!my_stricmp(argv[argc-1],"-V"))
    verbose = 1;

  /** Décode les paramètres **/
  param = GetParamLine(argc-verbose,argv);
  if(param == NULL)
    return(2);
  param->verbose = verbose;

  /** Actions **/
  if(param->action == ACTION_CATALOG)
    {
      /* Information */
      printf("  - Catalog volume '%s'\n",param->image_file_path);

      /** Charge l'image 2mg **/
      current_image = LoadProdosImage(param->image_file_path);
      if(current_image == NULL)
        return(3);

      /** Affichage du contenu de l'image **/
      DumpProdosImage(current_image,param->verbose);

      /* Libération mémoire */
      mem_free_image(current_image);
    }
  else if(param->action == ACTION_CHECK_VOLUME)
    {
      /* Information */
      printf("  - Check volume '%s'\n",param->image_file_path);

      /** Charge l'image 2mg **/
      current_image = LoadProdosImage(param->image_file_path);
      if(current_image == NULL)
        return(3);

      /** Affichage des informations sur le contenu de l'image **/
      CheckProdosImage(current_image,param->verbose);

      /* Libération mémoire */
      mem_free_image(current_image);
    }
  else if(param->action == ACTION_EXTRACT_FILE)
    {
      /* Information */
      printf("  - Extract file '%s'\n",param->prodos_file_path);

      /** Charge l'image 2mg **/
      current_image = LoadProdosImage(param->image_file_path);
      if(current_image == NULL)
        return(3);

      /** Extrait le fichier sur disque **/
      ExtractOneFile(current_image,param->prodos_file_path,param->output_directory_path);

      /* Libération mémoire */
      mem_free_image(current_image);
    }
  else if(param->action == ACTION_EXTRACT_FOLDER)
    {
      /* Information */
      printf("  - Extract folder '%s' :\n",param->prodos_folder_path);

      /** Charge l'image 2mg **/
      current_image = LoadProdosImage(param->image_file_path);
      if(current_image == NULL)
        return(3);

      /** Recherche l'entrée du dossier **/
      folder_entry = GetProdosFolder(current_image,param->prodos_folder_path,1);
      if(folder_entry == NULL)
        return(4);

      /** Extrait les fichiers du répertoire **/
      ExtractFolderFiles(current_image,folder_entry,param->output_directory_path);

      /* Stat */
      printf("    => File(s) : %d,  Folder(s) : %d,  Error(s) : %d\n",current_image->nb_extract_file,current_image->nb_extract_folder,current_image->nb_extract_error);

      /* Libération mémoire */
      mem_free_image(current_image);
    }
  else if(param->action == ACTION_EXTRACT_VOLUME)
    {
      /** Charge l'image 2mg **/
      current_image = LoadProdosImage(param->image_file_path);
      if(current_image == NULL)
        return(3);

      /* Information */
      printf("  - Extract volume '%s' :\n",current_image->volume_header->volume_name_case);

      /** Extrait les fichiers du volume **/
      ExtractVolumeFiles(current_image,param->output_directory_path);

      /* Stat */
      printf("    => File(s) : %d,  Folder(s) : %d,  Error(s) : %d\n",current_image->nb_extract_file,current_image->nb_extract_folder,current_image->nb_extract_error);

      /* Libération mémoire */
      mem_free_image(current_image);
    }
  else if(param->action == ACTION_RENAME_FILE)
    {
      /** Charge l'image 2mg **/
      current_image = LoadProdosImage(param->image_file_path);
      if(current_image == NULL)
        return(3);

      /* Information */
      printf("  - Rename file '%s' as '%s' :\n",param->prodos_file_path,param->new_file_name);

      /** Renome le fichier **/
      RenameProdosFile(current_image,param->prodos_file_path,param->new_file_name);

      /* Libération mémoire */
      mem_free_image(current_image);
    }
  else if(param->action == ACTION_RENAME_FOLDER)
    {
      /** Charge l'image 2mg **/
      current_image = LoadProdosImage(param->image_file_path);
      if(current_image == NULL)
        return(3);

      /* Information */
      printf("  - Rename folder '%s' as '%s' :\n",param->prodos_folder_path,param->new_folder_name);

      /** Renome le dossier **/
      RenameProdosFolder(current_image,param->prodos_folder_path,param->new_folder_name);

      /* Libération mémoire */
      mem_free_image(current_image);
    }
  else if(param->action == ACTION_RENAME_VOLUME)
    {
      /** Charge l'image 2mg **/
      current_image = LoadProdosImage(param->image_file_path);
      if(current_image == NULL)
        return(3);

      /* Information */
      printf("  - Rename volume '%s' as '%s' :\n",current_image->volume_header->volume_name_case,param->new_volume_name);

      /** Renome le volume **/
      RenameProdosVolume(current_image,param->new_volume_name);

      /* Libération mémoire */
      mem_free_image(current_image);
    }
  else if(param->action == ACTION_MOVE_FILE)
    {
      /** Charge l'image 2mg **/
      current_image = LoadProdosImage(param->image_file_path);
      if(current_image == NULL)
        return(3);

      /* Information */
      printf("  - Move file '%s' to folder '%s' :\n",param->prodos_file_path,param->new_file_path);

      /** Déplace le fichier **/
      MoveProdosFile(current_image,param->prodos_file_path,param->new_file_path);

      /* Libération mémoire */
      mem_free_image(current_image);
    }
  else if(param->action == ACTION_MOVE_FOLDER)
    {
      /** Charge l'image 2mg **/
      current_image = LoadProdosImage(param->image_file_path);
      if(current_image == NULL)
        return(3);

      /* Information */
      printf("  - Move folder '%s' to '%s' :\n",param->prodos_folder_path,param->new_folder_path);

      /** Déplace le dossier **/
      MoveProdosFolder(current_image,param->prodos_folder_path,param->new_folder_path);

      /* Libération mémoire */
      mem_free_image(current_image);
    }
  else if(param->action == ACTION_DELETE_FILE)
    {
      /** Charge l'image 2mg **/
      current_image = LoadProdosImage(param->image_file_path);
      if(current_image == NULL)
        return(3);

      /* Information */
      printf("  - Delete file '%s' :\n",param->prodos_file_path);

      /** Supprime le fichier **/
      DeleteProdosFile(current_image,param->prodos_file_path);

      /* Libération mémoire */
      mem_free_image(current_image);
    }
  else if(param->action == ACTION_DELETE_FOLDER)
    {
      /** Charge l'image 2mg **/
      current_image = LoadProdosImage(param->image_file_path);
      if(current_image == NULL)
        return(3);

      /* Information */
      printf("  - Delete folder '%s' :\n",param->prodos_folder_path);

      /** Supprime le dossier **/
      DeleteProdosFolder(current_image,param->prodos_folder_path);

      /* Libération mémoire */
      mem_free_image(current_image);
    }
  else if(param->action == ACTION_DELETE_VOLUME)
    {
      /** Charge l'image 2mg **/
      current_image = LoadProdosImage(param->image_file_path);
      if(current_image == NULL)
        return(3);

      /* Information */
      printf("  - Delete volume '%s' :\n",current_image->volume_header->volume_name_case);

      /** Supprime le volume **/
      DeleteProdosVolume(current_image);

      /* Libération mémoire */
      mem_free_image(current_image);
    }
  else if(param->action == ACTION_CREATE_FOLDER)
    {
      /** Charge l'image 2mg **/
      current_image = LoadProdosImage(param->image_file_path);
      if(current_image == NULL)
        return(3);

      /* Information */
      printf("  - Create folder '%s' :\n",param->prodos_folder_path);

      /** Création du Folder **/
      CreateProdosFolder(current_image,param->prodos_folder_path);

      /* Libération mémoire */
      mem_free_image(current_image);
    }
  else if(param->action == ACTION_CREATE_VOLUME)
    {
      /* Information */
      printf("  - Create volume '%s' :\n",param->image_file_path);

      /** Création de l'image 2mg **/
      current_image = CreateProdosVolume(param->image_file_path,param->new_volume_name,param->new_volume_size_kb);
      if(current_image == NULL)
        return(3);

      /* Libération mémoire */
      mem_free_image(current_image);
    }
  else if(param->action == ACTION_ADD_FILE)
    {
      /** Charge l'image 2mg **/
      current_image = LoadProdosImage(param->image_file_path);
      if(current_image == NULL)
        return(3);

      /* Information */
      printf("  - Add file '%s' :\n",param->file_path);

      /** Ajoute le fichier dans l'archive **/
      AddFile(current_image,param->file_path,param->prodos_folder_path,1);

      /* Libération mémoire */
      mem_free_image(current_image);
    }
  else if(param->action == ACTION_ADD_FOLDER)
    {
      /** Charge l'image 2mg **/
      current_image = LoadProdosImage(param->image_file_path);
      if(current_image == NULL)
        return(3);

      /* Information */
      printf("  - Add folder '%s' :\n",param->folder_path);

      /** Ajoute l'ensemble des fichiers du répertoire dans l'archive **/
      AddFolder(current_image,param->folder_path,param->prodos_folder_path);

      /* Stat */
      printf("    => File(s) : %d,  Folder(s) : %d,  Error(s) : %d\n",current_image->nb_add_file,current_image->nb_add_folder,current_image->nb_add_error);

      /* Libération mémoire */
      mem_free_image(current_image);
    }
  else if(param->action == ACTION_CLEAR_HIGH_BIT)
    {
      /** Construit la liste des fichiers **/
      filepath_tab = BuildFileList(param->file_path,&nb_filepath);

      /** Met à 0 le bit 7 des octets du fichier **/
      for(i=0; i<nb_filepath; i++)
        {
          printf("  - Clear High bit for file '%s'\n",filepath_tab[i]);
          ClearFileHighBit(filepath_tab[i]);
        }

      /* Libération mémoire */
      mem_free_list(nb_filepath,filepath_tab);
    }
  else if(param->action == ACTION_SET_HIGH_BIT)
    {
      /** Construit la liste des fichiers **/
      filepath_tab = BuildFileList(param->file_path,&nb_filepath);

      /** Met à 1 le bit 7 des octets du fichier **/
      for(i=0; i<nb_filepath; i++)
        {
          printf("  - Set High bit for file '%s'\n",filepath_tab[i]);
          SetFileHighBit(filepath_tab[i]);
        }

      /* Libération mémoire */
      mem_free_list(nb_filepath,filepath_tab);
    }
  else if(param->action == ACTION_INDENT_FILE)
    {
      /** Construit la liste des fichiers **/
      filepath_tab = BuildFileList(param->file_path,&nb_filepath);

      /** Indente les lignes de code du fichier **/
      for(i=0; i<nb_filepath; i++)
        {
          printf("  - Indent file '%s'\n",filepath_tab[i]);
          IndentFile(filepath_tab[i]);
        }

      /* Libération mémoire */
      mem_free_list(nb_filepath,filepath_tab);
    }
  else if(param->action == ACTION_OUTDENT_FILE)
    {
      /** Construit la liste des fichiers **/
      filepath_tab = BuildFileList(param->file_path,&nb_filepath);

      /** Dé-Indente les lignes de code du fichier **/
      for(i=0; i<nb_filepath; i++)
        {
          printf("  - Outdent file '%s'\n",filepath_tab[i]);
          OutdentFile(filepath_tab[i]);
        }

      /* Libération mémoire */
      mem_free_list(nb_filepath,filepath_tab);
    }

  /* Libération mémoire */
  mem_free_param(param);
  my_Memory(MEMORY_FREE,NULL,NULL);

  /* OK */              
  return(0);
}


/************************************************************/
/*  usage() :  Indique les différentes options du logiciel. */
/************************************************************/
void usage(char *program_path)
{
  printf("Usage : %s COMMAND <param_1> <param_2> <param_3>... [-V] : \n",program_path);
  printf("        ----\n");
  printf("        %s CATALOG       <[2mg|hdv|po]_image_path>   [-V]\n",program_path);
  printf("        %s CHECKVOLUME   <[2mg|hdv|po]_image_path>   [-V]\n",program_path);
  printf("        ----\n");
  printf("        %s EXTRACTFILE   <[2mg|hdv|po]_image_path>   <prodos_file_path>    <output_directory>\n",program_path);
  printf("        %s EXTRACTFOLDER <[2mg|hdv|po]_image_path>   <prodos_folder_path>  <output_directory>\n",program_path);
  printf("        %s EXTRACTVOLUME <[2mg|hdv|po]_image_path>   <output_directory>\n",program_path);
  printf("        ----\n");
  printf("        %s RENAMEFILE    <[2mg|hdv|po]_image_path>   <prodos_file_path>    <new_file_name>\n",program_path);
  printf("        %s RENAMEFOLDER  <[2mg|hdv|po]_image_path>   <prodos_folder_path>  <new_folder_name>\n",program_path);
  printf("        %s RENAMEVOLUME  <[2mg|hdv|po]_image_path>   <new_volume_name>\n",program_path);
  printf("        ----\n");
  printf("        %s MOVEFILE      <[2mg|hdv|po]_image_path>   <prodos_file_path>    <target_folder_path>\n",program_path);
  printf("        %s MOVEFOLDER    <[2mg|hdv|po]_image_path>   <prodos_folder_path>  <target_folder_path>\n",program_path);
  printf("        ----\n");
  printf("        %s DELETEFILE    <[2mg|hdv|po]_image_path>   <prodos_file_path>\n",program_path);
  printf("        %s DELETEFOLDER  <[2mg|hdv|po]_image_path>   <prodos_folder_path>\n",program_path);
  printf("        %s DELETEVOLUME  <[2mg|hdv|po]_image_path>\n",program_path);
  printf("        ----\n");
  printf("        %s ADDFILE       <[2mg|hdv|po]_image_path>   <prodos_folder_path>  <file_path>\n",program_path);
  printf("        %s ADDFOLDER     <[2mg|hdv|po]_image_path>   <prodos_folder_path>  <folder_path>\n",program_path);
  printf("        ----\n");
  printf("        %s CREATEFOLDER  <[2mg|hdv|po]_image_path>   <prodos_folder_path>\n",program_path);
  printf("        %s CREATEVOLUME  <[2mg|hdv|po]_image_path>   <volume_name>         <volume_size>\n",program_path);
  printf("        ----\n");
  printf("        %s CLEARHIGHBIT  <source_file_path>\n",program_path);
  printf("        %s SETHIGHBIT    <source_file_path>\n",program_path);
  printf("        %s INDENTFILE    <source_file_path>\n",program_path);
  printf("        %s OUTDENTFILE   <source_file_path>\n",program_path);
  printf("        ----\n");
}


/***********************************************************************/
/*  GetParamLine() :  Décodage des paramètres de la ligne de commande. */
/***********************************************************************/
struct parameter *GetParamLine(int argc, char *argv[])
{
  struct parameter *param;
  char local_buffer[256];

  /* Vérifications */
  if(argc < 3)
    {
      usage(argv[0]);
      return(NULL);
    }

  /* Allocation mémoire */
  param = (struct parameter *) calloc(1,sizeof(struct parameter));
  if(param == NULL)
    {
      printf("  Error : Impossible to allocate memory for structure Param.\n");
      return(NULL);
    }

  /** CATALOG <image_path> **/
  if(!my_stricmp(argv[1],"CATALOG") && argc == 3)
    {
      param->action = ACTION_CATALOG;

      /* Chemin du fichier Image */
      param->image_file_path = strdup(argv[2]);
      if(param->image_file_path == NULL)
        {
          printf("  Error : Impossible to allocate memory for structure Param.\n");
          mem_free_param(param);
          return(NULL);
        }

      /* OK */
      return(param);
    }

  /** CHECKVOLUME <image_path> **/
  if(!my_stricmp(argv[1],"CHECKVOLUME") && argc == 3)
    {
      param->action = ACTION_CHECK_VOLUME;

      /* Chemin du fichier Image */
      param->image_file_path = strdup(argv[2]);
      if(param->image_file_path == NULL)
        {
          printf("  Error : Impossible to allocate memory for structure Param.\n");
          mem_free_param(param);
          return(NULL);
        }

      /* OK */
      return(param);
    }

  /** EXTRACTFILE <image_path> <prodos_file_path> <output_directory> **/
  if(!my_stricmp(argv[1],"EXTRACTFILE") && argc == 5)
    {
      param->action = ACTION_EXTRACT_FILE;

      /* Chemin du fichier Image */
      param->image_file_path = strdup(argv[2]);

      /* Chemin du fichier Prodos */
      param->prodos_file_path = strdup(argv[3]);

      /* Chemin du Répertoire Windows */
      param->output_directory_path = strdup(argv[4]);

      /* Vérification */
      if(param->image_file_path == NULL || param->prodos_file_path == NULL || param->output_directory_path == NULL)
        {
          printf("  Error : Impossible to allocate memory for structure Param.\n");
          mem_free_param(param);
          return(NULL);
        }

      /* OK */
      return(param);
    }

  /** EXTRACTFOLDER <image_path> <prodos_folder_path> <output_directory> **/
  if(!my_stricmp(argv[1],"EXTRACTFOLDER") && argc == 5)
    {
      param->action = ACTION_EXTRACT_FOLDER;

      /* Chemin du fichier Image */
      param->image_file_path = strdup(argv[2]);

      /* Chemin du fichier Prodos */
      param->prodos_folder_path = strdup(argv[3]);

      /* Chemin du Répertoire Windows */
      param->output_directory_path = strdup(argv[4]);

      /* Vérification */
      if(param->image_file_path == NULL || param->prodos_folder_path == NULL || param->output_directory_path == NULL)
        {
          printf("  Error : Impossible to allocate memory for structure Param.\n");
          mem_free_param(param);
          return(NULL);
        }

      /* OK */
      return(param);
    }

  /** EXTRACTVOLUME <image_path> <output_directory> **/
  if(!my_stricmp(argv[1],"EXTRACTVOLUME") && argc == 4)
    {
      param->action = ACTION_EXTRACT_VOLUME;

      /* Chemin du fichier Image */
      param->image_file_path = strdup(argv[2]);

      /* Chemin du Répertoire Windows */
      param->output_directory_path = strdup(argv[3]);

      /* Vérification */
      if(param->image_file_path == NULL || param->output_directory_path == NULL)
        {
          printf("  Error : Impossible to allocate memory for structure Param.\n");
          mem_free_param(param);
          return(NULL);
        }

      /* OK */
      return(param);
    }

  /** RENAMEFILE <2mg_image_path> <prodos_file_path> <new_file_name> **/
  if(!my_stricmp(argv[1],"RENAMEFILE") && argc == 5)
    {
      param->action = ACTION_RENAME_FILE;

      /* Chemin du fichier Image */
      param->image_file_path = strdup(argv[2]);

      /* Chemin du fichier Prodos */
      param->prodos_file_path = strdup(argv[3]);

      /* Nouveau nom de fichier */
      param->new_file_name = strdup(argv[4]);

      /* Vérification */
      if(param->image_file_path == NULL || param->prodos_file_path == NULL || param->new_file_name == NULL)
        {
          printf("  Error : Impossible to allocate memory for structure Param.\n");
          mem_free_param(param);
          return(NULL);
        }

      /* OK */
      return(param);
    }

  /** RENAMEFOLDER <2mg_image_path> <prodos_folder_path> <new_folder_name> **/
  if(!my_stricmp(argv[1],"RENAMEFOLDER") && argc == 5)
    {
      param->action = ACTION_RENAME_FOLDER;

      /* Chemin du fichier Image */
      param->image_file_path = strdup(argv[2]);

      /* Chemin du dossier Prodos */
      param->prodos_folder_path = strdup(argv[3]);

      /* Nouveau nom de dossier */
      param->new_folder_name = strdup(argv[4]);

      /* Vérification */
      if(param->image_file_path == NULL || param->prodos_folder_path == NULL || param->new_folder_name == NULL)
        {
          printf("  Error : Impossible to allocate memory for structure Param.\n");
          mem_free_param(param);
          return(NULL);
        }

      /* OK */
      return(param);
    }

  /** RENAMEVOLUME <2mg_image_path> <new_volume_name> **/
  if(!my_stricmp(argv[1],"RENAMEVOLUME") && argc == 4)
    {
      param->action = ACTION_RENAME_VOLUME;

      /* Chemin du fichier Image */
      param->image_file_path = strdup(argv[2]);

      /* Nouveau nom de volume */
      param->new_volume_name = strdup(argv[3]);

      /* Vérification */
      if(param->image_file_path == NULL || param->new_volume_name == NULL)
        {
          printf("  Error : Impossible to allocate memory for structure Param.\n");
          mem_free_param(param);
          return(NULL);
        }

      /* OK */
      return(param);
    }

  /** MOVEFILE <2mg_image_path> <prodos_file_path> <new_file_path> **/
  if(!my_stricmp(argv[1],"MOVEFILE") && argc == 5)
    {
      param->action = ACTION_MOVE_FILE;

      /* Chemin du fichier Image */
      param->image_file_path = strdup(argv[2]);

      /* Chemin du fichier Prodos */
      param->prodos_file_path = strdup(argv[3]);

      /* Nouveau nom de fichier */
      param->new_file_path = strdup(argv[4]);

      /* Vérification */
      if(param->image_file_path == NULL || param->prodos_file_path == NULL || param->new_file_path == NULL)
        {
          printf("  Error : Impossible to allocate memory for structure Param.\n");
          mem_free_param(param);
          return(NULL);
        }

      /* OK */
      return(param);
    }

  /** MOVEFOLDER <2mg_image_path> <prodos_folder_path> <target_folder_path> **/
  if(!my_stricmp(argv[1],"MOVEFOLDER") && argc == 5)
    {
      param->action = ACTION_MOVE_FOLDER;

      /* Chemin du fichier Image */
      param->image_file_path = strdup(argv[2]);

      /* Chemin du dossier Prodos */
      param->prodos_folder_path = strdup(argv[3]);

      /* Nouveau nom de dossier */
      param->new_folder_path = strdup(argv[4]);

      /* Vérification */
      if(param->image_file_path == NULL || param->prodos_folder_path == NULL || param->new_folder_path == NULL)
        {
          printf("  Error : Impossible to allocate memory for structure Param.\n");
          mem_free_param(param);
          return(NULL);
        }

      /* OK */
      return(param);
    }

  /** DELETEFILE <2mg_image_path> <prodos_file_path> **/
  if(!my_stricmp(argv[1],"DELETEFILE") && argc == 4)
    {
      param->action = ACTION_DELETE_FILE;

      /* Chemin du fichier Image */
      param->image_file_path = strdup(argv[2]);

      /* Chemin du fichier Prodos */
      param->prodos_file_path = strdup(argv[3]);

      /* Vérification */
      if(param->image_file_path == NULL || param->prodos_file_path == NULL)
        {
          printf("  Error : Impossible to allocate memory for structure Param.\n");
          mem_free_param(param);
          return(NULL);
        }

      /* OK */
      return(param);
    }

  /** DELETEFOLDER <2mg_image_path> <prodos_folder_path> **/
  if(!my_stricmp(argv[1],"DELETEFOLDER") && argc == 4)
    {
      param->action = ACTION_DELETE_FOLDER;

      /* Chemin du fichier Image */
      param->image_file_path = strdup(argv[2]);

      /* Chemin du dossier Prodos */
      param->prodos_folder_path = strdup(argv[3]);

      /* Vérification */
      if(param->image_file_path == NULL || param->prodos_folder_path == NULL)
        {
          printf("  Error : Impossible to allocate memory for structure Param.\n");
          mem_free_param(param);
          return(NULL);
        }

      /* OK */
      return(param);
    }

  /** DELETEVOLUME <2mg_image_path> **/
  if(!my_stricmp(argv[1],"DELETEVOLUME") && argc == 3)
    {
      param->action = ACTION_DELETE_VOLUME;

      /* Chemin du fichier Image */
      param->image_file_path = strdup(argv[2]);

      /* Vérification */
      if(param->image_file_path == NULL)
        {
          printf("  Error : Impossible to allocate memory for structure Param.\n");
          mem_free_param(param);
          return(NULL);
        }

      /* OK */
      return(param);
    }

  /** CREATEFOLDER <2mg_image_path> <folder_name> **/
  if(!my_stricmp(argv[1],"CREATEFOLDER") && argc == 4)
    {
      param->action = ACTION_CREATE_FOLDER;

      /* Chemin du fichier Image */
      param->image_file_path = strdup(argv[2]);

      /* Nom du Dossier Prodos */
      param->prodos_folder_path = strdup(argv[3]);

      /* Vérification */
      if(param->image_file_path == NULL || param->prodos_folder_path == NULL)
        {
          printf("  Error : Impossible to allocate memory for structure Param.\n");
          mem_free_param(param);
          return(NULL);
        }

      /* OK */
      return(param);
    }

  /** CREATEVOLUME <2mg_image_path> <volume_name> <volume_size> **/
  if(!my_stricmp(argv[1],"CREATEVOLUME") && argc == 5)
    {
      param->action = ACTION_CREATE_VOLUME;

      /* Chemin du fichier Image */
      param->image_file_path = strdup(argv[2]);

      /* Nom du volume Prodos */
      param->new_volume_name = strdup(argv[3]);

      /* Taille du volume */
      param->new_volume_size_kb = 0;
      if(strlen(argv[4]) > 3)
        {
          strcpy(local_buffer,argv[4]);
          local_buffer[strlen(local_buffer)-2] = '\0';
          param->new_volume_size_kb = atoi(local_buffer);
          if(!my_stricmp(&argv[4][strlen(argv[4])-2],"MB"))
            param->new_volume_size_kb *= 1024;
        }
      if(param->new_volume_size_kb <= 143 || param->new_volume_size_kb > 32768)
        param->new_volume_size_kb = 0;
      if(param->new_volume_size_kb == 0)
        {
          printf("  Error : Invalide volume size : '%s'.\n",argv[4]);
          mem_free_param(param);
          return(NULL);
        }

      /* Vérification */
      if(param->image_file_path == NULL || param->new_volume_name == NULL)
        {
          printf("  Error : Impossible to allocate memory for structure Param.\n");
          mem_free_param(param);
          return(NULL);
        }

      /* OK */
      return(param);
    }

  /** ADDFILE <2mg_image_path> <target_folder_path> <file_path> **/
  if(!my_stricmp(argv[1],"ADDFILE") && argc == 5)
    {
      param->action = ACTION_ADD_FILE;

      /* Chemin du fichier Image */
      param->image_file_path = strdup(argv[2]);

      /* Chemin du dossier où copier ce fichier */
      param->prodos_folder_path = strdup(argv[3]);

      /* Chemin du fichier Windows */
      param->file_path = strdup(argv[4]);

      /* Vérification */
      if(param->image_file_path == NULL || param->file_path == NULL || param->prodos_folder_path == NULL)
        {
          printf("  Error : Impossible to allocate memory for structure Param.\n");
          mem_free_param(param);
          return(NULL);
        }

      /* OK */
      return(param);
    }

  /** ADDFOLDER <2mg_image_path> <target_folder_path> <folder_path> **/
  if(!my_stricmp(argv[1],"ADDFOLDER") && argc == 5)
    {
      param->action = ACTION_ADD_FOLDER;

      /* Chemin du fichier Image */
      param->image_file_path = strdup(argv[2]);

      /* Chemin du dossier où copier ce fichier */
      param->prodos_folder_path = strdup(argv[3]);

      /* Chemin du fichier Windows */
      param->folder_path = strdup(argv[4]);

      /* Vérification */
      if(param->image_file_path == NULL || param->prodos_folder_path == NULL || param->folder_path == NULL)
        {
          printf("  Error : Impossible to allocate memory for structure Param.\n");
          mem_free_param(param);
          return(NULL);
        }

      /* OK */
      return(param);
    }

  /** CLEARHIGHBIT <file_path> **/
  if(!my_stricmp(argv[1],"CLEARHIGHBIT") && argc == 3)
    {
      param->action = ACTION_CLEAR_HIGH_BIT;

      /* Chemin du fichier */
      param->file_path = strdup(argv[2]);

      /* Vérification */
      if(param->file_path == NULL)
        {
          printf("  Error : Impossible to allocate memory for structure Param.\n");
          mem_free_param(param);
          return(NULL);
        }

      /* OK */
      return(param);
    }

  /** SETHIGHBIT <file_path> **/
  if(!my_stricmp(argv[1],"SETHIGHBIT") && argc == 3)
    {
      param->action = ACTION_SET_HIGH_BIT;

      /* Chemin du fichier */
      param->file_path = strdup(argv[2]);

      /* Vérification */
      if(param->file_path == NULL)
        {
          printf("  Error : Impossible to allocate memory for structure Param.\n");
          mem_free_param(param);
          return(NULL);
        }

      /* OK */
      return(param);
    }

  /** INDENTFILE <file_path> **/
  if(!my_stricmp(argv[1],"INDENTFILE") && argc == 3)
    {
      param->action = ACTION_INDENT_FILE;

      /* Chemin du fichier */
      param->file_path = strdup(argv[2]);

      /* Vérification */
      if(param->file_path == NULL)
        {
          printf("  Error : Impossible to allocate memory for structure Param.\n");
          mem_free_param(param);
          return(NULL);
        }

      /* OK */
      return(param);
    }

  /** OUTDENTFILE <file_path> **/
  if(!my_stricmp(argv[1],"OUTDENTFILE") && argc == 3)
    {
      param->action = ACTION_OUTDENT_FILE;

      /* Chemin du fichier */
      param->file_path = strdup(argv[2]);

      /* Vérification */
      if(param->file_path == NULL)
        {
          printf("  Error : Impossible to allocate memory for structure Param.\n");
          mem_free_param(param);
          return(NULL);
        }

      /* OK */
      return(param);
    }
  
  /* Action inconnue */
  usage(argv[0]);

  /* Libération */
  mem_free_param(param);

  /* Unknow Action */
  return(NULL);
}

/***********************************************************************/
