/**********************************************************************/
/*                                                                    */
/*  Prodos_Create.h : Header pour la gestion des commandes CREATE.    */
/*                                                                    */
/**********************************************************************/
/*  Auteur : Olivier ZARDINI  *  Brutal Deluxe Software  *  Mar 2012  */
/**********************************************************************/

void CreateProdosFolder(struct prodos_image *,char *,bool);
struct prodos_image *CreateProdosVolume(char *,char *,int,int,bool);
struct file_descriptive_entry *CreateOneProdosFolder(struct prodos_image *,struct file_descriptive_entry *,char *,bool,int);
struct file_descriptive_entry *BuildProdosFolderPath(struct prodos_image *,char *,int *,bool,int);

/***********************************************************************/