#ifndef __IDCT_H__
#define __IDCT_H__

#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include "matrice.h"

// prend en argument le bloc fréquentiel et retourne le bloc spatial
extern struct matrice_8x8 *iDCT_bloc(struct matrice_8x8 *bloc_f);

#endif
