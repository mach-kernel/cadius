/***********************************************************************/
/*                                                                     */
/*   Prodos_Extract.h : Header pour la gestion des commandes EXTRACT.  */
/*                                                                     */
/***********************************************************************/
/*  Auteur : Olivier ZARDINI  *  Brutal Deluxe Software  *  Dec 2011   */
/***********************************************************************/

#include <stdbool.h>

int ExtractOneFile(struct prodos_image *, char *, char *, bool);
void ExtractFolderFiles(struct prodos_image *, struct file_descriptive_entry *, char *, bool);
void ExtractVolumeFiles(struct prodos_image *, char *, bool);

/***********************************************************************/
