/***********************************************************************/
/*                                                                     */
/* Dc_Memory.c : Module pour la bibliothèque de gestion de la mémoire. */
/*                                                                     */
/***********************************************************************/
/*  Auteur : Olivier ZARDINI  *  Brutal Deluxe Software  *  Dec 2011   */
/***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>

#include "Dc_Shared.h"
#include "Dc_Prodos.h"
#include "Dc_Memory.h"

/***************************************************/
/*  my_Memory() :  Gestion des ressources mémoire. */
/***************************************************/
void my_Memory(int code, void *data, void *value)
{
  int i, index;
  char *path;
  char *message;
  static int nb_entry;
  static struct file_descriptive_entry *first_entry;
  static struct file_descriptive_entry *last_entry;
  static struct file_descriptive_entry **tab_entry;
  struct file_descriptive_entry *current_entry;
  struct file_descriptive_entry *next_entry;
  struct file_descriptive_entry *delete_entry;
  static int nb_directory;
  static struct file_descriptive_entry *first_directory;
  static struct file_descriptive_entry *last_directory;
  static struct file_descriptive_entry **tab_directory;
  struct file_descriptive_entry *current_directory;
  struct file_descriptive_entry *next_directory;
  struct file_descriptive_entry *delete_directory;
  static int nb_filepath;
  static struct file_path *first_filepath;
  static struct file_path *last_filepath;
  struct file_path *current_filepath;
  struct file_path *next_filepath;
  static int nb_error;
  static struct error *first_error;
  static struct error *last_error;
  struct error *current_error;
  struct error *next_error;

  switch(code)
    {
      case MEMORY_INIT :
        nb_entry = 0;
        first_entry = NULL;
        last_entry = NULL;
        tab_entry = NULL;
        nb_directory = 0;
        first_directory = NULL;
        last_directory = NULL;
        tab_directory = NULL;
        nb_filepath = 0;
        first_filepath = NULL;
        last_filepath = NULL;
        nb_error = 0;
        first_error = NULL;
        last_error = NULL;
        break;

      case MEMORY_FREE :
        my_Memory(MEMORY_FREE_ENTRY,NULL,NULL);
        my_Memory(MEMORY_FREE_DIRECTORY,NULL,NULL);
        my_Memory(MEMORY_FREE_FILE,NULL,NULL);
        my_Memory(MEMORY_FREE_ERROR,NULL,NULL);
        break;

      /*********************************************/
      /*  Liste chainée des structure entry : File */
      /*********************************************/
      case MEMORY_ADD_ENTRY :
        current_entry = (struct file_descriptive_entry *) data;
        if(current_entry == NULL)
          return;

        /* Ajoute à la fin de la liste */
        if(first_entry == NULL)
          first_entry = current_entry;
        else
          last_entry->next = current_entry;
        last_entry = current_entry;
        nb_entry++;
        break;

      case MEMORY_GET_ENTRY_NB :
        *((int *)data) = nb_entry;
        break;

      case MEMORY_GET_ENTRY :
        index = *((int *) data);
        *((struct file_descriptive_entry **) value) = NULL;
        if(index <= 0 || index > nb_entry)
          return;

        /* Recherche le index-nth entry */
        for(i=1, current_entry = first_entry; i<index; i++)
          current_entry = current_entry->next;
        *((struct file_descriptive_entry **) value) = current_entry;
        break;

      case MEMORY_BUILD_ENTRY_TAB :
        /* Libération */
        if(tab_entry)
          free(tab_entry);

        /* Allocation */
        tab_entry = (struct file_descriptive_entry **) calloc(1+nb_entry,sizeof(struct file_descriptive_entry *));
        if(tab_entry == NULL)
          break;

        /* Remplissage */
        for(i=0, current_entry = first_entry; current_entry; i++,current_entry=current_entry->next)
          tab_entry[i] = current_entry;
        break;

      case MEMORY_REMOVE_ENTRY :
        delete_entry = (struct file_descriptive_entry *) data;
        if(delete_entry == NULL)
          break;

        /* Cas particulier : 1 seule structure */
        if(nb_entry == 1 && first_entry == delete_entry)
          {
            first_entry = NULL;
            last_entry = NULL;
            nb_entry = 0;
          }
        else if(first_entry == delete_entry)
          {
            /* En 1ère position */
            first_entry = first_entry->next;
            memmove(&tab_entry[0],&tab_entry[1],(nb_entry-1)*sizeof(struct file_descriptive_entry *));
            nb_entry--;
          }
        else if(last_entry == delete_entry)
          {
            /* En dernière position */
            tab_entry[nb_entry-2]->next = NULL;
            last_entry = tab_entry[nb_entry-2];
            nb_entry--;
          }
        else
          {
            /* Au milieu */
            for(i=0; i<nb_entry; i++)
              if(tab_entry[i] == delete_entry)
                {
                  tab_entry[i-1]->next = tab_entry[i]->next;
                  memmove(&tab_entry[i],&tab_entry[i+1],(nb_entry-i-1)*sizeof(struct file_descriptive_entry *));
                  nb_entry--;
                  break;
                }
          }
        break;

      case MEMORY_FREE_ENTRY :
        for(current_entry = first_entry; current_entry; )
          {
            next_entry = current_entry->next;
            mem_free_entry(current_entry);
            current_entry = next_entry;
          }
        if(tab_entry)
          free(tab_entry);
        nb_entry = 0;
        first_entry = NULL;
        last_entry = NULL;
        tab_entry = NULL;
        break;

      /***********************************************/
      /*  Liste chainée des structure entry : SubDir */
      /***********************************************/
      case MEMORY_ADD_DIRECTORY :
        current_directory = (struct file_descriptive_entry *) data;
        if(current_directory == NULL)
          return;

        /* Ajoute à la fin de la liste */
        if(first_directory == NULL)
          first_directory = current_directory;
        else
          last_directory->next = current_directory;
        last_directory = current_directory;
        nb_directory++;
        break;

      case MEMORY_GET_DIRECTORY_NB :
        *((int *)data) = nb_directory;
        break;

      case MEMORY_GET_DIRECTORY :
        index = *((int *) data);
        *((struct file_descriptive_entry **) value) = NULL;
        if(index <= 0 || index > nb_directory)
          return;

        /* Recherche le index-nth directory */
        for(i=1, current_directory = first_directory; i<index; i++)
          current_directory = current_directory->next;
        *((struct file_descriptive_entry **) value) = current_directory;
        break;

      case MEMORY_BUILD_DIRECTORY_TAB :
        /* Libération */
        if(tab_directory)
          free(tab_directory);

        /* Allocation */
        tab_directory = (struct file_descriptive_entry **) calloc(1+nb_directory,sizeof(struct file_descriptive_entry *));
        if(tab_directory == NULL)
          break;

        /* Remplissage */
        for(i=0, current_directory = first_directory; current_directory; i++,current_directory=current_directory->next)
          tab_directory[i] = current_directory;
        break;

      case MEMORY_REMOVE_DIRECTORY :
        delete_directory = (struct file_descriptive_entry *) data;
        if(delete_directory == NULL)
          break;

        /* Cas particulier : 1 seule structure */
        if(nb_directory == 1 && first_directory == delete_directory)
          {
            first_directory = NULL;
            last_directory = NULL;
            nb_directory = 0;
          }
        else if(first_directory == delete_directory)
          {
            /* En 1ère position */
            first_directory = first_directory->next;
            memmove(&tab_directory[0],&tab_directory[1],(nb_directory-1)*sizeof(struct file_descriptive_entry *));
            nb_directory--;
          }
        else if(last_directory == delete_directory)
          {
            /* En dernière position */
            tab_directory[nb_directory-2]->next = NULL;
            last_directory = tab_directory[nb_directory-2];
            nb_directory--;
          }
        else
          {
            /* Au milieu */
            for(i=0; i<nb_directory; i++)
              if(tab_directory[i] == delete_directory)
                {
                  tab_directory[i-1]->next = tab_directory[i]->next;
                  memmove(&tab_directory[i],&tab_directory[i+1],(nb_directory-i-1)*sizeof(struct file_descriptive_entry *));
                  nb_directory--;
                  break;
                }
          }
        break;

      case MEMORY_FREE_DIRECTORY :
        for(current_directory = first_directory; current_directory; )
          {
            next_directory = current_directory->next;
            mem_free_entry(current_directory);
            current_directory = next_directory;
          }
        if(tab_directory)
          free(tab_directory);
        nb_directory = 0;
        first_directory = NULL;
        last_directory = NULL;
        tab_directory = NULL;
        break;

      /*******************************************/
      /*  Liste chainée des structure file_path  */
      /*******************************************/
      case MEMORY_ADD_FILE :
        path = (char *) data;

        /* Allocation mémoire */
        current_filepath = (struct file_path *) calloc(1,sizeof(struct file_path));
        if(current_filepath == NULL)
          return;
        current_filepath->path = strdup(path);
        if(current_filepath->path == NULL)
          {
            free(current_filepath);
            return;
          }

        /* Ajoute à la fin de la liste */
        if(first_filepath == NULL)
          first_filepath = current_filepath;
        else
          last_filepath->next = current_filepath;
        last_filepath = current_filepath;
        nb_filepath++;
        break;

      case MEMORY_GET_FILE_NB :
        *((int *)data) = nb_filepath;
        break;

      case MEMORY_GET_FILE :
        index = *((int *) data);
        *((struct file_path **) value) = NULL;
        if(index <= 0 || index > nb_filepath)
          return;

        /* Recherche le index-nth entry */
        for(i=1, current_filepath = first_filepath; i<index; i++)
          current_filepath = current_filepath->next;
        *((char **) value) = current_filepath->path;
        break;

      case MEMORY_FREE_FILE :
        for(current_filepath = first_filepath; current_filepath; )
          {
            next_filepath = current_filepath->next;
            mem_free_filepath(current_filepath);
            current_filepath = next_filepath;
          }
        nb_filepath = 0;
        first_filepath = NULL;
        last_filepath = NULL;
        break;

      /***************************************/
      /*  Liste chainée des structure error  */
      /***************************************/
      case MEMORY_ADD_ERROR :
        message = (char *) data;

        /* Allocation mémoire */
        current_error = (struct error *) calloc(1,sizeof(struct error));
        if(current_error == NULL)
          return;
        current_error->message = strdup(message);
        if(current_error->message == NULL)
          {
            free(current_error);
            return;
          }

        /* Ajoute à la fin de la liste */
        if(first_error == NULL)
          first_error = current_error;
        else
          last_error->next = current_error;
        last_error = current_error;
        nb_error++;
        break;

      case MEMORY_GET_ERROR_NB :
        *((int *)data) = nb_error;
        break;

      case MEMORY_GET_ERROR :
        index = *((int *) data);
        *((struct error **) value) = NULL;
        if(index <= 0 || index > nb_error)
          return;

        /* Recherche le index-nth entry */
        for(i=1, current_error = first_error; i<index; i++)
          current_error = current_error->next;
        *((struct error **) value) = current_error;
        break;

      case MEMORY_FREE_ERROR :
        for(current_error = first_error; current_error; )
          {
            next_error = current_error->next;
            if(current_error->message)
              free(current_error->message);
            free(current_error);
            current_error = next_error;
          }
        nb_error = 0;
        first_error = NULL;
        last_error = NULL;
        break;

      default :
        break;
    }
}


/**********************************************************/
/*  mem_free_param() :  Libération de la structure Param. */
/**********************************************************/
void mem_free_param(struct parameter *param)
{
  if(param)
    {
      if(param->image_file_path)
        free(param->image_file_path);

      if(param->prodos_file_path)
        free(param->prodos_file_path);

      if(param->prodos_folder_path)
        free(param->prodos_folder_path);

      if(param->output_directory_path)
        free(param->output_directory_path);

      if(param->file_path)
        free(param->file_path);

      if(param->folder_path)
        free(param->folder_path);

      if(param->new_file_name)
        free(param->new_file_name);

      if(param->new_folder_name)
        free(param->new_folder_name);

      if(param->new_volume_name)
        free(param->new_volume_name);

      if(param->new_file_path)
        free(param->new_file_path);

      if(param->new_folder_path)
        free(param->new_folder_path);

      free(param);
    }
}

/***********************************************************************/
