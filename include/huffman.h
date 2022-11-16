#ifndef __HUFFMAN_H__
#define __HUFFMAN_H__

#include <stdint.h>
#include <stdbool.h>
#include "bitstream.h"


struct huff_table;

/* parcourt le flux de bits stream pour construire une table de Huffman propre à l'image en cours de décodage */
extern struct huff_table *huffman_load_table(struct bitstream *stream, uint16_t *nb_byte_read);

/* Renvoie une valeur si obtenue en parcourant l'arbre parallèlement au fichier */
extern int8_t huffman_next_value(struct huff_table *table, struct bitstream *stream);

/* free la table de huffman donnée en paramètre */
extern void huffman_free_table(struct huff_table *table);

#endif
