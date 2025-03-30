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
#include <stdbool.h>

#if IS_WINDOWS
#include <malloc.h>
#endif

#include "Dc_Shared.h"
#include "Dc_Prodos.h"
#include "Dc_Memory.h"
#include "os/os.h"
#include "Prodos_Dump.h"
#include "Prodos_Check.h"
#include "Prodos_Extract.h"
#include "Prodos_Rename.h"
#include "Prodos_Move.h"
#include "Prodos_Delete.h"
#include "Prodos_Create.h"
#include "Prodos_Add.h"
#include "Prodos_Source.h"
#include "log.h"

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
#define ACTION_REPLACE_FILE      62

#define ACTION_CREATE_FOLDER     70
#define ACTION_CREATE_VOLUME     71

#define ACTION_CLEAR_HIGH_BIT    80
#define ACTION_SET_HIGH_BIT      81
#define ACTION_INDENT_FILE       82
#define ACTION_OUTDENT_FILE      83

#define ERROR_HELP                1
#define ERROR_PARAM               2
#define ERROR_LOAD                3
#define ERROR_GET                 4
#define ERROR_EXTRACT             5
#define ERROR_ADD                 6

int apply_global_flags(struct parameter*, int, char**);
void apply_command_flags(struct parameter*, int, int, char**);
void usage(char *);
struct parameter *GetParamLine(int,char *[]);

/****************************************************/
/*  main() :  Fonction principale de l'application. */
/****************************************************/
int main(int argc, char *argv[])
{
  int i, nb_filepath, application_error=0;
  char **filepath_tab;
  struct parameter *param;
  struct prodos_image *current_image;
  struct file_descriptive_entry *folder_entry;

  /* Message Information */
  logf("%s v 1.4.5 (c) Brutal Deluxe 2011-2013.\n",argv[0]);

  /* Vérification des paramètres */
  if(argc < 3)
    {
      usage(argv[0]);
      return(ERROR_HELP);
    }

  /* Initialisation */
  my_Memory(MEMORY_INIT,NULL,NULL);

  /** Décode les paramètres **/
  param = GetParamLine(argc, argv);
  if(param == NULL)
    return(ERROR_PARAM);

  /** Actions **/
  if(param->action == ACTION_CATALOG)
    {
      /* Information */
      logf_info("  - Catalog volume '%s'\n",param->image_file_path);

      /** Charge l'image 2mg **/
      current_image = LoadProdosImage(param->image_file_path);
      if(current_image == NULL)
        return(ERROR_LOAD);

      /** Affichage du contenu de l'image **/
      DumpProdosImage(current_image,param->verbose);

      /* Libération mémoire */
      mem_free_image(current_image);
    }
  else if(param->action == ACTION_CHECK_VOLUME)
    {
      /* Information */
      logf_info("  - Check volume '%s'\n",param->image_file_path);

      /** Charge l'image 2mg **/
      current_image = LoadProdosImage(param->image_file_path);
      if(current_image == NULL)
        return(ERROR_LOAD);

      /** Affichage des informations sur le contenu de l'image **/
      CheckProdosImage(current_image,param->verbose);

      /* Libération mémoire */
      mem_free_image(current_image);
    }
  else if(param->action == ACTION_EXTRACT_FILE)
    {
      /* Information */
      logf_info("  - Extract file '%s'\n",param->prodos_file_path);
      if (param->output_apple_single)logf_info("    - Creating AppleSingle file!\n");

      /** Charge l'image 2mg **/
      current_image = LoadProdosImage(param->image_file_path);
      if(current_image == NULL)
        return(ERROR_LOAD);

      /** Extrait le fichier sur disque **/
      ExtractOneFile(
        current_image,
        param->prodos_file_path,
        param->output_directory_path,
        param->output_apple_single
      );

      /* Libération mémoire */
      mem_free_image(current_image);
    }
  else if(param->action == ACTION_EXTRACT_FOLDER)
    {
      /* Information */
      logf_info("  - Extract folder '%s' :\n",param->prodos_folder_path);
      if (param->output_apple_single)logf_info("    - Creating AppleSingle files!\n");

      /** Charge l'image 2mg **/
      current_image = LoadProdosImage(param->image_file_path);
      if(current_image == NULL)
        return(ERROR_LOAD);

      /** Recherche l'entrée du dossier **/
      folder_entry = GetProdosFolder(current_image,param->prodos_folder_path,1);
      if(folder_entry == NULL)
        return(ERROR_GET);

      /** Extrait les fichiers du répertoire **/
      ExtractFolderFiles(
        current_image,
        folder_entry,
        param->output_directory_path,
        param->output_apple_single
      );

      /* Stat */
      logf("    => File(s) : %d,  Folder(s) : %d,  Error(s) : %d\n",current_image->nb_extract_file,current_image->nb_extract_folder,current_image->nb_extract_error);
      if (current_image->nb_extract_error > 0) application_error = ERROR_EXTRACT;

      /* Libération mémoire */
      mem_free_image(current_image);
    }
  else if(param->action == ACTION_EXTRACT_VOLUME)
    {
      /** Charge l'image 2mg **/
      current_image = LoadProdosImage(param->image_file_path);
      if(current_image == NULL)
        return(ERROR_LOAD);

      /* Information */
      logf_info("  - Extract volume '%s' :\n",current_image->volume_header->volume_name_case);
      if (param->output_apple_single)logf_info("    - Creating AppleSingle files!\n");

      /** Extrait les fichiers du volume **/
      ExtractVolumeFiles(
        current_image,
        param->output_directory_path,
        param->output_apple_single
      );

      /* Stat */
      logf("    => File(s) : %d,  Folder(s) : %d,  Error(s) : %d\n",current_image->nb_extract_file,current_image->nb_extract_folder,current_image->nb_extract_error);
      if (current_image->nb_extract_error > 0) application_error = ERROR_EXTRACT;

      /* Libération mémoire */
      mem_free_image(current_image);
    }
  else if(param->action == ACTION_RENAME_FILE)
    {
      /** Charge l'image 2mg **/
      current_image = LoadProdosImage(param->image_file_path);
      if(current_image == NULL)
        return(ERROR_LOAD);

      /* Information */
      logf_info("  - Rename file '%s' as '%s' :\n",param->prodos_file_path,param->new_file_name);

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
        return(ERROR_LOAD);

      /* Information */
      logf_info("  - Rename folder '%s' as '%s' :\n",param->prodos_folder_path,param->new_folder_name);

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
        return(ERROR_LOAD);

      /* Information */
      logf_info("  - Rename volume '%s' as '%s' :\n",current_image->volume_header->volume_name_case,param->new_volume_name);

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
        return(ERROR_LOAD);

      /* Information */
      logf_info("  - Move file '%s' to folder '%s' :\n",param->prodos_file_path,param->new_file_path);

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
        return(ERROR_LOAD);

      /* Information */
      logf_info("  - Move folder '%s' to '%s' :\n",param->prodos_folder_path,param->new_folder_path);

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
        return(ERROR_LOAD);

      /* Information */
      logf_info("  - Delete file '%s' :\n",param->prodos_file_path);

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
        return(ERROR_LOAD);

      /* Information */
      logf_info("  - Delete folder '%s' :\n",param->prodos_folder_path);

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
        return(ERROR_LOAD);

      /* Information */
      logf_info("  - Delete volume '%s' :\n",current_image->volume_header->volume_name_case);

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
        return(ERROR_LOAD);

      /* Information */
      logf_info("  - Create folder '%s' :\n",param->prodos_folder_path);

      /** Création du Folder **/
      CreateProdosFolder(
        current_image,
        param->prodos_folder_path,
        param->zero_case_bits
      );

      /* Libération mémoire */
      mem_free_image(current_image);
    }
  else if(param->action == ACTION_CREATE_VOLUME)
    {
      /* Information */
      logf_info("  - Create volume '%s' :\n",param->image_file_path);

      /** Création de l'image 2mg **/
      current_image = CreateProdosVolume(
        param->image_file_path,
        param->new_volume_name,
        param->new_volume_size_kb,
        param->zero_case_bits
      );

      if(current_image == NULL)
        return(ERROR_LOAD);

      /* Libération mémoire */
      mem_free_image(current_image);
    }
  else if(param->action == ACTION_ADD_FILE)
    {
      /** Charge l'image 2mg **/
      current_image = LoadProdosImage(param->image_file_path);
      if(current_image == NULL)
        return(ERROR_LOAD);

      /* Information */
      logf_info("  - Add file '%s' :\n",param->file_path);

      /** Ajoute le fichier dans l'archive **/
      AddFile(
        current_image,
        param->file_path,
        param->prodos_folder_path,
        param->zero_case_bits,
        1
      );
      if (current_image->nb_add_error > 0) application_error = ERROR_ADD;

      /* Libération mémoire */
      mem_free_image(current_image);
    }
  else if(param->action == ACTION_ADD_FOLDER)
    {
      /** Charge l'image 2mg **/
      current_image = LoadProdosImage(param->image_file_path);
      if(current_image == NULL)
        return(ERROR_LOAD);

      /* Information */
      logf_info("  - Add folder '%s' :\n",param->folder_path);

      /** Ajoute l'ensemble des fichiers du répertoire dans l'archive **/
      AddFolder(current_image,param->folder_path,param->prodos_folder_path,param->zero_case_bits);
      if (current_image->nb_add_error > 0) application_error = ERROR_ADD;

      /* Stat */
      logf("    => File(s) : %d,  Folder(s) : %d,  Error(s) : %d\n",current_image->nb_add_file,current_image->nb_add_folder,current_image->nb_add_error);

      /* Libération mémoire */
      mem_free_image(current_image);
    }
  else if(param->action == ACTION_REPLACE_FILE)
    {
      /** Charge l'image 2mg **/
      current_image = LoadProdosImage(param->image_file_path);
      if(current_image == NULL)
        return(ERROR_LOAD);

      int fcharloc = 0;
      for (int i=strlen(param->file_path); i >= 0; --i) {
        if (!strncmp(&param->file_path[i], FOLDER_CHARACTER, 1))
        {
          fcharloc = i + 1;
          break;
        }
      }

      // Prepare parameters for delete
      char *file_name = param->file_path + fcharloc;
      char *prodos_file_name = calloc(1024, 1);
      strcat(prodos_file_name, param->prodos_folder_path);

      // The tool does not use Windows path conventions
      if (strncmp(&prodos_file_name[strlen(prodos_file_name) - 1], \
                  FOLDER_CHARACTER, strlen(FOLDER_CHARACTER)))
        strcat(prodos_file_name, "/");

      strcat(prodos_file_name, file_name);

      logf_info("  - Replacing file '%s' :\n",prodos_file_name);

      log_off();
      DeleteProdosFile(current_image, prodos_file_name);
      log_on();

      int add_error = AddFile(
        current_image,
        param->file_path,
        param->prodos_folder_path,
        param->zero_case_bits,
        1
      );
      if (add_error != 0) application_error = ERROR_ADD;

      free(prodos_file_name);
      mem_free_image(current_image);
    }
  else if(param->action == ACTION_CLEAR_HIGH_BIT)
    {
      /** Construit la liste des fichiers **/
      filepath_tab = BuildFileList(param->file_path,&nb_filepath);

      /** Met à 0 le bit 7 des octets du fichier **/
      for(i=0; i<nb_filepath; i++)
        {
          logf_info("  - Clear High bit for file '%s'\n",filepath_tab[i]);
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
          logf_info("  - Set High bit for file '%s'\n",filepath_tab[i]);
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
          logf_info("  - Indent file '%s'\n",filepath_tab[i]);
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
          logf_info("  - Outdent file '%s'\n",filepath_tab[i]);
          OutdentFile(filepath_tab[i]);
        }

      /* Libération mémoire */
      mem_free_list(nb_filepath,filepath_tab);
    }

  /* Libération mémoire */
  mem_free_param(param);
  my_Memory(MEMORY_FREE,NULL,NULL);

  /* return final error code, if any */
  return(application_error);
}

/**
 * Parse tail end of args and toggle global settings
 *
 * @param argc
 * @param args
 */
int apply_global_flags(struct parameter *params, int argc, char **argv)
{
  int found = 0;

  for (int i=3; i < argc; ++i)
  {
    if (!my_stricmp(argv[i], "--quiet"))
    {
      log_set_level(ERROR);
      found += 1;
    }

    if(!my_stricmp(argv[i], "-V"))
    {
      params -> verbose = 1;
      found += 1;
    }

    if (!my_stricmp(argv[i], "-A"))
    {
      params -> output_apple_single = true;
      found += 1;
    }
  }

  return argc-found;
}

void apply_command_flags(struct parameter *params, int start, int argc, char **argv)
{
  for (int i = start; i < argc; i++) {
    if ((
        params->action == ACTION_ADD_FILE ||
        params->action == ACTION_REPLACE_FILE ||
        params->action == ACTION_ADD_FOLDER ||
        params->action == ACTION_CREATE_FOLDER ||
        params->action == ACTION_CREATE_VOLUME
      )
      && (
        !my_stricmp(argv[i], "-C") ||
        !my_stricmp(argv[i], "--no-case-bits")
      )
    ) {
      params->zero_case_bits = true;
    }
  }
}

/************************************************************/
/*  usage() :  Indique les différentes options du logiciel. */
/************************************************************/
void usage(char *program_path)
{
  logf("Usage : %s COMMAND <param_1> <param_2> <param_3>... [-V --quiet] : \n",program_path);
  logf("        ----\n");
  logf("        %s CATALOG       <[2mg|hdv|po]_image_path>   [-V]\n",program_path);
  logf("        %s CHECKVOLUME   <[2mg|hdv|po]_image_path>   [-V]\n",program_path);
  logf("        ----\n");
  logf("        %s EXTRACTFILE   <[2mg|hdv|po]_image_path>   <prodos_file_path>    <output_directory>\n",program_path);
  logf("        %s EXTRACTFOLDER <[2mg|hdv|po]_image_path>   <prodos_folder_path>  <output_directory>\n",program_path);
  logf("        %s EXTRACTVOLUME <[2mg|hdv|po]_image_path>   <output_directory>\n\n",program_path);
  logf("        [-A] Extract as AppleSingle\n");
  logf("        ----\n");
  logf("        %s RENAMEFILE    <[2mg|hdv|po]_image_path>   <prodos_file_path>    <new_file_name>\n",program_path);
  logf("        %s RENAMEFOLDER  <[2mg|hdv|po]_image_path>   <prodos_folder_path>  <new_folder_name>\n",program_path);
  logf("        %s RENAMEVOLUME  <[2mg|hdv|po]_image_path>   <new_volume_name>\n",program_path);
  logf("        ----\n");
  logf("        %s MOVEFILE      <[2mg|hdv|po]_image_path>   <prodos_file_path>    <target_folder_path>\n",program_path);
  logf("        %s MOVEFOLDER    <[2mg|hdv|po]_image_path>   <prodos_folder_path>  <target_folder_path>\n",program_path);
  logf("        ----\n");
  logf("        %s DELETEFILE    <[2mg|hdv|po]_image_path>   <prodos_file_path>\n",program_path);
  logf("        %s DELETEFOLDER  <[2mg|hdv|po]_image_path>   <prodos_folder_path>\n",program_path);
  logf("        %s DELETEVOLUME  <[2mg|hdv|po]_image_path>\n",program_path);
  logf("        ----\n");
  logf("        %s ADDFILE       <[2mg|hdv|po]_image_path>   <prodos_folder_path>  <file_path>\n",program_path);
  logf("        [-C | --no-case-bits]\n");
  logf("        Specify a file's type and auxtype by formatting the file name: THING.S16#B30000\n");
  logf("        Delimiter must be '#'. Also works with REPLACEFILE, DELETEFILE, etc.\n");
  logf("        ----\n");
  logf("        %s REPLACEFILE   <[2mg|hdv|po]_image_path>   <prodos_folder_path>  <file_path>\n",program_path);
  logf("        [-C | --no-case-bits]\n");
  logf("        You may also specify a different type/auxtype for the file you intend to replace\n");
  logf("        (i.e. by changing the suffix) \n");
  logf("        ----\n");
  logf("        %s ADDFOLDER     <[2mg|hdv|po]_image_path>   <prodos_folder_path>  <folder_path>\n",program_path);
  logf("        [-C | --no-case-bits]\n");
  logf("        ----\n");
  logf("        %s CREATEFOLDER  <[2mg|hdv|po]_image_path>   <prodos_folder_path>\n",program_path);
  logf("        [-C | --no-case-bits]\n");
  logf("        %s CREATEVOLUME  <[2mg|hdv|po]_image_path>   <volume_name>         <volume_size>\n",program_path);
  logf("        [-C | --no-case-bits]\n");
  logf("        ----\n");
  logf("        %s CLEARHIGHBIT  <source_file_path>\n",program_path);
  logf("        %s SETHIGHBIT    <source_file_path>\n",program_path);
  logf("        %s INDENTFILE    <source_file_path>\n",program_path);
  logf("        %s OUTDENTFILE   <source_file_path>\n",program_path);
  logf("        ----\n");
}


/***********************************************************************/
/*  GetParamLine() :  Décodage des paramètres de la ligne de commande. */
/***********************************************************************/
struct parameter *GetParamLine(int argc, char *argv[])
{
  struct parameter *param;
  char local_buffer[256];

  for (int i = 0; i < argc; ++i)
    if (strlen(argv[i]) > 256)
    {
      logf("  Error: Argument too long!\n");
      return(NULL);
    }

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
      logf("  Error : Impossible to allocate memory for structure Param.\n");
      return(NULL);
    }

  int argc_no_global_flags = apply_global_flags(param, argc, argv);

  /** CATALOG <image_path> **/
  if(!my_stricmp(argv[1],"CATALOG") && argc_no_global_flags == 3)
    {
      param->action = ACTION_CATALOG;

      /* Chemin du fichier Image */
      param->image_file_path = strdup(argv[2]);
      if(param->image_file_path == NULL)
        {
          logf("  Error : Impossible to allocate memory for structure Param.\n");
          mem_free_param(param);
          return(NULL);
        }

      /* OK */
      return(param);
    }

  /** CHECKVOLUME <image_path> **/
  if(!my_stricmp(argv[1],"CHECKVOLUME") && argc_no_global_flags == 3)
    {
      param->action = ACTION_CHECK_VOLUME;

      /* Chemin du fichier Image */
      param->image_file_path = strdup(argv[2]);
      if(param->image_file_path == NULL)
        {
          logf("  Error : Impossible to allocate memory for structure Param.\n");
          mem_free_param(param);
          return(NULL);
        }

      /* OK */
      return(param);
    }

  /** EXTRACTFILE <image_path> <prodos_file_path> <output_directory> **/
  if(!my_stricmp(argv[1],"EXTRACTFILE") && argc_no_global_flags >= 5)
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
          logf("  Error : Impossible to allocate memory for structure Param.\n");
          mem_free_param(param);
          return(NULL);
        }

      /* OK */
      return(param);
    }

  /** EXTRACTFOLDER <image_path> <prodos_folder_path> <output_directory> **/
  if(!my_stricmp(argv[1],"EXTRACTFOLDER") && argc_no_global_flags >= 5)
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
          logf("  Error : Impossible to allocate memory for structure Param.\n");
          mem_free_param(param);
          return(NULL);
        }

      /* OK */
      return(param);
    }

  /** EXTRACTVOLUME <image_path> <output_directory> **/
  if(!my_stricmp(argv[1],"EXTRACTVOLUME") && argc_no_global_flags >= 4)
    {
      param->action = ACTION_EXTRACT_VOLUME;

      /* Chemin du fichier Image */
      param->image_file_path = strdup(argv[2]);

      /* Chemin du Répertoire Windows */
      param->output_directory_path = strdup(argv[3]);

      /* Vérification */
      if(param->image_file_path == NULL || param->output_directory_path == NULL)
        {
          logf("  Error : Impossible to allocate memory for structure Param.\n");
          mem_free_param(param);
          return(NULL);
        }

      /* OK */
      return(param);
    }

  /** RENAMEFILE <2mg_image_path> <prodos_file_path> <new_file_name> **/
  if(!my_stricmp(argv[1],"RENAMEFILE") && argc_no_global_flags == 5)
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
          logf("  Error : Impossible to allocate memory for structure Param.\n");
          mem_free_param(param);
          return(NULL);
        }

      /* OK */
      return(param);
    }

  /** RENAMEFOLDER <2mg_image_path> <prodos_folder_path> <new_folder_name> **/
  if(!my_stricmp(argv[1],"RENAMEFOLDER") && argc_no_global_flags == 5)
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
          logf("  Error : Impossible to allocate memory for structure Param.\n");
          mem_free_param(param);
          return(NULL);
        }

      /* OK */
      return(param);
    }

  /** RENAMEVOLUME <2mg_image_path> <new_volume_name> **/
  if(!my_stricmp(argv[1],"RENAMEVOLUME") && argc_no_global_flags == 4)
    {
      param->action = ACTION_RENAME_VOLUME;

      /* Chemin du fichier Image */
      param->image_file_path = strdup(argv[2]);

      /* Nouveau nom de volume */
      param->new_volume_name = strdup(argv[3]);

      /* Vérification */
      if(param->image_file_path == NULL || param->new_volume_name == NULL)
        {
          logf("  Error : Impossible to allocate memory for structure Param.\n");
          mem_free_param(param);
          return(NULL);
        }

      /* OK */
      return(param);
    }

  /** MOVEFILE <2mg_image_path> <prodos_file_path> <new_file_path> **/
  if(!my_stricmp(argv[1],"MOVEFILE") && argc_no_global_flags == 5)
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
          logf("  Error : Impossible to allocate memory for structure Param.\n");
          mem_free_param(param);
          return(NULL);
        }

      /* OK */
      return(param);
    }

  /** MOVEFOLDER <2mg_image_path> <prodos_folder_path> <target_folder_path> **/
  if(!my_stricmp(argv[1],"MOVEFOLDER") && argc_no_global_flags == 5)
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
          logf("  Error : Impossible to allocate memory for structure Param.\n");
          mem_free_param(param);
          return(NULL);
        }

      /* OK */
      return(param);
    }

  /** DELETEFILE <2mg_image_path> <prodos_file_path> **/
  if(!my_stricmp(argv[1],"DELETEFILE") && argc_no_global_flags == 4)
    {
      param->action = ACTION_DELETE_FILE;

      /* Chemin du fichier Image */
      param->image_file_path = strdup(argv[2]);

      /* Chemin du fichier Prodos */
      param->prodos_file_path = strdup(argv[3]);

      /* Vérification */
      if(param->image_file_path == NULL || param->prodos_file_path == NULL)
        {
          logf("  Error : Impossible to allocate memory for structure Param.\n");
          mem_free_param(param);
          return(NULL);
        }

      /* OK */
      return(param);
    }

  /** DELETEFOLDER <2mg_image_path> <prodos_folder_path> **/
  if(!my_stricmp(argv[1],"DELETEFOLDER") && argc_no_global_flags == 4)
    {
      param->action = ACTION_DELETE_FOLDER;

      /* Chemin du fichier Image */
      param->image_file_path = strdup(argv[2]);

      /* Chemin du dossier Prodos */
      param->prodos_folder_path = strdup(argv[3]);

      /* Vérification */
      if(param->image_file_path == NULL || param->prodos_folder_path == NULL)
        {
          logf("  Error : Impossible to allocate memory for structure Param.\n");
          mem_free_param(param);
          return(NULL);
        }

      /* OK */
      return(param);
    }

  /** DELETEVOLUME <2mg_image_path> **/
  if(!my_stricmp(argv[1],"DELETEVOLUME") && argc_no_global_flags == 3)
    {
      param->action = ACTION_DELETE_VOLUME;

      /* Chemin du fichier Image */
      param->image_file_path = strdup(argv[2]);

      /* Vérification */
      if(param->image_file_path == NULL)
        {
          logf("  Error : Impossible to allocate memory for structure Param.\n");
          mem_free_param(param);
          return(NULL);
        }

      /* OK */
      return(param);
    }

  /** CREATEFOLDER <2mg_image_path> <folder_name> **/
  if(!my_stricmp(argv[1],"CREATEFOLDER") && argc_no_global_flags >= 4)
    {
      param->action = ACTION_CREATE_FOLDER;

      /* Chemin du fichier Image */
      param->image_file_path = strdup(argv[2]);

      /* Nom du Dossier Prodos */
      param->prodos_folder_path = strdup(argv[3]);

      apply_command_flags(param, 3, argc, argv);

      /* Vérification */
      if(param->image_file_path == NULL || param->prodos_folder_path == NULL)
        {
          logf("  Error : Impossible to allocate memory for structure Param.\n");
          mem_free_param(param);
          return(NULL);
        }

      /* OK */
      return(param);
    }

  /** CREATEVOLUME <2mg_image_path> <volume_name> <volume_size> **/
  if(!my_stricmp(argv[1],"CREATEVOLUME") && argc_no_global_flags >= 5)
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
      if(param->new_volume_size_kb < 140 || param->new_volume_size_kb > 32768)
        param->new_volume_size_kb = 0;
      if(param->new_volume_size_kb == 0)
        {
          logf("  Error : Invalid volume size : '%s'.\n",argv[4]);
          mem_free_param(param);
          return(NULL);
        }

      apply_command_flags(param, 4, argc, argv);

      /* Vérification */
      if(param->image_file_path == NULL || param->new_volume_name == NULL)
        {
          logf("  Error : Impossible to allocate memory for structure Param.\n");
          mem_free_param(param);
          return(NULL);
        }

      /* OK */
      return(param);
    }

  /** ADDFILE <2mg_image_path> <target_folder_path> <file_path> **/
  if(!my_stricmp(argv[1],"ADDFILE") && argc_no_global_flags >= 5)
    {
      param->action = ACTION_ADD_FILE;

      /* Chemin du fichier Image */
      param->image_file_path = strdup(argv[2]);

      /* Chemin du dossier où copier ce fichier */
      param->prodos_folder_path = strdup(argv[3]);

      /* Chemin du fichier Windows */
      param->file_path = strdup(argv[4]);

      apply_command_flags(param, 4, argc, argv);

      /* Vérification */
      if(param->image_file_path == NULL || param->file_path == NULL || param->prodos_folder_path == NULL)
        {
          logf("  Error : Impossible to allocate memory for structure Param.\n");
          mem_free_param(param);
          return(NULL);
        }

      /* OK */
      return(param);
    }

  /** REPLACEFILE <2mg_image_path> <target_folder_path> <file_path> **/
  if(!my_stricmp(argv[1],"REPLACEFILE") && argc_no_global_flags >= 5)
    {
      param->action = ACTION_REPLACE_FILE;

      /* Chemin du fichier Image */
      param->image_file_path = strdup(argv[2]);

      /* Chemin du dossier où copier ce fichier */
      param->prodos_folder_path = strdup(argv[3]);

      /* Chemin du fichier Windows */
      param->file_path = strdup(argv[4]);

      apply_command_flags(param, 4, argc, argv);

      /* Vérification */
      if(param->image_file_path == NULL || param->file_path == NULL || param->prodos_folder_path == NULL)
        {
          logf("  Error : Impossible to allocate memory for structure Param.\n");
          mem_free_param(param);
          return(NULL);
        }

      /* OK */
      return(param);
    }

  /** ADDFOLDER <2mg_image_path> <target_folder_path> <folder_path> **/
  if(!my_stricmp(argv[1],"ADDFOLDER") && argc_no_global_flags >= 5)
    {
      param->action = ACTION_ADD_FOLDER;

      /* Chemin du fichier Image */
      param->image_file_path = strdup(argv[2]);

      /* Chemin du dossier où copier ce fichier */
      param->prodos_folder_path = strdup(argv[3]);

      /* Chemin du fichier Windows */
      param->folder_path = strdup(argv[4]);

      apply_command_flags(param, 4, argc, argv);

      /* Vérification */
      if(param->image_file_path == NULL || param->prodos_folder_path == NULL || param->folder_path == NULL)
        {
          logf("  Error : Impossible to allocate memory for structure Param.\n");
          mem_free_param(param);
          return(NULL);
        }

      /* OK */
      return(param);
    }

  /** CLEARHIGHBIT <file_path> **/
  if(!my_stricmp(argv[1],"CLEARHIGHBIT") && argc_no_global_flags == 3)
    {
      param->action = ACTION_CLEAR_HIGH_BIT;

      /* Chemin du fichier */
      param->file_path = strdup(argv[2]);

      /* Vérification */
      if(param->file_path == NULL)
        {
          logf("  Error : Impossible to allocate memory for structure Param.\n");
          mem_free_param(param);
          return(NULL);
        }

      /* OK */
      return(param);
    }

  /** SETHIGHBIT <file_path> **/
  if(!my_stricmp(argv[1],"SETHIGHBIT") && argc_no_global_flags == 3)
    {
      param->action = ACTION_SET_HIGH_BIT;

      /* Chemin du fichier */
      param->file_path = strdup(argv[2]);

      /* Vérification */
      if(param->file_path == NULL)
        {
          logf("  Error : Impossible to allocate memory for structure Param.\n");
          mem_free_param(param);
          return(NULL);
        }

      /* OK */
      return(param);
    }

  /** INDENTFILE <file_path> **/
  if(!my_stricmp(argv[1],"INDENTFILE") && argc_no_global_flags == 3)
    {
      param->action = ACTION_INDENT_FILE;

      /* Chemin du fichier */
      param->file_path = strdup(argv[2]);

      /* Vérification */
      if(param->file_path == NULL)
        {
          logf("  Error : Impossible to allocate memory for structure Param.\n");
          mem_free_param(param);
          return(NULL);
        }

      /* OK */
      return(param);
    }

  /** OUTDENTFILE <file_path> **/
  if(!my_stricmp(argv[1],"OUTDENTFILE") && argc_no_global_flags == 3)
    {
      param->action = ACTION_OUTDENT_FILE;

      /* Chemin du fichier */
      param->file_path = strdup(argv[2]);

      /* Vérification */
      if(param->file_path == NULL)
        {
          logf("  Error : Impossible to allocate memory for structure Param.\n");
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
