#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

/*------------------------------------------------------------------------------------------------------------*/
/*                                                                                                            */
/*                                                                                                            */
/*                                                 Bitstream                                                  */
/*                                                                                                            */
/*                                                                                                            */
/*------------------------------------------------------------------------------------------------------------*/

// -----------------------------------------------------------


struct bitstream {
    // Pointeur vers le fichier en cours de lecture
    FILE *fichier;
    // Reste-t-il des octects à lire après l'octet courant ?
    bool est_vide;
    // Octet qu'on est en train de lire
    uint8_t octet_courant;
    // Position du bit qu'on est en train de lire de gauche à droite
    uint8_t num_bit;
};

// ---------------------Fonctions Annexes---------------------

void octet_suivant(struct bitstream *stream){
    /* On passe à l'octet suivant sur le flux. */

    if (fread(&stream->octet_courant, sizeof(uint8_t), 1, stream->fichier) == 0){
        stream->est_vide = true;
    }
    stream->num_bit = 0;
  }

// -------------------Création et fermeture-------------------

struct bitstream *bitstream_create(const char *filename){
    /* Crée un flux, positionné au début du fichier filename */

    // Lecture du fichier
    FILE *image = fopen(filename, "rb");
    if(image == NULL) {
        fprintf(stderr, "Erreur lors de l'ouverture de l'image %s\n", filename);
        return NULL;
    }
    // Allocation de la variable stream dans le tas
    struct bitstream *stream = malloc(sizeof(struct bitstream));

    if (fread(&stream->octet_courant, sizeof(uint8_t), 1, image) == 0){
        // Si on ne lit rien dans le fichier
        fclose(image);
        return NULL;
    } else {
        stream->num_bit = 0;
        stream->est_vide = false;
        stream->fichier = image;
        return stream;
    }
}

void bitstream_close(struct bitstream *stream){
    /* Sert à fermer le fichier et à désallouer proprement le flux de bit référencé par le pointeur stream */

    fclose(stream->fichier);
    free(stream);
}

// --------------------------Lecture--------------------------

uint8_t bitstream_read(struct bitstream *stream, uint8_t nb_bits, uint32_t *dest, bool discard_byte_stuffing){
    /* Permet de lire des bits et d'avancer dans le flux */

    uint8_t nb_bits_lus = 0;
    uint8_t bit_lu;
    *dest = 0;
    while (nb_bits_lus<nb_bits && !stream->est_vide){
        // on isole le bit à la position indiqué par stream->num_bit
        bit_lu = stream->octet_courant >> (7-stream->num_bit) & 1;
        *dest = (*dest<<1) + bit_lu;
        nb_bits_lus+=1;
        stream->num_bit+=1;
        if (stream->num_bit==8){
            if (stream->octet_courant == 0xff){
            // Cas où on doit faire du byte stuffing
                octet_suivant(stream);
                if (discard_byte_stuffing && stream->octet_courant == 0x00){
                    octet_suivant(stream);
                }
            } else {
                octet_suivant(stream);
            }
        }
    }
    return nb_bits_lus;
}

// --------------------------The end--------------------------

bool bitstream_is_empty(struct bitstream *stream){
    /* Retourne true si le flux a été entièrement parcouru, false s'il reste des bits à lire */

    return (stream->est_vide);
}
