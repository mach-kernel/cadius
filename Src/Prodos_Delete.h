/***********************************************************************/
/*                                                                     */
/*   Prodos_Delete.h : Header pour la gestion des commandes DELETE.    */
/*                                                                     */
/***********************************************************************/
/*  Auteur : Olivier ZARDINI  *  Brutal Deluxe Software  *  Mar 2012   */
/***********************************************************************/

#include <stdbool.h>

void DeleteProdosFile(struct prodos_image *,char *,bool);
void DeleteProdosFolder(struct prodos_image *,char *);
void DeleteProdosVolume(struct prodos_image *);

/***********************************************************************/