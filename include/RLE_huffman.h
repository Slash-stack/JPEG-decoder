#ifndef __RLE_HUFFMAN_H__
#define __RLE_HUFFMAN_H__

#include <stdint.h>
#include <stdio.h>
#include "huffman.h"
#include "bitstream.h"
#include "jpeg_reader.h"

extern void RLE_huffman(struct jpeg_desc *jdesc, struct bitstream *stream, int16_t *tab, int16_t *valeur_DC_prec, uint8_t mode);

#endif