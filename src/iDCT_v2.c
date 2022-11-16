#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include "matrice.h"


/*------------------------------------------------------------------------------------------------------------*/
/*                                                                                                            */
/*                                                                                                            */
/*                                                  iDCT v2                                                   */
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

static float * init_cos(uint16_t n){
    /* Calcul tout les cos préalablement et les stock dans un tableau. */

    float pi = acos(-1.0);
    float *tab_cos = malloc(64*sizeof(float));
    for (uint8_t x = 0; x < 8; x++){
        for (uint8_t l = 0; l < 8; l++){
            tab_cos[x*8 + l] = cosf((2 * x + 1) * l * pi/ (2*n));
        }
    }
    return tab_cos;
}

struct matrice_8x8 *iDCT_bloc_v2(struct matrice_8x8 *bloc_f){
    /* prend en argument le bloc fréquentiel et retourne le bloc spatial dans cette version v2       */
    /* on introduit un tableau pour memoriser les cos au lieu de les recalculer à chaque interation. */

    uint16_t n = 8;
    float * tab_cos = init_cos(n);
    struct matrice_8x8 *bloc_s = initialisation();
    for (uint8_t x = 0; x < 8; x++){
        for (uint8_t y = 0; y < 8; y++){
            /* La double somme */
            float sum = 0;
            for (uint8_t l = 0; l < 8; l++){
                for (uint8_t m = 0; m < 8; m++){
                    float prod1 = coeff(l) * coeff(m);
                    float prod3 = tab_cos[x*8 + l];
                    float prod2 = tab_cos[y*8 + m];
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
    }
    free(tab_cos);
    return bloc_s;
}

