/***********************************************************************/
/*                                                                     */
/*   Prodos_Extract.h : Header pour la gestion des commandes EXTRACT.  */
/*                                                                     */
/***********************************************************************/
/*  Auteur : Olivier ZARDINI  *  Brutal Deluxe Software  *  Dec 2011   */
/***********************************************************************/

void ExtractOneFile(struct prodos_image *,char *,char *);
void ExtractFolderFiles(struct prodos_image *,struct file_descriptive_entry *,char *);
void ExtractVolumeFiles(struct prodos_image *,char *);

/***********************************************************************/
