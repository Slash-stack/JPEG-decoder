#ifndef __MATRICE_H__
#define __MATRICE_H__

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

// structure pixel
struct pixel {
    uint8_t X, Y, Z;
};

// liste chaînée de matrices de pixels
struct matrice_8x8_pixel {
    struct pixel *tab;
    struct matrice_8x8_pixel *suivant;
};

// liste chainée de matrices d'entiers
struct matrice_8x8 {
    int16_t *tab;
    uint32_t id;
    struct matrice_8x8 *suivant;
};

/* initialise une matrice d'entiers */
extern struct matrice_8x8 *initialisation();

struct matrice_8x8_pixel *initialisation_pixel(struct matrice_8x8 *matrice_Y, struct matrice_8x8 *matrice_Cb, struct matrice_8x8 *matrice_Cr);

/* free une matrice d'entiers */
extern void free_matrice(struct matrice_8x8 *matrice);

/* free une matrice de pixels */
extern void free_matrice_pixel(struct matrice_8x8_pixel *matrice);

/* free une liste chaînée de matrice d'entiers */
extern void free_liste_matrice(struct matrice_8x8 *matrice);

/* free une liste chaînée de matrice de pixels */
extern void free_liste_matrice_pixel(struct matrice_8x8_pixel *matrice);

#endif