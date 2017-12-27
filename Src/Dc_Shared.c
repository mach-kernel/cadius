/***********************************************************************/
/*                                                                     */
/*  Dc_Shared.c : Module pour la bibliothèque de fonctions génériques. */
/*                                                                     */
/***********************************************************************/
/*  Auteur : Olivier ZARDINI  *  Brutal Deluxe Software  *  Janv 2011  */
/***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <malloc.h>

#include "Dc_Shared.h"
#include "Dc_OS.h"
#include "Dc_Memory.h"

/***************************************************************/
/*  LoadBinaryFile() :  Récupération des données d'un fichier. */
/***************************************************************/
unsigned char *LoadBinaryFile(char *file_path, int *data_length_rtn)
{
  FILE *fd;
  int nb_read, file_size;
  unsigned char *data;

  /* Init */
  *data_length_rtn = 0;

  /* Ouverture du fichier */
  fd = fopen(file_path,"rb");
  if(fd == NULL)
    return(NULL);

  /* Taille du fichier */
  fseek(fd,0L,SEEK_END);
  file_size = ftell(fd);
  fseek(fd,0L,SEEK_SET);
   
  /* Allocation mémoire */
  data = (unsigned char *) calloc(1,file_size+1);
  if(data == NULL)
    {
      fclose(fd);
      return(NULL);
    }

  /* Lecture des données */
  nb_read = fread(data,1,file_size,fd);
  if(nb_read != file_size)
    {
      free(data);
      fclose(fd);
      return(NULL);
    }
  data[nb_read] = '\0';

  /* Fermeture du fichier */
  fclose(fd);

  /* Renvoi les données et la taille */
  *data_length_rtn = nb_read;
  return(data);
}


/*************************************************************/
/*  LoadTextFile() :  Récupération des données d'un fichier. */
/*************************************************************/
unsigned char *LoadTextFile(char *file_path, int *data_length_rtn)
{
  FILE *fd;
  int nb_read, file_size;
  unsigned char *data;

  /* Ouverture du fichier */
  fd = fopen(file_path,"r");
  if(fd == NULL)
    return(NULL);

  /* Taille du fichier */
  fseek(fd,0L,SEEK_END);
  file_size = ftell(fd);
  fseek(fd,0L,SEEK_SET);
   
  /* Allocation mémoire */
  data = (unsigned char *) calloc(1,file_size+10);
  if(data == NULL)
    {
      fclose(fd);
      return(NULL);
    }

  /* Lecture des données */
  nb_read = fread(data,1,file_size,fd);
  if(nb_read < 0)
    {
      free(data);
      fclose(fd);
      return(NULL);
    }
  data[nb_read] = '\0';

  /* Fermeture du fichier */
  fclose(fd);

  /* Renvoi les données et la taille */
  *data_length_rtn = nb_read;
  return(data);
}


/*************************************************************/
/*  Get24bitValue() :  Décode une valeur codée sur 3 octets. */
/*************************************************************/
int Get24bitValue(unsigned char *data, int offset)
{
  return(data[offset] + 256*data[offset+1] + 65536*data[offset+2]);
}


/************************************************************/
/*  GetWordValue() :  Décode une valeur codée sur 2 octets. */
/************************************************************/
int GetWordValue(unsigned char *data, int offset)
{
  return(data[offset] + 256*data[offset+1]);
}


/***********************************************************/
/*  GetByteValue() :  Décode une valeur codée sur 1 octet. */
/***********************************************************/
int GetByteValue(unsigned char *data, int offset)
{
  return(data[offset]);
}


/***********************************************************/
/*  SetWordValue() :  Place une valeur codée sur 2 octets. */
/***********************************************************/
void SetWordValue(unsigned char *data, int offset, WORD value)
{
  data[offset] = (unsigned char) (value & 0x00FF);
  data[offset+1] = (unsigned char) ((value >> 8) & 0x00FF);
}


/************************************************************/
/*  Set24bitValue() :  Place une valeur codée sur 3 octets. */
/************************************************************/
void Set24bitValue(unsigned char *data, int offset, int value)
{
  data[offset] = (unsigned char) (value & 0x000000FF);
  data[offset+1] = (unsigned char) ((value >> 8) & 0x000000FF);
  data[offset+2] = (unsigned char) ((value >> 16) & 0x000000FF);
}


/************************************************************/
/*  SetDWordValue() :  Place une valeur codée sur 4 octets. */
/************************************************************/
void SetDWordValue(unsigned char *data, int offset, DWORD value)
{
  data[offset] = (unsigned char) (value & 0x000000FF);
  data[offset+1] = (unsigned char) ((value >> 8) & 0x000000FF);
  data[offset+2] = (unsigned char) ((value >> 16) & 0x000000FF);
  data[offset+3] = (unsigned char) ((value >> 24) & 0x000000FF);
}


/************************************************************/
/*  CreateBinaryFile() :  Création d'un fichier sur disque. */
/************************************************************/
int CreateBinaryFile(char *file_path, unsigned char *data, int length)
{
  int nb_write;
  FILE *fd;

  /* Suppression du fichier */
  my_DeleteFile(file_path);

  /* Création du fichier */
  fd = fopen(file_path,"wb");
  if(fd == NULL)
    return(1);

  /* Ecriture des données */
  nb_write = fwrite(data,1,length,fd);
  if(nb_write != length)
    {
      fclose(fd);
      return(2);
    } 

  /* Fermeture du fichier */
  fclose(fd);

  /* OK */
  return(0);
}


/***************************************************************/
/*  CreateTextFile() :  Création d'un fichier Text sur disque. */
/***************************************************************/
int CreateTextFile(char *file_path, unsigned char *data, int length)
{
  int nb_write;
  FILE *fd;

  /* Suppression du fichier */
  my_DeleteFile(file_path);

  /* Création du fichier */
  fd = fopen(file_path,"w");
  if(fd == NULL)
    return(1);

  /* Ecriture des données */
  nb_write = fwrite(data,1,length,fd);
  if(nb_write < length)
    {
      fclose(fd);
      return(2);
    } 

  /* Fermeture du fichier */
  fclose(fd);

  /* OK */
  return(0);
}


/********************************************************/
/*  BuildFileList() :  Construit la liste des fichiers. */
/********************************************************/
char **BuildFileList(char *hierarchy, int *nb_file_rtn)
{
  int i, result;
  char *file_path;
  char **tab_file;
  int nb_file;
  char hierarchy_path[2048];
  char folder_path[2048];

  /* Init */
  *nb_file_rtn = 0;
  my_Memory(MEMORY_FREE_FILE,NULL,NULL);

  /** Y a t'il un * dans le chemin du fichier ? **/
  if(strchr(hierarchy,'*') == NULL)
    {
      /* Un seul fichier */
      tab_file = (char **) calloc(1,sizeof(char *));
      if(tab_file == NULL)
        {
          return(NULL);
        }
      tab_file[0] = strdup(hierarchy);
      if(tab_file[0] == NULL)
        {
          free(tab_file);
          return(NULL);
        }
      *nb_file_rtn = 1;
      return(tab_file);
    }

  /** Hiérachie de fichier à traiter **/
  strcpy(hierarchy_path,hierarchy);
  CleanHierarchie(hierarchy_path);

  /** Répertoire de départ **/
  strcpy(folder_path,hierarchy_path);
  for(i=0; i<(int)strlen(folder_path); i++)
    if(folder_path[i] == '*')
      {
        folder_path[i] = '\0';
        break;
      }
  for(i=strlen(folder_path); i>=0; i--)
    if(folder_path[i] == '\\' || folder_path[i] == '/')
      {
        folder_path[i+1] = '\0';
        break;
      }

  /** Recherche des fichiers **/
  result = GetFolderFiles(folder_path,hierarchy_path);
  if(result)
    {
      my_Memory(MEMORY_FREE_FILE,NULL,NULL);
      return(NULL);
    }

  /** Création du tableau **/
  my_Memory(MEMORY_GET_FILE_NB,&nb_file,NULL);

  /* Allocation mémoire */
  tab_file = (char **) calloc(nb_file,sizeof(char *));
  if(tab_file == NULL)
    {
      my_Memory(MEMORY_FREE_FILE,NULL,NULL);
      return(NULL);
    }

  /* Remplissage du tableau */
  for(i=1; i<=nb_file; i++)
    {
      my_Memory(MEMORY_GET_FILE,&i,&file_path);
      tab_file[i-1] = strdup(file_path);
      if(tab_file[i-1] == NULL)
        {
          my_Memory(MEMORY_FREE_FILE,NULL,NULL);
          return(NULL);
        }
    }

  /* Libération mémoire */
  my_Memory(MEMORY_FREE_FILE,NULL,NULL);

  /* Renvoi les valeurs */
  *nb_file_rtn = nb_file;
  return(tab_file);
}


/***********************************************************************/
/*  MatchHierarchie() : Indique si un nom appartient à une hiérarchie. */
/***********************************************************************/
int MatchHierarchie(char *name, char *hierarchie)
{
  int i,j,k;
  int length;
  char *hier_ptr;
  char *name_ptr;
  int result;
  int count;
  int offset;
  char buffer[BUFFER_SIZE];

  /*** On parcours les deux chaînes ***/
  for(i=0,j=0; i<(int)strlen(hierarchie); i++)
    {
      if(hierarchie[i] != '*')
        {
          if(toupper(hierarchie[i]) !=  toupper(name[j]))
            return(0);
          j++;
        }
      else if(hierarchie[i] == '?')
        j++;
      else
        {
          /* Si '*' dernier caractère de la chaîne => OK */
          if(hierarchie[i+1] == '\0')
            return(1);

          /** S'il ne reste pas d'autre '*' : On compare la fin **/
          hier_ptr = strchr(&hierarchie[i+1],'*');
          if(hier_ptr == NULL)
            {
              length = strlen(&hierarchie[i+1]);
              if((int)strlen(&name[j]) < length)
                return(0);
              if(!mh_stricmp(&name[strlen(name)-length],&hierarchie[i+1]))
                return(1);
              else
                return(0);
            }

          /** On compte le nb d'occurences de la partie entre les deux '*' dans name **/
          strncpy(buffer,&hierarchie[i+1],hier_ptr-&hierarchie[i+1]);
          buffer[hier_ptr-&hierarchie[i+1]] = '\0';
          length = strlen(buffer);
          for(count = 0,offset = j;;count++)
            {
              name_ptr = mh_stristr(&name[offset],buffer);
              if(name_ptr == NULL)
                break;
              offset = (name_ptr - name) + 1;
            }
          /* Si aucune occurence => pas de matching */
          if(count == 0)
            return(0);

          /** On lance la récursivité sur toutes les occurences trouvées **/
          for(k=0,offset=j; k<count; k++)
            {
              name_ptr = mh_stristr(&name[offset],buffer);
              result = MatchHierarchie(name_ptr+length,&hierarchie[i+1+length]);
              if(result)
                return(1);
              offset = (name_ptr - name) + 1;
            }
          return(0);
        }
    }

  /* Est t'on également à la fin de name ? */
  if(j != (int)strlen(name))
    return(0);

  /* On est arrivé au bout : OK */
  return(1);
}


/****************************************************************/
/* CleanHierarchie() : Supprime les '*' en trop dans la chaîne. */
/****************************************************************/
void CleanHierarchie(char *hierarchie)
{
  int i,j;
  char buffer[BUFFER_SIZE];

  for(i=0,j=0; i<=(int)strlen(hierarchie); i++)
    {
      if(hierarchie[i] != '*')
        buffer[j++] = hierarchie[i];
      else
        {
          /* On ne laisse pas plusieurs '*' consécutifs */
          if(j == 0)
            buffer[j++] = hierarchie[i];
          else
            {
              if(buffer[j-1] == '*')
                continue;
              else
                buffer[j++] = hierarchie[i];
            }
        }
    }

  /* On recopie la chaine nettoyée */
  strcpy(hierarchie,buffer);
}


/***********************************************************************/
/*  mh_stristr() : Recherche l'occurence d'une chaîne dans une autre.  */
/***********************************************************************/
char *mh_stristr(char *name, char *hierarchie)
{
  int i;
  int length_n = strlen(name);
  int length_h = strlen(hierarchie);

  /* On élimine les cas extrêmes */
  if(length_n < length_h)
    return(NULL);

  /** On parcours la chaîne 'name' afin de localiser la chaine 'hierarchie' **/
  for(i=0; i<length_n-length_h+1; i++)
    {
      /* La recherche ne tient pas compte de la casse */
      if(!my_strnicmp(&name[i],hierarchie,length_h))
        return(&name[i]);
    }

  /* On a rien trouvé */
  return(NULL);
}


/********************************************************************/
/*  mh_stricmp() : Compare deux chaînes de caractères avec les '?'. */
/********************************************************************/
int mh_stricmp(char *string_n, char *string_h)
{
  int i;
  int length_n = strlen(string_n);
  int length_h = strlen(string_h);

  /* On élimine immédiatement les cas défavorables */
  if(length_n != length_h)
    return(1);

  /** On compare tous les caractères avec gestion du '?' **/
  for(i=0; i<length_n; i++)
    {
      if(string_h[i] == '?')
        continue;
      else if(toupper(string_n[i]) != toupper(string_h[i]))
        return(1);
    }

  /* On a deux chaînes identiques */
  return(0);
}


/*****************************************************************************/
/*  BuildUniqueListFromFile() :  Récupère la liste des valeurs d'un fichier. */
/*****************************************************************************/
char **BuildUniqueListFromFile(char *file_path, int *nb_value)
{
  FILE *fd;
  int i, found, nb_line, line_length;
  char **tab;
  char buffer_line[1024];

  /* Ouverture du fichier */
  fd = fopen(file_path,"r");
  if(fd == NULL)
    return(NULL);

  /* Compte le nombre de lignes */
  nb_line = 0;
  fseek(fd,0L,SEEK_SET);
  while(fgets(buffer_line,1024-1,fd))
    nb_line++;

  /* Allocation du tableau */
  tab = (char **) calloc(nb_line,sizeof(char *));
  if(tab == NULL)
    {
      fclose(fd);
      return(NULL);
    }

  /** Lecture du fichier **/
  nb_line = 0;
  fseek(fd,0L,SEEK_SET);
  while(fgets(buffer_line,1024-1,fd))
    {
      /** Traitement préliminaire de nettoyage **/
      line_length = strlen(buffer_line);
      if(line_length < 2)              /* Ligne vide */
        continue;
      if(buffer_line[line_length-1] == '\n')
        buffer_line[line_length-1] = '\0';  /* On vire le \n final */

      /** Stocke la valeur **/
      for(i=0,found=0; i<nb_line; i++)
        if(!my_stricmp(tab[i],buffer_line))
          {
            found = 1;
            break;
          }
      if(found == 0)
        {
          tab[nb_line] = strdup(buffer_line);
          if(tab[nb_line] == NULL)
            {
              for(i=0;i<nb_line;i++)
                free(tab[i]);
              free(tab);
              fclose(fd);
              return(NULL);
            }
          nb_line++;
        }
    }

  /* Fermeture du fichier */
  fclose(fd);

  /* Renvoi le tableau */
  *nb_value = nb_line;
  return(tab);
}


/**************************************************************************/
/*  GetContainerNumber() :  Détermine le nombre de container nécessaires. */
/**************************************************************************/
int GetContainerNumber(int data_size, int container_size)
{
  int nb_container;

  /* Cas particulier */
  if(data_size == 0)
    return(0);
  if(data_size < container_size)
    return(1);

  /* Nombre */
  nb_container = data_size / container_size;

  /* Ajustement : On en ajote 1 pour ce qu'il reste */
  if(nb_container * container_size != data_size)
    nb_container++;

  /* Renvi le nombre */
  return(nb_container);
}


/*****************************************************************/
/*  mem_free_filepath() :  Libération de la structure file_path. */
/*****************************************************************/
void mem_free_filepath(struct file_path *current_filepath)
{
  if(current_filepath)
    {
      if(current_filepath->path)
        free(current_filepath->path);

      free(current_filepath);
    }
}


/********************************************************/
/*  mem_free_list() :  Libération mémoire d'un tableau. */
/********************************************************/
void mem_free_list(int nb_element, char **element_tab)
{
  int i;

  if(element_tab)
    {
      for(i=0; i<nb_element; i++)
        if(element_tab[i])
          free(element_tab[i]);

      free(element_tab);
    }
}

/***********************************************************************/
