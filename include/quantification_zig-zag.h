#ifndef __QUANTIFICATION_ZIG_ZAG_H__
#define __QUANTIFICATION_ZIG_ZAG_H__

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>


// retourne un vecteur de dimension 1x64 qui résulte de la multiplication des coefficients
// par les coefficients de la matrice de quantification de Y
extern void quantification_inverse_MCU_1x64(const struct jpeg_desc *jpeg, int16_t *bloc, uint8_t mode);

// retourne une matrice de dimension 8x8 à partir d'un vecteur 1x64 à l'aide d'un parcours
// en zig-zag
extern void zig_zag_inverse(int16_t *bloc, struct matrice_8x8 * matrice);

#endif
