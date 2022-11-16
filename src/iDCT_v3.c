#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include "matrice.h"


/*------------------------------------------------------------------------------------------------------------*/
/*                                                                                                            */
/*                                                                                                            */
/*                                                  iDCT v3                                                   */
/*                                                                                                            */
/*                                                                                                            */
/*------------------------------------------------------------------------------------------------------------*/

// -----------------------------------------------------------


float * multiplication(float * vecteur, float a){
    /* Prends un vecteur de taille 8 et le multiplie par une constante */
    float * res = malloc(8*sizeof(float));
    for(uint8_t i=0; i<8 ;i++){
        res[i]=a*vecteur[i];
    }
    return res;
}
void rotation_inverse(float * i0, float * i1, float o0, float o1, uint8_t n, float k){
    float pi = acos(-1);
    * i0 = (o0*cosf((n*pi)/16) - o1*sinf((n*pi)/16))/k;
    * i1 = (o1*cosf((n*pi)/16) + o0*sinf((n*pi)/16))/k;
};

void somme_inverse(float * i0, float * i1, float o0, float o1){
    * i0 = (o0 + o1)/2 ;
    * i1 = (o0 - o1)/2 ;
};

float * loeffler_inverse(float * vecteur){
    float * vecteur0;
    float vecteur1[8];
    float vecteur2[8];
    float vecteur3[8];
    float vecteur4[8];
    /*On multiplie par 2sqrt(2)*/
    vecteur0 = multiplication(vecteur,2*sqrt(2));

    /* PremiÃ¨re Ã©tape (indice pairs restent les mÃªmes, division par sqrt(2) de (3,5) et rotation_inverse(1,7)) */
    vecteur1[0] = vecteur0[0];
    vecteur1[2] = vecteur0[2];
    vecteur1[4] = vecteur0[4];
    vecteur1[6] = vecteur0[6];
    vecteur1[3] = vecteur0[3]/sqrt(2);
    vecteur1[5] = vecteur0[5]/sqrt(2);
    somme_inverse(&vecteur1[1],&vecteur1[7],vecteur0[1],vecteur0[7]);

    /* DeuxiÃ¨me Ã©tape (Sommes inverses et rotations inverses) */
    somme_inverse(&vecteur2[0],&vecteur2[4],vecteur1[0],vecteur1[4]);
    somme_inverse(&vecteur2[7],&vecteur2[5],vecteur1[7],vecteur1[5]);
    somme_inverse(&vecteur2[1],&vecteur2[3],vecteur1[1],vecteur1[3]);
    rotation_inverse(&vecteur2[2],&vecteur2[6],vecteur1[2],vecteur1[6],6,sqrt(2));

    /* TroisiÃ¨me Ã©tape */
    somme_inverse(&vecteur3[0],&vecteur3[6],vecteur2[0],vecteur2[6]);
    somme_inverse(&vecteur3[4],&vecteur3[2],vecteur2[4],vecteur2[2]);
    rotation_inverse(&vecteur3[7],&vecteur3[1],vecteur2[7],vecteur2[1],3,1);
    rotation_inverse(&vecteur3[3],&vecteur3[5],vecteur2[3],vecteur2[5],1,1);

    /* QuatriÃ¨me Ã©tape */
    somme_inverse(&vecteur4[0],&vecteur4[1],vecteur3[0],vecteur3[1]);
    somme_inverse(&vecteur4[4],&vecteur4[5],vecteur3[4],vecteur3[5]);
    somme_inverse(&vecteur4[2],&vecteur4[3],vecteur3[2],vecteur3[3]);
    somme_inverse(&vecteur4[6],&vecteur4[7],vecteur3[6],vecteur3[7]);

    float * resultat = malloc(8*sizeof(float));
    resultat[0] = vecteur4[0];
    resultat[1] = vecteur4[4];
    resultat[2] = vecteur4[2];
    resultat[3] = vecteur4[6];
    resultat[4] = vecteur4[7];
    resultat[5] = vecteur4[3];
    resultat[6] = vecteur4[5];
    resultat[7] = vecteur4[1];
    free(vecteur0);
    return resultat;
}

float * recupere_colonne(struct matrice_8x8 *bloc_f, uint8_t j){
    /* Recupere la jÃ¨me colonne de la matrice et retourne un vecteur 8x1 */
    float * vecteur = malloc(8*sizeof(float));
    for(uint8_t i = 0; i < 8; i++){
            vecteur[i] = (float)bloc_f->tab[8*i + j];
        }
    return vecteur;
};

float * recupere_ligne(float * tableau, uint8_t i){
    /* Recupere la iÃ¨me ligne du tableau et retourne un vecteur 8x1 */
    float * vecteur = malloc(8*sizeof(float));
    for(uint8_t j = 0; j < 8; j++){
            vecteur[j] = (float)tableau[8*i + j];
        }
    return vecteur;
};


struct matrice_8x8 *iDCT_bloc_v3(struct matrice_8x8 *bloc_f){
    struct matrice_8x8 *bloc_s = initialisation();
    float * tab = malloc(64*sizeof(float));
    /* loeffler sur les colonnes */
    for(uint8_t j = 0; j< 8; j++){
        float * vecteur_c = recupere_colonne(bloc_f, j);
        float * vecteur_c_loeffler = loeffler_inverse(vecteur_c);
        for(uint8_t i = 0; i < 8; i++){
            tab[8*i + j] = vecteur_c_loeffler[i];
        }
        free(vecteur_c);
        free(vecteur_c_loeffler);
    }
    /* loeffler sur les lignes */
    for(uint8_t i = 0; i < 8; i++){
        float * vecteur_l = recupere_ligne(tab, i);
        float* vecteur_l_loeffler = loeffler_inverse(vecteur_l);
        for(uint8_t j = 0; j < 8; j++){
            float res = vecteur_l_loeffler[j];
            res += 128;
            if(res < 0){
                res = 0;
            }
            else if (res > 255){
                res = 255;
            }
            //res = (uint16_t) res;
            bloc_s->tab[8*i + j] = (uint16_t) res;
        }
        free(vecteur_l);
        free(vecteur_l_loeffler);
    }
    free(tab);
    return bloc_s;
}   