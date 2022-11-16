#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

/*------------------------------------------------------------------------------------------------------------*/
/*                                                                                                            */
/*                                                                                                            */
/*                                                 MATRICE                                                    */
/*                                                                                                            */
/*                                                                                                            */
/*------------------------------------------------------------------------------------------------------------*/

// structure pixel
struct pixel {
    uint8_t X, Y, Z;
};

// liste chaînée de matrices de pixels
struct matrice_8x8_pixel {
    struct pixel *tab;
    uint32_t id;
    struct matrice_8x8_pixel *suivant;
};

// liste chainée de matrices d'entiers
struct matrice_8x8 {
    int16_t *tab;
    uint32_t id;
    struct matrice_8x8 *suivant;
};

struct matrice_8x8 *initialisation()
/* initialise une matrice d'entiers */
{
    struct matrice_8x8 *matrice = (struct matrice_8x8 *) malloc(sizeof(struct matrice_8x8));
    matrice->tab = malloc(64 * sizeof(int16_t));
    matrice->suivant = NULL;
    for (int8_t i=0;i<8;i++){
        for (int8_t j=0; j<8; j++){
            matrice->tab[i * 8 + j] = 0;
        }
    }
    return matrice;
};

void free_matrice(struct matrice_8x8 *matrice)
/* free une matrice d'entiers */
{
    free(matrice->tab);
    free(matrice);
}

void free_matrice_pixel(struct matrice_8x8_pixel *matrice)
/* free une matrice de pixels */
{
    free(matrice->tab);
    free(matrice);
}

void free_liste_matrice(struct matrice_8x8 *matrice)
/* free une liste de matrices d'entiers */
{
    free(matrice->tab);
    if (matrice->suivant != NULL) {
        free_liste_matrice(matrice->suivant);
        free(matrice->suivant);
    }
}

void free_liste_matrice_pixel(struct matrice_8x8_pixel *matrice)
/* free une liste de matrices de pixels */
{
    free(matrice->tab);
    if (matrice->suivant != NULL) {
        free_liste_matrice_pixel(matrice->suivant);
        free(matrice->suivant);
    }
}

static struct pixel YCbCr_to_pixel(struct pixel pix)
/* Conversion d'un pixel en YCbCr vers un pixel en RGB */
{
    uint16_t Y = pix.X, Cb = pix.Y, Cr = pix.Z;
    float r =  Y - 0.0009267 * (Cb - 128) + 1.4016868 * (Cr - 128);
    float g =  Y - 0.3436954 * (Cb - 128) - 0.7141690 * (Cr - 128);
    float b =  Y + 1.7721604 * (Cb - 128) + 0.0009902 * (Cr - 128);

    if (r >= 255){
        r = 255;
    } else if (r <= 0){
        r = 0;
    }

    if (g >= 255){
        g = 255;
    } else if (g <= 0){
        g = 0;
    }

    if (b >= 255){
        b = 255;
    } else if (b <= 0){
        b = 0;
    }

    uint16_t R = (uint16_t) r, G = (uint16_t) g, B = (uint16_t) b;
    struct pixel p = {R, G, B};
    return p;
}

struct matrice_8x8_pixel *initialisation_pixel(struct matrice_8x8 *matrice_Y, struct matrice_8x8 *matrice_Cb, struct matrice_8x8 *matrice_Cr) {
    /* Initialise une matrice pixel tout en convertissant les pixels du format YCbCr vers RGB */
    
    struct matrice_8x8_pixel *matrice = (struct matrice_8x8_pixel *) malloc(sizeof(struct matrice_8x8_pixel));
    matrice->tab = malloc(64 * sizeof(struct pixel));
    matrice->suivant = NULL;
    for (uint8_t i=0;i<8;i++){
        for (uint8_t j=0; j<8; j++){
            struct pixel p = {matrice_Y->tab[i * 8 + j], matrice_Cb->tab[i * 8 + j], matrice_Cr->tab[i * 8 + j]};
            matrice->tab[i * 8 + j] = YCbCr_to_pixel(p);
        }
    }
    return matrice;
};

