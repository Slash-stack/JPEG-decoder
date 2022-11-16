#ifndef __IDCT_V3_H__
#define __IDCT_V3_H__

#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include "matrice.h"

// prend en argument le bloc frÃ©quentiel et retourne le bloc spatial
extern struct matrice_8x8 *iDCT_bloc_v3(struct matrice_8x8 *bloc_f);

#endif