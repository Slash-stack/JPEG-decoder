#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "jpeg_reader.h"
#include "bitstream.h"
#include "matrice.h"


/*------------------------------------------------------------------------------------------------------------*/
/*                                                                                                            */
/*                                                                                                            */
/*                                        Quantification Zig-Zag                                              */
/*                                                                                                            */
/*                                                                                                            */
/*------------------------------------------------------------------------------------------------------------*/

// -----------------------------------------------------------

void quantification_inverse_MCU_1x64(const struct jpeg_desc *jpeg, int16_t *bloc, uint8_t mode){
/* retourne un vecteur de dimension 1x64 qui résulte de la multiplication des coefficients */
/* par les coefficients de la matrice de quantification de Y.                              */

    uint8_t *matrice_Y = jpeg_get_quantization_table(jpeg, mode);
    for (uint8_t i = 0; i < 64; i++){
        bloc[i] = bloc[i] * matrice_Y[i];
    }
}

void zig_zag_inverse(int16_t *bloc, struct matrice_8x8 *matrice){
/* retourne une matrice de dimension 8x8 à partir d'un vecteur 1x64 à l'aide d'un parcours */
/* en zig-zag.                                                                             */

    uint8_t compteur = 0;

    for (int z = 0; z < 8; z++) {
        if (z % 2 == 0) {
            for (int i = 0; i <= z; i++) {
                matrice->tab[(z - i) * 8 + i] = bloc[compteur];
                compteur++;
            }
        } else {
            for (int i = 0; i <= z; i++) {
                matrice->tab[i * 8 + z - i] = bloc[compteur];
                compteur++;
            }
        }
    }

    for (int z = 1; z < 8; z++) {
        if (z % 2 == 8 % 2) {
            for (int i = 1; i <= 8 - z; i++) {
                matrice->tab[(z + i - 1) * 8 + 8 - i] = bloc[compteur];
                compteur++;
            }
        } else {
            for (int i = 1; i <= 8 - z; i++) {
                matrice->tab[(8 - i) * 8 + z + i - 1] = bloc[compteur];
                compteur++;
            }
        }
    }
}