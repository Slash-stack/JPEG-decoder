#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include "matrice.h"

/*------------------------------------------------------------------------------------------------------------*/
/*                                                                                                            */
/*                                                                                                            */
/*                                                    iDCT                                                    */
/*                                                                                                            */
/*                                                                                                            */
/*------------------------------------------------------------------------------------------------------------*/

// -----------------------------------------------------------

static float coeff(uint8_t ksi){
    /* retourne le coefficient C en float */

    if (ksi == 0){
        return 1 / sqrt(2);
    
    } else {
        return 1;
    }
};


struct matrice_8x8 *iDCT_bloc(struct matrice_8x8 *bloc_f){
    /* prend en argument le bloc fréquentiel et retourne le bloc spatial */

    struct matrice_8x8 *bloc_s = initialisation();
    uint16_t n = 8;
    float pi = acos(-1.0);
    for (uint8_t x = 0; x < 8; x++){
        for (uint8_t y = 0; y < 8; y++){
            /* La double somme */
            float sum = 0;
            for (uint8_t l = 0; l < 8; l++){
                for (uint8_t m = 0; m < 8; m++){
                    float prod1 = coeff(l) * coeff(m);
                    float prod3 = cosf((2 * x + 1) * l * pi/ (2*n));
                    float prod2 = cosf((2 * y + 1) * m * pi/ (2*n));
                    sum = sum + prod1 * prod2 * prod3 * bloc_f->tab[l * 8 + m];
                }
            }
            sum *= (1/sqrt(2*n));
            sum += 128;
            if (sum > 255){
                bloc_s->tab[x * 8 + y] = (uint16_t) 255;
            } else if (sum < 0) {
                bloc_s->tab[x * 8 + y] = (uint16_t)0;
            } else {
                bloc_s->tab[x * 8 + y] = (uint16_t) sum;
            }
        }
    } return bloc_s;
}

