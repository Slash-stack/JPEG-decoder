#ifndef __CREATEIMAGE_H__
#define __CREATEIMAGE_H__

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>

// Génère le nom du l'image .ppm / .pgm
char *create_name(struct jpeg_desc *jdesc, bool couleur);

// Génère l'image en couleur au format .ppm à partir d'une matrice de MCUs
void create_ppm_couleur(struct matrice_8x8_pixel **matrice, struct jpeg_desc *jdesc);

// Génère l'image en nuance de gris au format .pgm
void create_pgm_gris(struct matrice_8x8 *matrice, struct jpeg_desc *jdesc);

// Affiche un matrice d'entiers dans la console
void print_matrice(struct matrice_8x8 *mat);

// Affiche un matrice de pixels dans la console
void print_matrice_pixel(struct matrice_8x8_pixel *mat);

// Affiche un tableau de pixels dans la console
void print_tab(int16_t *tab);

#endif