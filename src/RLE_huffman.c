#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>
#include "huffman.h"
#include "bitstream.h"
#include "jpeg_reader.h"

/*------------------------------------------------------------------------------------------------------------*/
/*                                                                                                            */
/*                                                                                                            */
/*                                               RLE HUFFMAN                                                  */
/*                                                                                                            */
/*                                                                                                            */
/*------------------------------------------------------------------------------------------------------------*/

// -----------------------------------------------------------

void RLE_huffman(struct jpeg_desc *jdesc, struct bitstream *stream, int16_t *tab, int16_t *valeur_DC_prec, uint8_t mode) {
    /* Lecture des données + Décodage de Huffman*/

    // On stock les tables de Huffman pour DC dans un tableau
    uint8_t n_DC = jpeg_get_nb_huffman_tables(jdesc, 0);
    struct huff_table *table_de_huff_DC[n_DC];
    for (uint8_t i = 0; i < n_DC; i++) {
        table_de_huff_DC[i] = jpeg_get_huffman_table(jdesc, 0, i);
    }
    // On stock les tables de Huffman pour AC dans un tableau
    uint8_t n_AC = jpeg_get_nb_huffman_tables(jdesc, 1);
    struct huff_table *table_de_huff_AC[n_AC];
    for (uint8_t i = 0; i < n_AC; i++) {
        table_de_huff_AC[i] = jpeg_get_huffman_table(jdesc, 1, i);
    }

    uint32_t index = 0;
    while (index < 64){
        if (index != 0){
            int8_t valeur_huff_AC = huffman_next_value(table_de_huff_AC[mode], stream);
            if (valeur_huff_AC != 0x00 && valeur_huff_AC != -16){
                // On isole la magnitude
                uint8_t magnitude = valeur_huff_AC << 4;
                magnitude = magnitude >> 4;
                // on isole le nombre de zéros
                uint32_t nb_zero = ((uint8_t) valeur_huff_AC) >> 4;
                if (magnitude > 10) {
                    magnitude = 10;
                }
                // Ajout des zéros avant la valeur non nulle
                for (uint32_t i = 0; i < nb_zero; i++){
                    tab[index] = 0;
                    index++;
                }
                uint32_t byte = 0;
                // On lit les *magnitude* bits suivants afin d'extraire l'indice binaire
                bitstream_read(stream, magnitude, &byte, true);
                int16_t borne_inf = -(pow(2, magnitude)) + 1;
                uint16_t demi_plage = pow(2, magnitude - 1);
                // Ajout de la valeur non nulle
                if (byte < demi_plage){
                    tab[index] = borne_inf + byte;
                    index++;
                } else {
                    tab[index] = byte;
                    index++;
                }
            } else if (valeur_huff_AC == 0x00) {
                // Cas où toutes les valeurs sont nulles
                while (index < 64) {
                    tab[index] = 0;
                    index++;
                }
                
            } else if (valeur_huff_AC == -16) {
                // Cas où les 16 prochaines valeurs sont nulles
                for (uint8_t k=0; k<16; k++) {
                    tab[index] = 0;
                    index++;
                }
            }
        } else {
            int8_t valeur_huff_DC = huffman_next_value(table_de_huff_DC[mode], stream);
            // On isole la magnitude
            uint8_t magnitude = (valeur_huff_DC << 4) >> 4;
            uint32_t byte = 0;
            // On lit les *magnitude* bits suivants afin d'extraire l'indice binaire
            bitstream_read(stream, magnitude, &byte, true);
            int16_t borne_inf = -(pow(2, magnitude)) + 1;
            uint16_t demi_plage = pow(2, magnitude - 1);
            // Ajout de la valeur non nulle en prenant en compte la valeur précedente de la composante
            if (byte < demi_plage){
                *valeur_DC_prec = *valeur_DC_prec + borne_inf + byte;
                tab[index] = *valeur_DC_prec;
                index++;
            } else {
                *valeur_DC_prec = *valeur_DC_prec + byte;
                tab[index] = *valeur_DC_prec;
                index++;
            }
        }
        
    }
}