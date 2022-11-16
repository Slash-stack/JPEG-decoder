#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>

#include "matrice.h"
#include "jpeg_reader.h"
#include "bitstream.h"


/*------------------------------------------------------------------------------------------------------------*/
/*                                                                                                            */
/*                                                                                                            */
/*                                               CREATE IMAGE                                                 */
/*                                                                                                            */
/*                                                                                                            */
/*------------------------------------------------------------------------------------------------------------*/

// ----------------------Fonction Annexe----------------------

void print_matrice(struct matrice_8x8 *mat){
    /* Affiche un matrice de nombres dans la console */

    //printf("\nMatrice taille 8x8 n° %u : \n", mat->id);
    printf("\nMatrice taille 8x8 : \n");
    for (uint8_t i=0; i<8;  i++) {
        printf("[ ");
        for (uint8_t j=0; j<8;  j++) {
            printf("%x, ", mat->tab[i * 8 + j]);
        } printf("]\n");
    } printf("\n");
}

void print_matrice_pixel(struct matrice_8x8_pixel *mat){
    /* Affiche un matrice de pixels dans la console */

    //printf("\nMatrice taille 8x8 pixels n° %u : \n", mat->id);
    printf("\nMatrice taille 8x8 pixels : \n");
    for (uint8_t i=0; i<8;  i++) {
        printf("( ");
        for (uint8_t j=0; j<8;  j++) {
            printf("{%x, %x, %x}, ", mat->tab[i * 8 + j].X, mat->tab[i * 8 + j].Y, mat->tab[i * 8 + j].Z);
        } printf(")\n");
    } printf("\n");
}

void print_tab(int16_t *tab){
    /* Affiche un tableau de pixels dans la console */

    printf("\nTableau taille 64 : \n");
    printf("| ");
    for (uint8_t i=0; i<64;  i++) {
        printf("%x, ", tab[i]);
    } printf("|\n");
}

// --------------------Création de fichier--------------------

char *create_name(struct jpeg_desc *jdesc, bool couleur){
    /* on génére le nom du fichier pgm à partir du fichier traité */

    const char *filename = jpeg_get_filename(jdesc);
    uint8_t k = 7, longueur_filename = 4;
    char c = filename[k];
    while (c != '.') {
        k++;
        c = filename[k];
    } longueur_filename += k - 7;

    char *filename_out = malloc((longueur_filename + 1) * sizeof(char)); 
    k = 7, c = filename[k];
    while (c != '.') {
        filename_out[k - 7] = c;
        k++;
        c = filename[k];
    }
    filename_out[k - 7 + 0] = '.';
    filename_out[k - 7 + 1] = 'p';
    if (couleur == true){
        filename_out[k - 7 + 2] = 'p';        
    } else {
        filename_out[k - 7 + 2] = 'g';
    }
    filename_out[k - 7 + 3] = 'm';
    filename_out[k - 7 + 4] = '\0';
    return filename_out;
}

void create_pgm_gris(struct matrice_8x8 *matrice, struct jpeg_desc *jdesc){
    /* Il s'agit d'une fonction qui prend en argument une liste chaînée de MCUs au format matrice_8x8 */
    /* et crée un fichier pgm tout en écrivant les valeurs en niveau de gris dessus.                  */

    char *filename_out = create_name(jdesc, false);

    uint16_t longueur = jpeg_get_image_size(jdesc, 0);
    uint16_t hauteur = jpeg_get_image_size(jdesc, 1);

    // nombre de pixel à remplir pour la dernière colonne / dernière ligne de MCUs
    uint16_t ecart_horizontale = longueur % 8;
    uint16_t ecart_verticale = hauteur % 8;
    if (ecart_horizontale == 0){
        ecart_horizontale = 8;
    }
    if (ecart_verticale == 0){
        ecart_verticale = 8;
    }

    uint32_t nbre_MCU_horizontale = ceil((float)longueur / 8);
    uint32_t nbre_MCU_verticale = ceil((float)hauteur / 8);

    FILE* fichier = fopen(filename_out, "wb");
    if (fichier != NULL)
    {
        // écriture de l'entête du fichier pgm
        fprintf(fichier, "P5 \n%u %u\n255\n", longueur, hauteur);

        struct matrice_8x8 *matrice_courante;
        matrice_courante = matrice;
        struct matrice_8x8 *tete_ligne;
        tete_ligne = matrice;

        // parcours de la ième ligne de MCU
        for (uint32_t i = 0; i < nbre_MCU_verticale; i++) {
            tete_ligne = matrice_courante;

            // si on ne se trouve pas à la dernière ligne (là où il y'a les blocs manquants)
            if (i != nbre_MCU_verticale - 1) {

                // parcours de la ième ligne de la matrice courante
                for (uint8_t ligne = 0; ligne < 8; ligne++) {

                    // retour au début de la ligne
                    matrice_courante = tete_ligne;

                    // parcours de la matrice d'indice i et de colonne j
                    for (uint16_t j = 0; j < nbre_MCU_horizontale; j++) {
                        if (matrice_courante != NULL){

                            // si on ne se trouve pas à la dernière colonne (là où il y'a les blocs manquants)
                            if (j != nbre_MCU_horizontale - 1){

                                // écriture des élèments de la ligne d'indice *ligne*
                                for (uint8_t colonne = 0; colonne < 8; colonne++) {
                                    fwrite(&matrice_courante->tab[ligne * 8 + colonne], 1, 1, fichier);
                                }

                            // cas où on se trouve dans la derniére matrice de la ligne
                            } else {

                                // on écrit seulement les élèments dans les dimensions de l'image
                                for (uint8_t colonne = 0; colonne < ecart_horizontale; colonne++) {
                                    fwrite(&matrice_courante->tab[ligne * 8 + colonne], 1, 1, fichier);
                                }
                            }

                            // passage à la matrice voisine / passage à la ligne suivante
                                matrice_courante = matrice_courante->suivant;
                        } else {
                            break;
                        }
                    }
                }
            // cas où on se trouve à la dernière ligne
            } else {

                // parcours des lignes des matrices dans les dimensions de l'image
                for (uint8_t ligne = 0; ligne < ecart_verticale; ligne++) {

                    // retour au début de la ligne
                    matrice_courante = tete_ligne;

                    // parcours de la matrice d'indice i et de colonne j
                    for (uint16_t j = 0; j < nbre_MCU_horizontale; j++) {
                        if (matrice_courante != NULL){

                            // si on ne se trouve pas à la dernière colonne (là où il y'a les blocs manquants)
                            if (j != nbre_MCU_horizontale - 1){

                                // écriture des élèments de la ligne d'indice *ligne*
                                for (uint8_t colonne = 0; colonne < 8; colonne++) {
                                    fwrite(&matrice_courante->tab[ligne * 8 + colonne], 1, 1, fichier);
                                }

                            // cas où on se trouve dans la derniére matrice de l'image
                            } else {

                                // on écrit seulement les élèments dans les dimensions de l'image
                                for (uint8_t colonne = 0; colonne < ecart_horizontale; colonne++) {
                                    fwrite(&matrice_courante->tab[ligne * 8 + colonne], 1, 1, fichier);
                                }
                            }
                            // passage à la matrice voisine / retour à la ligne
                            matrice_courante = matrice_courante->suivant;
                        } else {
                            break;
                        }
                    }
                }
            }
        }
        fprintf(fichier, "\n");
        fclose(fichier);
    }
    free(filename_out);
}

void create_ppm_couleur(struct matrice_8x8_pixel **matrice, struct jpeg_desc *jdesc){
    /* Génère l'image en couleur au format .ppm à partir d'une matrice de MCUs */

    // on génére le nom du fichier ppm à partir du fichier traité
    char *filename_out = create_name(jdesc, true);

    uint16_t longueur = jpeg_get_image_size(jdesc, 0);
    uint16_t hauteur = jpeg_get_image_size(jdesc, 1);

    uint16_t ecart_horizontale = longueur % 8;
    uint16_t ecart_verticale = hauteur % 8;
    if (ecart_horizontale == 0){
        ecart_horizontale = 8;
    }
    if (ecart_verticale == 0){
        ecart_verticale = 8;
    }

    uint32_t nbre_MCU_horizontale = ceil((float)longueur / 8);
    uint32_t nbre_MCU_verticale = ceil((float)hauteur / 8);

    FILE* fichier = fopen(filename_out, "wb");
    if (fichier != NULL)
    {
        // écriture de l'entête du fichier pgm
        fprintf(fichier, "P6 \n%u %u\n255\n", longueur, hauteur);


        // parcours de la ième ligne de MCU
        for (uint32_t i = 0; i < nbre_MCU_verticale; i++) {

            // si on ne se trouve pas à la dernière ligne (là où il y'a les blocs manquants)
            if (i != nbre_MCU_verticale - 1) {

                // parcours de la ième ligne de la matrice courante
                for (uint8_t ligne = 0; ligne < 8; ligne++) {

                    // parcours de la matrice d'indice i et de colonne j
                    for (uint16_t j = 0; j < nbre_MCU_horizontale; j++) {

                        // si on ne se trouve pas à la dernière colonne (là où il y'a les blocs manquants)
                        if (j != nbre_MCU_horizontale - 1){

                            // écriture des pixels de la ligne d'indice *ligne*
                            // X -> R, Y -> G, Z -> B
                            for (uint8_t colonne = 0; colonne < 8; colonne++) {
                                fwrite(&matrice[i * nbre_MCU_horizontale + j]->tab[ligne * 8 + colonne].X, 1, 1, fichier);
                                fwrite(&matrice[i * nbre_MCU_horizontale + j]->tab[ligne * 8 + colonne].Y, 1, 1, fichier);
                                fwrite(&matrice[i * nbre_MCU_horizontale + j]->tab[ligne * 8 + colonne].Z, 1, 1, fichier);
                            }

                        // cas où on se trouve dans la derniére matrice de la ligne
                        } else {

                            // on écrit seulement les élèments dans les dimensions de l'image
                            // X -> R, Y -> G, Z -> B
                            for (uint8_t colonne = 0; colonne < ecart_horizontale; colonne++) {
                                /*fprintf(stderr, "%d // ", matrice[i * nbre_MCU_horizontale + j]->tab[ligne * 8 + colonne].Z);*/
                                fwrite(&matrice[i * nbre_MCU_horizontale + j]->tab[ligne * 8 + colonne].X, 1, 1, fichier);
                                fwrite(&matrice[i * nbre_MCU_horizontale + j]->tab[ligne * 8 + colonne].Y, 1, 1, fichier);
                                fwrite(&matrice[i * nbre_MCU_horizontale + j]->tab[ligne * 8 + colonne].Z, 1, 1, fichier);
                            }
                        }
                    }
                }
            // cas où on se trouve à la dernière ligne
            } else {

                // parcours des lignes des matrices dans les dimensions de l'image
                for (uint8_t ligne = 0; ligne < ecart_verticale; ligne++) {

                    // parcours de la matrice d'indice i et de colonne j
                    for (uint16_t j = 0; j < nbre_MCU_horizontale; j++) {

                        // si on ne se trouve pas à la dernière colonne (là où il y'a les blocs manquants)
                        if (j != nbre_MCU_horizontale - 1){

                            // écriture des élèments de la ligne d'indice *ligne*
                            // X -> R, Y -> G, Z -> B
                            for (uint8_t colonne = 0; colonne < 8; colonne++) {
                                fwrite(&matrice[i * nbre_MCU_horizontale + j]->tab[ligne * 8 + colonne].X, 1, 1, fichier);
                                fwrite(&matrice[i * nbre_MCU_horizontale + j]->tab[ligne * 8 + colonne].Y, 1, 1, fichier);
                                fwrite(&matrice[i * nbre_MCU_horizontale + j]->tab[ligne * 8 + colonne].Z, 1, 1, fichier);
                            }

                        // cas où on se trouve dans la derniére matrice de l'image
                        } else {
                            // on écrit seulement les élèments dans les dimensions de l'image
                            // X -> R, Y -> G, Z -> B
                            for (uint8_t colonne = 0; colonne < ecart_horizontale; colonne++) {
                                fwrite(&matrice[i * nbre_MCU_horizontale + j]->tab[ligne * 8 + colonne].X, 1, 1, fichier);
                                fwrite(&matrice[i * nbre_MCU_horizontale + j]->tab[ligne * 8 + colonne].Y, 1, 1, fichier);
                                fwrite(&matrice[i * nbre_MCU_horizontale + j]->tab[ligne * 8 + colonne].Z, 1, 1, fichier);
                            }
                        }
                    }
                }
            }
        }
        fprintf(fichier, "\n");
        fclose(fichier);
    }
    free(filename_out);

}