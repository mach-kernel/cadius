/***********************************************************************/
/*                                                                     */
/*   Prodos_Source.c : Module pour la gestion des fichiers Source.     */
/*                                                                     */
/***********************************************************************/
/*  Auteur : Olivier ZARDINI  *  Brutal Deluxe Software  *  Dec 2011   */
/***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

#include "Dc_Shared.h"
#include "Dc_OS.h"
#include "Prodos_source.h"

#define TYPE_LINE_CODE    1
#define TYPE_LINE_COMMENT 2
#define TYPE_LINE_EMPTY   3
#define TYPE_LINE_DATA    4

#define ERR_MEMORY_ALLOC   "Memory allocation impossible"
#define ERR_LINE_FORMAT    "Wrong line format"

struct line
{
  int type;

  char *label;
  char *opcode;
  char *operand;
  char *comment;

  int number;       /* Numéro de la ligne dans le fichier */
  int length;       /* Taille de la ligne */

  struct line *next;
};

static char *BuildIndentBuffer(int,struct line **);
static char *BuildOutdentBuffer(int,struct line **);
static struct line **BuildLineTab(unsigned char *,int,int *);
static struct line *BuildOneLine(char *,int);
static int GetLineValue(char *,int,char **);
static void mem_free_line(struct line *);

/******************************************************************/
/*  ClearFileHighBit() :  Met à 0 le bit 7 des octets du fichier. */
/******************************************************************/
int ClearFileHighBit(char *file_path)
{
  int i, result;
  int length;
  unsigned char *data;

  /** Chargement en mémoire du fichier **/
  data = LoadBinaryFile(file_path,&length);
  if(data == NULL)
    {
      printf("  Error : Impossible to open file '%s'.\n",file_path);
      return(1);
    }

  /** Clear Bit 7 (on ne touche pas aux 0x20) **/
  for(i=0; i<length; i++)
    if(data[i] != 0x20)
      data[i] &= 0x7F;

  /** Conversion des 0D en 0A **/
  for(i=0; i<length; i++)
    if(data[i] == 0x0D)
      data[i] = 0x0A;

  /** Ecriture du fichier Texte (0A devient 0D 0A) **/
  result = CreateTextFile(file_path,data,length);
  if(result)
    {
      printf("  Error : Impossible to update file '%s'.\n",file_path);
      return(1);
    }

  /* Libération mémoire */
  free(data);

  /* OK */
  return(0);
}


/****************************************************************/
/*  SetFileHighBit() :  Met à 1 le bit 7 des octets du fichier. */
/****************************************************************/
int SetFileHighBit(char *file_path)
{
  int i, result;
  int length;
  unsigned char *data;

  /** Chargement en mémoire du fichier Texte **/
  data = LoadTextFile(file_path,&length);
  if(data == NULL)
    {
      printf("  Error : Impossible to open file '%s'.\n",file_path);
      return(1);
    }

  /** On convertit les 0A en 0D **/
  for(i=0; i<length; i++)
    if(data[i] == 0x0A)
      data[i] = 0x0D;

  /** Set Bit 7 **/
  for(i=0; i<length; i++)
    {
      if(data[i] == '\t')
        data[i] = 0xA0;
      else if(data[i] != 0x20)
        data[i] |= 0x80;
    }
    
  /** Ecriture du fichier **/
  result = CreateBinaryFile(file_path,data,length);
  if(result)
    {
      printf("  Error : Impossible to update file '%s'.\n",file_path);
      return(1);
    }

  /* Libération mémoire */
  free(data);

  /* OK */
  return(0);
}


/************************************************************/
/*  IndentFile() :  Indente le contenu d'un fichier Source. */
/************************************************************/
int IndentFile(char *file_path)
{
  int i, result, length_src, length_dst;
  unsigned char *data;
  int nb_line;
  struct line **tab_line;

  /** Chargement en mémoire du fichier **/
  data = LoadTextFile(file_path,&length_src);
  if(data == NULL)
    {
      printf("  Error : Impossible to open file '%s'.\n",file_path);
      return(1);
    }

  /** Lecture des lignes du fichier **/
  tab_line = BuildLineTab(data,length_src,&nb_line);
  if(tab_line == NULL)
    {
      free(data);
      return(2);
    }

  /* Libération du buffer */
  free(data);

  /** Création du buffer des lignes indentées **/
  data = BuildIndentBuffer(nb_line,tab_line);

  /* Libération mémoire */
  for(i=0; i<nb_line; i++)
    mem_free_line(tab_line[i]);
  free(tab_line);

  /* Erreur */
  if(data == NULL)
    return(3);

  /** Ecriture du fichier **/
  length_dst = strlen(data);
  result = CreateTextFile(file_path,data,length_dst);
  if(result)
    {
      printf("  Error : Impossible to update file '%s'.\n",file_path);
      return(4);
    }

  /* Libération mémoire */
  free(data);

  /* OK */
  return(0);
}


/****************************************************************/
/*  OutdentFile() :  Dé-Indente le contenu d'un fichier Source. */
/****************************************************************/
int OutdentFile(char *file_path)
{
  int i, result, length;
  unsigned char *data;
  int nb_line;
  struct line **tab_line;

  /** Chargement en mémoire du fichier **/
  data = LoadTextFile(file_path,&length);
  if(data == NULL)
    {
      printf("  Error : Impossible to open file '%s'.\n",file_path);
      return(1);
    }

  /** Lecture des lignes du fichier **/
  tab_line = BuildLineTab(data,length,&nb_line);
  if(tab_line == NULL)
    {
      free(data);
      return(2);
    }

  /* Libération du buffer */
  free(data);

  /** Création du buffer des lignes dé-indentées **/
  data = BuildOutdentBuffer(nb_line,tab_line);

  /* Libération mémoire */
  for(i=0; i<nb_line; i++)
    mem_free_line(tab_line[i]);
  free(tab_line);

  /* Erreur */
  if(data == NULL)
    return(3);

  /** Ecriture du fichier **/
  result = CreateTextFile(file_path,data,strlen(data));
  if(result)
    {
      printf("  Error : Impossible to update file '%s'.\n",file_path);
      return(4);
    }

  /* Libération mémoire */
  free(data);

  /* OK */
  return(0);
}


/************************************************************************/
/*  BuildIndentBuffer() :  Construction du buffer des lignes indentées. */
/************************************************************************/
static char *BuildIndentBuffer(int nb_line, struct line **tab_line)
{
  int i, j, length, offset;
  int label_length, opcode_length, operand_length, operand_data_length, comment_length, line_length;
  unsigned char *data;

  /** Détermine la taille max des zones **/
  label_length = 10;
  opcode_length = 4;
  operand_length = 10;
  operand_data_length = 10;
  comment_length = 10;
  line_length = 0;
  for(i=0; i<nb_line; i++)
    {
      if(tab_line[i]->type == TYPE_LINE_CODE || tab_line[i]->type == TYPE_LINE_DATA)
        {
          if((int)strlen(tab_line[i]->label) > label_length)
            label_length = strlen(tab_line[i]->label);

          if((int)strlen(tab_line[i]->opcode) > opcode_length && strlen(tab_line[i]->operand) > 0)
            opcode_length = strlen(tab_line[i]->opcode);

          if((int)strlen(tab_line[i]->comment) > comment_length)
            comment_length = strlen(tab_line[i]->comment);
        }

      if(tab_line[i]->type == TYPE_LINE_CODE)
        {
          if((int)strlen(tab_line[i]->operand) > operand_length)
            operand_length = strlen(tab_line[i]->operand);
        }
      else if(tab_line[i]->type == TYPE_LINE_DATA)
        {
          if((int)strlen(tab_line[i]->operand) > operand_data_length)
            operand_data_length = strlen(tab_line[i]->operand);
        }

      if(line_length < tab_line[i]->length)
        line_length = tab_line[i]->length;
    }
  operand_data_length = (operand_data_length > operand_length) ? operand_data_length : operand_length;
  if(line_length < (label_length + opcode_length + operand_data_length + comment_length))
    line_length = (label_length + opcode_length + operand_data_length + comment_length);

  /* Allocation mémoire du fichier */
  data = (char *) calloc(nb_line,line_length+1);
  if(data == NULL)
    {
      printf("  Error : Impossible to allocate memory to process the file.\n");
      return(NULL);
    }

  /**********************************/
  /** Création du fichier résultat **/
  /**********************************/
  for(i=0,offset=0; i<nb_line; i++)
    {
      if(tab_line[i]->type == TYPE_LINE_EMPTY)
        data[offset++] = '\n';
      else if(tab_line[i]->type == TYPE_LINE_COMMENT)
        {
          memcpy(&data[offset],tab_line[i]->comment,strlen(tab_line[i]->comment));
          offset += strlen(tab_line[i]->comment);
          data[offset++] = '\n';
        }
      else if(tab_line[i]->type == TYPE_LINE_CODE || tab_line[i]->type == TYPE_LINE_DATA)
        {
          /** Label **/
          length = strlen(tab_line[i]->label);
          memcpy(&data[offset],tab_line[i]->label,length);
          offset += length;
          
          /* Separator */
          if(strlen(tab_line[i]->opcode) + strlen(tab_line[i]->operand) + strlen(tab_line[i]->comment) == 0)
            {
              data[offset++] = '\n';
              continue;
            }
          for(j=length; j<label_length; j++)
            data[offset++] = ' ';
          data[offset++] = ' ';
          data[offset++] = ' ';

          /** Opcode **/
          length = strlen(tab_line[i]->opcode);
          memcpy(&data[offset],tab_line[i]->opcode,length);
          offset += length;

          /* Separator */
          if(strlen(tab_line[i]->operand) + strlen(tab_line[i]->comment) == 0)
            {
              data[offset++] = '\n';
              continue;
            }
          for(j=length; j<opcode_length; j++)
            data[offset++] = ' ';
          data[offset++] = ' ';
          data[offset++] = ' ';

          /** Operand **/
          length = strlen(tab_line[i]->operand);
          memcpy(&data[offset],tab_line[i]->operand,length);
          offset += length;

          /* Separator */
          if(strlen(tab_line[i]->comment) == 0)
            {
              data[offset++] = '\n';
              continue;
            }
          if(tab_line[i]->type == TYPE_LINE_CODE)
            {
              /* L'opcode a débordé sur l'operand, qui n'était pas là */
              if(strlen(tab_line[i]->operand) == 0 && (int) strlen(tab_line[i]->opcode) > opcode_length)
                {
                  for(j=length+strlen(tab_line[i]->opcode)-opcode_length; j<operand_length; j++)
                    data[offset++] = ' ';
                }
              else
                {
                  for(j=length; j<operand_length; j++)
                    data[offset++] = ' ';
                }
            }
          else if(tab_line[i]->type == TYPE_LINE_DATA && length < operand_length)
            {
              /* On met le commentaire des Data au même niveau que celui du code */
              for(j=length; j<operand_length; j++)
                data[offset++] = ' ';              
            }
          data[offset++] = ' ';
          data[offset++] = ' ';

          /** Comment **/
          length = strlen(tab_line[i]->comment);
          memcpy(&data[offset],tab_line[i]->comment,length);
          offset += length;

          /* Ligne suivante */
          data[offset++] = '\n';
        }
    }
  data[offset] = '\0';

  /* Renvoi le buffer */
  return(data);
}


/****************************************************************************/
/*  BuildOutdentBuffer() :  Construction du buffer des lignes dé-indentées. */
/****************************************************************************/
static char *BuildOutdentBuffer(int nb_line, struct line **tab_line)
{
  int i, length, offset, line_length;
  unsigned char *data;

  /** Détermine la taille max de la ligne **/
  line_length = 0;
  for(i=0; i<nb_line; i++)
    {
      if(tab_line[i]->type == TYPE_LINE_CODE || tab_line[i]->type == TYPE_LINE_DATA)
        length = strlen(tab_line[i]->label) + 1 + strlen(tab_line[i]->opcode) + 1 + strlen(tab_line[i]->operand) + 1 + strlen(tab_line[i]->comment);
      else if(tab_line[i]->type == TYPE_LINE_COMMENT)
        length = strlen(tab_line[i]->comment) + 3;
      else
        length = 1;

      if(line_length < length)
        line_length = length;
    }

  /* Allocation mémoire du fichier */
  data = (char *) calloc(nb_line,line_length+1);
  if(data == NULL)
    {
      printf("  Error : Impossible to allocate memory to process the file.\n");
      return(NULL);
    }

  /**********************************/
  /** Création du fichier résultat **/
  /**********************************/
  for(i=0,offset=0; i<nb_line; i++)
    {
      if(tab_line[i]->type == TYPE_LINE_EMPTY)
        data[offset++] = '\n';
      else if(tab_line[i]->type == TYPE_LINE_COMMENT)
        {
          memcpy(&data[offset],tab_line[i]->comment,strlen(tab_line[i]->comment));
          offset += strlen(tab_line[i]->comment);
          data[offset++] = '\n';
        }
      else if(tab_line[i]->type == TYPE_LINE_CODE || tab_line[i]->type == TYPE_LINE_DATA)
        {
          /** Label **/
          length = strlen(tab_line[i]->label);
          memcpy(&data[offset],tab_line[i]->label,length);
          offset += length;
          
          /* Separator */
          data[offset++] = '\t';

          /** Opcode **/
          length = strlen(tab_line[i]->opcode);
          memcpy(&data[offset],tab_line[i]->opcode,length);
          offset += length;

          /* Separator */
          data[offset++] = '\t';

          /** Operand **/
          length = strlen(tab_line[i]->operand);
          memcpy(&data[offset],tab_line[i]->operand,length);
          offset += length;

          /* Separator */
          data[offset++] = '\t';

          /** Comment **/
          length = strlen(tab_line[i]->comment);
          memcpy(&data[offset],tab_line[i]->comment,length);
          offset += length;

          /* Compact EndOfLine */
          while(offset > 0 && data[offset-1] == '\t')
            offset--;

          /* Ligne suivante */
          data[offset++] = '\n';
        }
    }
  data[offset] = '\0';

  /* Renvoi le buffer */
  return(data);
}


/*********************************************************************/
/*  BuildLineTab() :  Décode un fichier sous forme de ligne de code. */
/*********************************************************************/
static struct line **BuildLineTab(unsigned char *data, int length, int *nb_line_rtn)
{
  char *end;
  char *begin;
  int i, nb_line;
  struct line *current_line;
  struct line **tab_line;

  /* Compte le nombre de ligne */
  for(i=0,nb_line=1; i<length; i++)
    if(data[i] == '\n')
      nb_line++;

  /* Allocation mémoire du tableau */
  tab_line = (struct line **) calloc(nb_line,sizeof(struct line *));
  if(tab_line == NULL)
    {
      printf("  Error : Impossible to allocate memory to process the file.\n");
      return(NULL);
    }

  /*** Décodage des lignes ***/
  nb_line = 0;
  begin = data;
  while(begin)
    {
      /* Fin de ligne */
      end = strchr(begin,'\n');
      if(end != NULL)
        *end = '\0';

      /** Décodage de la ligne **/
      current_line = BuildOneLine(begin,nb_line+1);
      if(current_line == NULL)
        {
          for(i=0; i<nb_line; i++)
            mem_free_line(tab_line[i]);
          free(tab_line);
          return(NULL);
        }

      /* Stockage de la ligne */
      tab_line[nb_line++] = current_line;

      /* Ligne suivante */
      begin = (end == NULL) ? NULL : end+1;
    }

  /* Renvoie le tableau */
  *nb_line_rtn = nb_line;
  return(tab_line);
}


/************************************************/
/*  BuildOneLine() :  Décode une ligne de Code. */
/************************************************/
static struct line *BuildOneLine(char *line, int line_nb)
{
  int i, offset, length;
  struct line *current_line;
  char *data_tab[] = {"HEX","DC","DE","DP","DS","DA","DDB","DFB","DB","DW","ADR","ADRL","ASC","DCI","INV","FLS","STR","STRL","PUT","USE","ANOP","ORG","START","END","MX","XC","LONGA","LONGI","USING","LST","REL","DSK","IF","DO","ELSE","FIN","LUP","--^",NULL};  

  /* Allocation mémoire */
  current_line = (struct line *) calloc(1,sizeof(struct line));
  if(current_line == NULL)
    {
      printf("  Error processing line %d (%s) : %s\n",line_nb,line,ERR_MEMORY_ALLOC);
      return(NULL);
    }
  current_line->number = line_nb;
  current_line->length = strlen(line) + 1;

  /* Ligne vide ? */
  if(strlen(line) == 0)
    {
      current_line->type = TYPE_LINE_EMPTY;
      return(current_line);
    }

  /* Ligne commentaire */
  if(line[0] == ';' || line[0] == '*')
    {
      current_line->type = TYPE_LINE_COMMENT;
      current_line->comment = strdup(line);
      if(current_line->comment == NULL)
        {
          printf("  Error processing line %d (%s) : %s\n",line_nb,line,ERR_MEMORY_ALLOC);
          mem_free_line(current_line);
          return(NULL);
        }
      return(current_line);
    }

  /*** Décodage des 4 zones ***/  
  current_line->type = TYPE_LINE_CODE;

  /** Label **/
  offset = 0;
  length = GetLineValue(&line[offset],0,&current_line->label);
  if(length < 0)
    {
      printf("  Error processing line %d (%s) : %s\n",line_nb,line,(length == -1)?ERR_MEMORY_ALLOC:ERR_LINE_FORMAT);
      mem_free_line(current_line);
      return(NULL);
    }

  /** Opcode **/
  offset += length;
  length = GetLineValue(&line[offset],0,&current_line->opcode);
  if(length < 0)
    {
      printf("  Error processing line %d (%s) : %s\n",line_nb,line,(length == -1)?ERR_MEMORY_ALLOC:ERR_LINE_FORMAT);
      mem_free_line(current_line);
      return(NULL);
    }

  /** Operand **/
  offset += length;
  length = GetLineValue(&line[offset],0,&current_line->operand);
  if(length < 0)
    {
      printf("  Error processing line %d (%s) : %s\n",line_nb,line,(length == -1)?ERR_MEMORY_ALLOC:ERR_LINE_FORMAT);
      mem_free_line(current_line);
      return(NULL);
    }

  /** Comment **/
  offset += length;
  length = GetLineValue(&line[offset],1,&current_line->comment);
  if(length < 0)
    {
      printf("  Error processing line %d (%s) : %s\n",line_nb,line,(length == -1)?ERR_MEMORY_ALLOC:ERR_LINE_FORMAT);
      mem_free_line(current_line);
      return(NULL);
    }

  /** Est-ce une ligne DATA ? **/
  if(strlen(current_line->opcode) > 0)
    {
      for(i=0; data_tab[i] != NULL; i++)
        if(!my_stricmp(data_tab[i],current_line->opcode))
          {
            current_line->type = TYPE_LINE_DATA;
            break;
          }
    }

  /* Renvoi la structure */
  return(current_line);
}


/************************************************************/
/*  GetLineValue() :  Récupère une des valeurs de la ligne. */
/************************************************************/
static int GetLineValue(char *line_data, int is_comment, char **value_rtn)
{
  int i, length;
  char *value;
  char *end_sep;

  /** Pas de valeur **/
  if(line_data[0] == '\0')
    {
      /* Valeur */
      value = strdup("");
      if(value == NULL)
        return(-1);
      
      /* Longueur */
      length = 0;
    }
  else if(line_data[0] == ';' && is_comment == 0)
    {
      /* Valeur */
      value = strdup("");
      if(value == NULL)
        return(-1);
      
      /* Longueur */
      length = 0; 
    }
  else if(line_data[0] == ';' && is_comment == 1)
    {
      /* Valeur */
      value = strdup(line_data);
      if(value == NULL)
        return(-1);
      
      /* Longueur */
      length = strlen(line_data);      
    }
  else if(line_data[0] == ' ' || line_data[0] == '\t')
    {
      /** Valeur vide **/
      /* valeur */
      value = strdup("");
      if(value == NULL)
        return(-1);

      /* Longueur */
      length = 0;
      while(line_data[length] == ' ' || line_data[length] == '\t')
        length++;
    }
  else
    {
      /** Valeur **/
      for(i=0; i<(int) strlen(line_data); i++)
        {
          if(line_data[i] == '"')
            {
              end_sep = strchr(&line_data[i+1],'"');
              if(end_sep == NULL)
                return(-2);
              i += ((end_sep - &line_data[i+1]) + 1);
            }
          else if(line_data[i] == '\'')
            {
              end_sep = strchr(&line_data[i+1],'\'');
              if(end_sep == NULL)
                return(-2);
              i += ((end_sep - &line_data[i+1]) + 1);
            }
          else if(line_data[i] == ' ' || line_data[i] == '\t')
            break;
        }
      length = i;

      /* Allocation mémoire */
      value = (char *) calloc(1,length+1);
      if(value == NULL)
        return(-1);
      memcpy(value,line_data,length);

      /* Continue tant qu'il y a des espaces */
      while(line_data[length] == ' ' || line_data[length] == '\t')
        length++;
    }

  /* Renvoi la valeur */
  *value_rtn = value;
  return(length);
}


/****************************************************************/
/*  mem_free_line() :  Libération mémoire de la structure line. */
/****************************************************************/
static void mem_free_line(struct line *current_line)
{
  if(current_line)
    {
      if(current_line->label)
        free(current_line->label);

      if(current_line->opcode)
        free(current_line->opcode);

      if(current_line->operand)
        free(current_line->operand);

      if(current_line->comment)
        free(current_line->comment);

      free(current_line);
    }
}

/***********************************************************************/
