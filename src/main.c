#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>

#include "matrice.h"
#include "jpeg_reader.h"
#include "bitstream.h"
#include "huffman.h"
#include "quantification_zig-zag.h"
#include "iDCT_v2.h"
#include "iDCT_v3.h"
#include "RLE_huffman.h"
#include "createIMAGE.h"

/*------------------------------------------------------------------------------------------------------------*/
/*                                                                                                            */
/*                                                                                                            */
/*                                                   main                                                     */
/*                                                                                                            */
/*                                                                                                            */
/*------------------------------------------------------------------------------------------------------------*/

// -----------------------------------------------------------

int main(int argc, char **argv){
    /* Permet de générer une image pgm / ppm à partir d'une image jpeg */

    if (argc != 2) {
	// S'il n'y'a pas au moins un argument en ligne de commandes, on renvoie un usage.
	fprintf(stderr, "Usage: %s fichier.jpeg\n", argv[0]);
	return EXIT_FAILURE;
    }
    
    // On recupere le nom du fichier JPEG sur la ligne de commande.
    const char *filename = argv[1];

    // On crée un jpeg_desc qui permettra de lire ce fichier.
    struct jpeg_desc *jdesc = jpeg_read(filename);

    // On recupere le flux des donnees brutes a partir du descripteur.
    struct bitstream *stream = jpeg_get_bitstream(jdesc);

    uint16_t longueurr = jpeg_get_image_size(jdesc, 0);
    uint16_t hauteurr = jpeg_get_image_size(jdesc, 1);
    

    // image en noir et blanc
    if (jpeg_get_nb_components(jdesc) == 1) {

        int16_t *valeur_DC_prec = malloc (sizeof(int16_t));
        *valeur_DC_prec = 0;

        int16_t *tab = malloc(64 * sizeof(int16_t));
        
        //Variable Lenght Decoding
        RLE_huffman(jdesc, stream, tab, valeur_DC_prec, 0);
        
        // IQZZ
        quantification_inverse_MCU_1x64(jdesc, tab, 0);
        struct matrice_8x8 *matrice_frequences = initialisation();
        zig_zag_inverse(tab, matrice_frequences);

        // IDCT
        struct matrice_8x8 *matrice_spatial = iDCT_bloc_v3(matrice_frequences);

        // Création de la liste chaînée des MCUs
        struct matrice_8x8 tete = *matrice_spatial;
        struct matrice_8x8 *matrice_courante = &tete;
        matrice_courante->id = 0;
        free(tab);

        for (uint32_t k = 1; k < hauteurr * longueurr / 64 ; k++) {

            int16_t *tab = malloc(64 * sizeof(int16_t));
            
            //Variable Lenght Decoding
            RLE_huffman(jdesc, stream, tab, valeur_DC_prec, 0);

            // IQZZ
            quantification_inverse_MCU_1x64(jdesc, tab, 0);
            zig_zag_inverse(tab, matrice_frequences);

            // IDCT
            struct matrice_8x8 *mat_spatial = iDCT_bloc_v3(matrice_frequences);
            
            // Ajout dans la liste chaînée des MCUs
            matrice_courante->suivant = mat_spatial;
            matrice_courante = matrice_courante->suivant;
            matrice_courante->id = k;
            free(tab);
        }

        free(valeur_DC_prec);
        free_matrice(matrice_frequences);

        // Ecriture dans un fichier pgm
        create_pgm_gris(&tete, jdesc);

        free_liste_matrice(&tete);
        free(matrice_spatial);
        jpeg_close(jdesc);
        return EXIT_SUCCESS;
    }
    
    // image en couleur
    else {
        if (jpeg_get_frame_component_sampling_factor(jdesc, 0, 0) == 1) {
            if (jpeg_get_frame_component_sampling_factor(jdesc, 1, 0) == 1) {
                // pas de sous-échantillonnage
                uint32_t nbre_MCU_horizontale = ceil((float)longueurr / 8);
                uint32_t nbre_MCU_verticale = ceil((float)hauteurr / 8);
                struct matrice_8x8_pixel **matrice_image = malloc((nbre_MCU_verticale * nbre_MCU_horizontale) * sizeof(struct matrice_8x8_pixel *));

                int16_t *valeur_DC_prec_Y = malloc (sizeof(int16_t));
                *valeur_DC_prec_Y = 0;
                int16_t *valeur_DC_prec_Cb = malloc (sizeof(int16_t));
                *valeur_DC_prec_Cb = 0;
                int16_t *valeur_DC_prec_Cr = malloc (sizeof(int16_t));
                *valeur_DC_prec_Cr = 0;

                int16_t *tab_Y = malloc(64 * sizeof(int16_t));
                int16_t *tab_Cb = malloc(64 * sizeof(int16_t));
                int16_t *tab_Cr = malloc(64 * sizeof(int16_t));

                //Variable Lenght Decoding
                RLE_huffman(jdesc, stream, tab_Y, valeur_DC_prec_Y, 0);
                RLE_huffman(jdesc, stream, tab_Cb, valeur_DC_prec_Cb, 1);
                RLE_huffman(jdesc, stream, tab_Cr, valeur_DC_prec_Cr, 1);

                // IQZZ
                quantification_inverse_MCU_1x64(jdesc, tab_Y, 0);
                quantification_inverse_MCU_1x64(jdesc, tab_Cb, 1);
                quantification_inverse_MCU_1x64(jdesc, tab_Cr, 1);

                struct matrice_8x8 *matrice_frequences_Y = initialisation();
                struct matrice_8x8 *matrice_frequences_Cb = initialisation();
                struct matrice_8x8 *matrice_frequences_Cr = initialisation();

                zig_zag_inverse(tab_Y, matrice_frequences_Y);
                zig_zag_inverse(tab_Cb, matrice_frequences_Cb);
                zig_zag_inverse(tab_Cr, matrice_frequences_Cr);

                // IDCT
                struct matrice_8x8 *matrice_spatial_Y = iDCT_bloc_v3(matrice_frequences_Y);
                struct matrice_8x8 *matrice_spatial_Cb = iDCT_bloc_v3(matrice_frequences_Cb);
                struct matrice_8x8 *matrice_spatial_Cr = iDCT_bloc_v3(matrice_frequences_Cr);

                // Création de la liste chaînée des MCUs
                matrice_image[0] = initialisation_pixel(matrice_spatial_Y, matrice_spatial_Cb, matrice_spatial_Cr);
                
                free_matrice(matrice_spatial_Y);
                free_matrice(matrice_spatial_Cb);
                free_matrice(matrice_spatial_Cr);
                free(tab_Y);
                free(tab_Cb);
                free(tab_Cr);

                for (uint32_t k = 1; k < (uint32_t)(nbre_MCU_verticale * nbre_MCU_horizontale) ; k++) {
                    
                    int16_t *tab_Y = malloc(64 * sizeof(int16_t));
                    int16_t *tab_Cb = malloc(64 * sizeof(int16_t));
                    int16_t *tab_Cr = malloc(64 * sizeof(int16_t));

                    //Variable Lenght Decoding
                    RLE_huffman(jdesc, stream, tab_Y, valeur_DC_prec_Y, 0);
                    RLE_huffman(jdesc, stream, tab_Cb, valeur_DC_prec_Cb, 1);
                    RLE_huffman(jdesc, stream, tab_Cr, valeur_DC_prec_Cr, 1);

                    // IQZZ
                    quantification_inverse_MCU_1x64(jdesc, tab_Y, 0);
                    quantification_inverse_MCU_1x64(jdesc, tab_Cb, 1);
                    quantification_inverse_MCU_1x64(jdesc, tab_Cr, 1);

                    zig_zag_inverse(tab_Y, matrice_frequences_Y);
                    zig_zag_inverse(tab_Cb, matrice_frequences_Cb);
                    zig_zag_inverse(tab_Cr, matrice_frequences_Cr);

                    // IDCT
                    struct matrice_8x8 *mat_spatial_Y = iDCT_bloc_v3(matrice_frequences_Y);
                    struct matrice_8x8 *mat_spatial_Cb = iDCT_bloc_v3(matrice_frequences_Cb);
                    struct matrice_8x8 *mat_spatial_Cr = iDCT_bloc_v3(matrice_frequences_Cr);

                    // Ajout dans la liste chaînée des MCUs
                    matrice_image[k] = initialisation_pixel(mat_spatial_Y, mat_spatial_Cb, mat_spatial_Cr);
                    free_matrice(mat_spatial_Y);
                    free_matrice(mat_spatial_Cb);
                    free_matrice(mat_spatial_Cr);
                    free(tab_Y);
                    free(tab_Cb);
                    free(tab_Cr);
                }

                free(valeur_DC_prec_Y);
                free(valeur_DC_prec_Cb);
                free(valeur_DC_prec_Cr);
                free_matrice(matrice_frequences_Y);
                free_matrice(matrice_frequences_Cb);
                free_matrice(matrice_frequences_Cr);

                // Ecriture dans un fichier ppm
                create_ppm_couleur(matrice_image, jdesc);

                for (uint16_t i = 0; i < (uint32_t)(nbre_MCU_verticale * nbre_MCU_horizontale); i++){
                    free_matrice_pixel(matrice_image[i]);
                } free(matrice_image);
                jpeg_close(jdesc);
            
            } else {
                // sous-échantillage vertical

                uint32_t nbre_MCU_horizontale = ceil((float)longueurr / 8);
                uint32_t nbre_MCU_verticale = ceil((float)hauteurr / 8);

                struct matrice_8x8_pixel **matrice_image = malloc((nbre_MCU_verticale * nbre_MCU_horizontale) * sizeof(struct matrice_8x8_pixel *));

                int16_t *valeur_DC_prec_Y = malloc (sizeof(int16_t));
                *valeur_DC_prec_Y = 0;
                int16_t *valeur_DC_prec_Cb = malloc (sizeof(int16_t));
                *valeur_DC_prec_Cb = 0;
                int16_t *valeur_DC_prec_Cr = malloc (sizeof(int16_t));
                *valeur_DC_prec_Cr = 0;

                int16_t *tab_Y0 = malloc(64 * sizeof(int16_t));
                int16_t *tab_Y1 = malloc(64 * sizeof(int16_t));
                int16_t *tab_Cb = malloc(64 * sizeof(int16_t));
                int16_t *tab_Cr = malloc(64 * sizeof(int16_t));

                //Variable Lenght Decoding
                RLE_huffman(jdesc, stream, tab_Y0, valeur_DC_prec_Y, 0);
                RLE_huffman(jdesc, stream, tab_Y1, valeur_DC_prec_Y, 0);
                RLE_huffman(jdesc, stream, tab_Cb, valeur_DC_prec_Cb, 1);
                RLE_huffman(jdesc, stream, tab_Cr, valeur_DC_prec_Cr, 1);

                // IQZZ
                quantification_inverse_MCU_1x64(jdesc, tab_Y0, 0);
                quantification_inverse_MCU_1x64(jdesc, tab_Y1, 0);
                quantification_inverse_MCU_1x64(jdesc, tab_Cb, 1);
                quantification_inverse_MCU_1x64(jdesc, tab_Cr, 1);

                struct matrice_8x8 *matrice_frequences_Y0 = initialisation();
                struct matrice_8x8 *matrice_frequences_Y1 = initialisation();
                struct matrice_8x8 *matrice_frequences_Cb = initialisation();
                struct matrice_8x8 *matrice_frequences_Cr = initialisation();

                zig_zag_inverse(tab_Y0, matrice_frequences_Y0);
                zig_zag_inverse(tab_Y0, matrice_frequences_Y1);
                zig_zag_inverse(tab_Cb, matrice_frequences_Cb);
                zig_zag_inverse(tab_Cr, matrice_frequences_Cr);

                // IDCT
                struct matrice_8x8 *matrice_spatial_Y0 = iDCT_bloc_v3(matrice_frequences_Y0);
                struct matrice_8x8 *matrice_spatial_Y1 = iDCT_bloc_v3(matrice_frequences_Y1);
                struct matrice_8x8 *matrice_spatial_Cb = iDCT_bloc_v3(matrice_frequences_Cb);
                struct matrice_8x8 *matrice_spatial_Cr = iDCT_bloc_v3(matrice_frequences_Cr);

                // YCbCr to RGB
                matrice_image[0] = initialisation_pixel(matrice_spatial_Y0, matrice_spatial_Cb, matrice_spatial_Cr);
                matrice_image[nbre_MCU_horizontale] = initialisation_pixel(matrice_spatial_Y1, matrice_spatial_Cb, matrice_spatial_Cr);
                free_matrice(matrice_spatial_Y0);
                free_matrice(matrice_spatial_Y1);
                free_matrice(matrice_spatial_Cb);
                free_matrice(matrice_spatial_Cr);


                for (uint32_t ligne = 0; ligne < ceil(((float)nbre_MCU_verticale / 2)); ligne++) {
                    for (uint32_t colonne = 0; colonne < nbre_MCU_horizontale; colonne++){
                        if (ligne == 0 && colonne == 0){
                            continue;
                        }

                        //Variable Lenght Decoding
                        RLE_huffman(jdesc, stream, tab_Y0, valeur_DC_prec_Y, 0);
                        RLE_huffman(jdesc, stream, tab_Y1, valeur_DC_prec_Y, 0);
                        RLE_huffman(jdesc, stream, tab_Cb, valeur_DC_prec_Cb, 1);
                        RLE_huffman(jdesc, stream, tab_Cr, valeur_DC_prec_Cr, 1);

                        // IQZZ
                        quantification_inverse_MCU_1x64(jdesc, tab_Y0, 0);
                        quantification_inverse_MCU_1x64(jdesc, tab_Y1, 0);
                        quantification_inverse_MCU_1x64(jdesc, tab_Cb, 1);
                        quantification_inverse_MCU_1x64(jdesc, tab_Cr, 1);

                        zig_zag_inverse(tab_Y0, matrice_frequences_Y0);
                        zig_zag_inverse(tab_Y1, matrice_frequences_Y1);
                        zig_zag_inverse(tab_Cb, matrice_frequences_Cb);
                        zig_zag_inverse(tab_Cr, matrice_frequences_Cr);

                        // IDCT
                        struct matrice_8x8 *mat_spatial_Y0 = iDCT_bloc_v3(matrice_frequences_Y0);
                        struct matrice_8x8 *mat_spatial_Y1 = iDCT_bloc_v3(matrice_frequences_Y1);
                        struct matrice_8x8 *mat_spatial_Cb = iDCT_bloc_v3(matrice_frequences_Cb);
                        struct matrice_8x8 *mat_spatial_Cr = iDCT_bloc_v3(matrice_frequences_Cr);

                        // YCbCr to RGB
                        matrice_image[2 * ligne * nbre_MCU_horizontale + colonne] = initialisation_pixel(mat_spatial_Y0, mat_spatial_Cb, mat_spatial_Cr);
                        if ((2 * ligne + 1) * nbre_MCU_horizontale + colonne < (uint32_t)(nbre_MCU_verticale * nbre_MCU_horizontale)){
                            matrice_image[(2 * ligne + 1) * nbre_MCU_horizontale + colonne] = initialisation_pixel(mat_spatial_Y1, mat_spatial_Cb, mat_spatial_Cr);
                        }
                        free_matrice(mat_spatial_Y0);
                        free_matrice(mat_spatial_Y1);
                        free_matrice(mat_spatial_Cb);
                        free_matrice(mat_spatial_Cr);
                    }
                }

                free(tab_Y0);
                free(tab_Y1);
                free(tab_Cb);
                free(tab_Cr);
                free(valeur_DC_prec_Y);
                free(valeur_DC_prec_Cb);
                free(valeur_DC_prec_Cr);
                free_matrice(matrice_frequences_Y0);
                free_matrice(matrice_frequences_Y1);
                free_matrice(matrice_frequences_Cb);
                free_matrice(matrice_frequences_Cr);

                // Ecriture dans un fichier ppm
                create_ppm_couleur(matrice_image, jdesc); 

                for (uint16_t i = 0; i < (uint32_t)(nbre_MCU_verticale * nbre_MCU_horizontale); i++){
                    free_matrice_pixel(matrice_image[i]);
                } free(matrice_image);
                jpeg_close(jdesc);
            }
        } else {
            if (jpeg_get_frame_component_sampling_factor(jdesc, 1, 0) == 1) {
                // sous-échantillonnage horizontal

                uint32_t nbre_MCU_horizontale = ceil((float)longueurr / 8);
                uint32_t nbre_MCU_verticale = ceil((float)hauteurr / 8);

                struct matrice_8x8_pixel **matrice_image = malloc((nbre_MCU_verticale * nbre_MCU_horizontale) * sizeof(struct matrice_8x8_pixel *));

                int16_t *valeur_DC_prec_Y = malloc (sizeof(int16_t));
                *valeur_DC_prec_Y = 0;
                int16_t *valeur_DC_prec_Cb = malloc (sizeof(int16_t));
                *valeur_DC_prec_Cb = 0;
                int16_t *valeur_DC_prec_Cr = malloc (sizeof(int16_t));
                *valeur_DC_prec_Cr = 0;

                int16_t *tab_Y0 = malloc(64 * sizeof(int16_t));
                int16_t *tab_Y1 = malloc(64 * sizeof(int16_t));
                int16_t *tab_Cb = malloc(64 * sizeof(int16_t));
                int16_t *tab_Cr = malloc(64 * sizeof(int16_t));

                //Variable Lenght Decoding
                RLE_huffman(jdesc, stream, tab_Y0, valeur_DC_prec_Y, 0);
                RLE_huffman(jdesc, stream, tab_Y1, valeur_DC_prec_Y, 0);
                RLE_huffman(jdesc, stream, tab_Cb, valeur_DC_prec_Cb, 1);
                RLE_huffman(jdesc, stream, tab_Cr, valeur_DC_prec_Cr, 1);

                // IQZZ
                quantification_inverse_MCU_1x64(jdesc, tab_Y0, 0);
                quantification_inverse_MCU_1x64(jdesc, tab_Y1, 0);
                quantification_inverse_MCU_1x64(jdesc, tab_Cb, 1);
                quantification_inverse_MCU_1x64(jdesc, tab_Cr, 1);

                struct matrice_8x8 *matrice_frequences_Y0 = initialisation();
                struct matrice_8x8 *matrice_frequences_Y1 = initialisation();
                struct matrice_8x8 *matrice_frequences_Cb = initialisation();
                struct matrice_8x8 *matrice_frequences_Cr = initialisation();

                zig_zag_inverse(tab_Y0, matrice_frequences_Y0);
                zig_zag_inverse(tab_Y1, matrice_frequences_Y1);
                zig_zag_inverse(tab_Cb, matrice_frequences_Cb);
                zig_zag_inverse(tab_Cr, matrice_frequences_Cr);

                // IDCT
                struct matrice_8x8 *matrice_spatial_Y0 = iDCT_bloc_v3(matrice_frequences_Y0);
                struct matrice_8x8 *matrice_spatial_Y1 = iDCT_bloc_v3(matrice_frequences_Y1);
                struct matrice_8x8 *matrice_spatial_Cb = iDCT_bloc_v3(matrice_frequences_Cb);
                struct matrice_8x8 *matrice_spatial_Cr = iDCT_bloc_v3(matrice_frequences_Cr);

                // YCbCr to RGB
                matrice_image[0] = initialisation_pixel(matrice_spatial_Y0, matrice_spatial_Cb, matrice_spatial_Cr);
                matrice_image[1] = initialisation_pixel(matrice_spatial_Y1, matrice_spatial_Cb, matrice_spatial_Cr);
                
                free_matrice(matrice_spatial_Y0);
                free_matrice(matrice_spatial_Y1);
                free_matrice(matrice_spatial_Cb);
                free_matrice(matrice_spatial_Cr);

                for (uint32_t ligne = 0; ligne < nbre_MCU_verticale; ligne++) {
                    for (uint32_t colonne = 0; colonne < ceil(((float)nbre_MCU_horizontale / 2)); colonne++){
                        if (ligne == 0 && colonne == 0){
                            continue;
                        }

                        //Variable Lenght Decoding
                        RLE_huffman(jdesc, stream, tab_Y0, valeur_DC_prec_Y, 0);
                        RLE_huffman(jdesc, stream, tab_Y1, valeur_DC_prec_Y, 0);
                        RLE_huffman(jdesc, stream, tab_Cb, valeur_DC_prec_Cb, 1);
                        RLE_huffman(jdesc, stream, tab_Cr, valeur_DC_prec_Cr, 1);

                        // IQZZ
                        quantification_inverse_MCU_1x64(jdesc, tab_Y0, 0);
                        quantification_inverse_MCU_1x64(jdesc, tab_Y1, 0);
                        quantification_inverse_MCU_1x64(jdesc, tab_Cb, 1);
                        quantification_inverse_MCU_1x64(jdesc, tab_Cr, 1);

                        zig_zag_inverse(tab_Y0, matrice_frequences_Y0);
                        zig_zag_inverse(tab_Y1, matrice_frequences_Y1);
                        zig_zag_inverse(tab_Cb, matrice_frequences_Cb);
                        zig_zag_inverse(tab_Cr, matrice_frequences_Cr);

                        // IDCT
                        struct matrice_8x8 *mat_spatial_Y0 = iDCT_bloc_v3(matrice_frequences_Y0);
                        struct matrice_8x8 *mat_spatial_Y1 = iDCT_bloc_v3(matrice_frequences_Y1);
                        struct matrice_8x8 *mat_spatial_Cb = iDCT_bloc_v3(matrice_frequences_Cb);
                        struct matrice_8x8 *mat_spatial_Cr = iDCT_bloc_v3(matrice_frequences_Cr);

                        // YCbCr to RGB
                        matrice_image[ligne * nbre_MCU_horizontale + 2 * colonne] = initialisation_pixel(mat_spatial_Y0, mat_spatial_Cb, mat_spatial_Cr);
                        if (2 * colonne + 1 < nbre_MCU_horizontale){
                            matrice_image[ligne * nbre_MCU_horizontale + 2 * colonne + 1] = initialisation_pixel(mat_spatial_Y1, mat_spatial_Cb, mat_spatial_Cr);
                        }
                        free_matrice(mat_spatial_Y0);
                        free_matrice(mat_spatial_Y1);
                        free_matrice(mat_spatial_Cb);
                        free_matrice(mat_spatial_Cr);
                    }
                }
                free(tab_Y0);
                free(tab_Y1);
                free(tab_Cb);
                free(tab_Cr);
                free(valeur_DC_prec_Y);
                free(valeur_DC_prec_Cb);
                free(valeur_DC_prec_Cr);
                free_matrice(matrice_frequences_Y0);
                free_matrice(matrice_frequences_Y1);
                free_matrice(matrice_frequences_Cb);
                free_matrice(matrice_frequences_Cr);

                // Ecriture dans un fichier ppm
                create_ppm_couleur(matrice_image, jdesc);

                for (uint16_t i = 0; i < (uint32_t)(nbre_MCU_verticale * nbre_MCU_horizontale); i++){
                    free_matrice_pixel(matrice_image[i]);
                } free(matrice_image);
                jpeg_close(jdesc);

            } else {
                // sous-échantillonnage horizontal & vertical
                //(on suppose qu'on échantillonne pareil pour les deux)
                uint32_t nbre_MCU_horizontale = ceil((float)longueurr / 8);
                uint32_t nbre_MCU_verticale = ceil((float)hauteurr / 8);

                struct matrice_8x8_pixel **matrice_image = malloc((nbre_MCU_verticale * nbre_MCU_horizontale) * sizeof(struct matrice_8x8_pixel *));

                int16_t *valeur_DC_prec_Y = malloc (sizeof(int16_t));
                *valeur_DC_prec_Y = 0;
                int16_t *valeur_DC_prec_Cb = malloc (sizeof(int16_t));
                *valeur_DC_prec_Cb = 0;
                int16_t *valeur_DC_prec_Cr = malloc (sizeof(int16_t));
                *valeur_DC_prec_Cr = 0;

                int16_t *tab_Y0 = malloc(64 * sizeof(int16_t));
                int16_t *tab_Y1 = malloc(64 * sizeof(int16_t));
                int16_t *tab_Y2 = malloc(64 * sizeof(int16_t));
                int16_t *tab_Y3 = malloc(64 * sizeof(int16_t));
                int16_t *tab_Cb = malloc(64 * sizeof(int16_t));
                int16_t *tab_Cr = malloc(64 * sizeof(int16_t));

                //Variable Lenght Decoding
                RLE_huffman(jdesc, stream, tab_Y0, valeur_DC_prec_Y, 0);
                RLE_huffman(jdesc, stream, tab_Y1, valeur_DC_prec_Y, 0);
                RLE_huffman(jdesc, stream, tab_Y2, valeur_DC_prec_Y, 0);
                RLE_huffman(jdesc, stream, tab_Y3, valeur_DC_prec_Y, 0);
                RLE_huffman(jdesc, stream, tab_Cb, valeur_DC_prec_Cb, 1);
                RLE_huffman(jdesc, stream, tab_Cr, valeur_DC_prec_Cr, 1);

                // IQZZ
                quantification_inverse_MCU_1x64(jdesc, tab_Y0, 0);
                quantification_inverse_MCU_1x64(jdesc, tab_Y1, 0);
                quantification_inverse_MCU_1x64(jdesc, tab_Y2, 0);
                quantification_inverse_MCU_1x64(jdesc, tab_Y3, 0);
                quantification_inverse_MCU_1x64(jdesc, tab_Cb, 1);
                quantification_inverse_MCU_1x64(jdesc, tab_Cr, 1);

                struct matrice_8x8 *matrice_frequences_Y0 = initialisation();
                struct matrice_8x8 *matrice_frequences_Y1 = initialisation();
                struct matrice_8x8 *matrice_frequences_Y2 = initialisation();
                struct matrice_8x8 *matrice_frequences_Y3 = initialisation();
                struct matrice_8x8 *matrice_frequences_Cb = initialisation();
                struct matrice_8x8 *matrice_frequences_Cr = initialisation();

                zig_zag_inverse(tab_Y0, matrice_frequences_Y0);
                zig_zag_inverse(tab_Y1, matrice_frequences_Y1);
                zig_zag_inverse(tab_Y2, matrice_frequences_Y2);
                zig_zag_inverse(tab_Y3, matrice_frequences_Y3);
                zig_zag_inverse(tab_Cb, matrice_frequences_Cb);
                zig_zag_inverse(tab_Cr, matrice_frequences_Cr);

                // IDCT
                struct matrice_8x8 *matrice_spatial_Y0 = iDCT_bloc_v3(matrice_frequences_Y0);
                struct matrice_8x8 *matrice_spatial_Y1 = iDCT_bloc_v3(matrice_frequences_Y1);
                struct matrice_8x8 *matrice_spatial_Y2 = iDCT_bloc_v3(matrice_frequences_Y2);
                struct matrice_8x8 *matrice_spatial_Y3 = iDCT_bloc_v3(matrice_frequences_Y3);
                struct matrice_8x8 *matrice_spatial_Cb = iDCT_bloc_v3(matrice_frequences_Cb);
                struct matrice_8x8 *matrice_spatial_Cr = iDCT_bloc_v3(matrice_frequences_Cr);

                // YCbCr to RGB
                matrice_image[0] = initialisation_pixel(matrice_spatial_Y0, matrice_spatial_Cb, matrice_spatial_Cr);
                matrice_image[1] = initialisation_pixel(matrice_spatial_Y1, matrice_spatial_Cb, matrice_spatial_Cr);
                matrice_image[nbre_MCU_horizontale] = initialisation_pixel(matrice_spatial_Y2, matrice_spatial_Cb, matrice_spatial_Cr);
                matrice_image[nbre_MCU_horizontale + 1] = initialisation_pixel(matrice_spatial_Y3, matrice_spatial_Cb, matrice_spatial_Cr);
                
                free_matrice(matrice_spatial_Y0);
                free_matrice(matrice_spatial_Y1);
                free_matrice(matrice_spatial_Y2);
                free_matrice(matrice_spatial_Y3);
                free_matrice(matrice_spatial_Cb);
                free_matrice(matrice_spatial_Cr);


                for (uint32_t ligne = 0; ligne < ceil(((float)nbre_MCU_verticale / 2)); ligne++) {
                    for (uint32_t colonne = 0; colonne < ceil(((float)nbre_MCU_horizontale / 2)); colonne++){
                        if (ligne == 0 && colonne == 0){
                            continue;
                        }
                        //Variable Lenght Decoding
                        RLE_huffman(jdesc, stream, tab_Y0, valeur_DC_prec_Y, 0);
                        RLE_huffman(jdesc, stream, tab_Y1, valeur_DC_prec_Y, 0);
                        RLE_huffman(jdesc, stream, tab_Y2, valeur_DC_prec_Y, 0);
                        RLE_huffman(jdesc, stream, tab_Y3, valeur_DC_prec_Y, 0);
                        RLE_huffman(jdesc, stream, tab_Cb, valeur_DC_prec_Cb, 1);
                        RLE_huffman(jdesc, stream, tab_Cr, valeur_DC_prec_Cr, 1);

                        // IQZZ
                        quantification_inverse_MCU_1x64(jdesc, tab_Y0, 0);
                        quantification_inverse_MCU_1x64(jdesc, tab_Y1, 0);
                        quantification_inverse_MCU_1x64(jdesc, tab_Y2, 0);
                        quantification_inverse_MCU_1x64(jdesc, tab_Y3, 0);
                        quantification_inverse_MCU_1x64(jdesc, tab_Cb, 1);
                        quantification_inverse_MCU_1x64(jdesc, tab_Cr, 1);

                        // IZZ
                        zig_zag_inverse(tab_Y0, matrice_frequences_Y0);
                        zig_zag_inverse(tab_Y1, matrice_frequences_Y1);
                        zig_zag_inverse(tab_Y2, matrice_frequences_Y2);
                        zig_zag_inverse(tab_Y3, matrice_frequences_Y3);
                        zig_zag_inverse(tab_Cb, matrice_frequences_Cb);
                        zig_zag_inverse(tab_Cr, matrice_frequences_Cr);

                        // IDCT
                        struct matrice_8x8 *mat_spatial_Y0 = iDCT_bloc_v3(matrice_frequences_Y0);
                        struct matrice_8x8 *mat_spatial_Y1 = iDCT_bloc_v3(matrice_frequences_Y1);
                        struct matrice_8x8 *mat_spatial_Y2 = iDCT_bloc_v3(matrice_frequences_Y2);
                        struct matrice_8x8 *mat_spatial_Y3 = iDCT_bloc_v3(matrice_frequences_Y3);
                        struct matrice_8x8 *mat_spatial_Cb = iDCT_bloc_v3(matrice_frequences_Cb);
                        struct matrice_8x8 *mat_spatial_Cr = iDCT_bloc_v3(matrice_frequences_Cr);

                        // YCbCr to RGB
                        matrice_image[2 * ligne * nbre_MCU_horizontale + 2 * colonne] = initialisation_pixel(mat_spatial_Y0, mat_spatial_Cb, mat_spatial_Cr);
                        if (2 * colonne + 1 < nbre_MCU_horizontale){
                            matrice_image[2 * ligne * nbre_MCU_horizontale + 2 * colonne + 1] = initialisation_pixel(mat_spatial_Y1, mat_spatial_Cb, mat_spatial_Cr);
                        }
                        if ((2 * ligne + 1) * nbre_MCU_horizontale + 2 * colonne < (uint32_t)(nbre_MCU_verticale * nbre_MCU_horizontale)) {
                            matrice_image[(2 * ligne + 1) * nbre_MCU_horizontale + 2 * colonne] = initialisation_pixel(mat_spatial_Y2, mat_spatial_Cb, mat_spatial_Cr);
                            if (2 * colonne + 1 < nbre_MCU_horizontale){
                                matrice_image[(2 * ligne + 1) * nbre_MCU_horizontale + 2 * colonne + 1] = initialisation_pixel(mat_spatial_Y3, mat_spatial_Cb, mat_spatial_Cr);
                            }
                        }
                        free_matrice(mat_spatial_Y0);
                        free_matrice(mat_spatial_Y1);
                        free_matrice(mat_spatial_Y2);
                        free_matrice(mat_spatial_Y3);
                        free_matrice(mat_spatial_Cb);
                        free_matrice(mat_spatial_Cr);
                    }
                }

                free(tab_Y0);
                free(tab_Y1);
                free(tab_Y2);
                free(tab_Y3);
                free(tab_Cb);
                free(tab_Cr);
                free(valeur_DC_prec_Y);
                free(valeur_DC_prec_Cb);
                free(valeur_DC_prec_Cr);
                free_matrice(matrice_frequences_Y0);
                free_matrice(matrice_frequences_Y1);
                free_matrice(matrice_frequences_Y2);
                free_matrice(matrice_frequences_Y3);
                free_matrice(matrice_frequences_Cb);
                free_matrice(matrice_frequences_Cr);

                // Ecriture dans un fichier ppm
                create_ppm_couleur(matrice_image, jdesc);

                for (uint16_t i = 0; i < (uint32_t)(nbre_MCU_verticale * nbre_MCU_horizontale); i++){
                    free_matrice_pixel(matrice_image[i]);
                } free(matrice_image);
                jpeg_close(jdesc);
            }
        }
    }
}
