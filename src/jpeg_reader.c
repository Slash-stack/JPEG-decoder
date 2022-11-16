#include <stdlib.h>
#include <stdio.h>

#include "jpeg_reader.h"
#include "bitstream.h"
#include "huffman.h"

/*------------------------------------------------------------------------------------------------------------*/
/*                                                                                                            */
/*                                                                                                            */
/*                                                JPEG READER                                                 */
/*                                                                                                            */
/*                                                                                                            */
/*------------------------------------------------------------------------------------------------------------*/

// -----------------------------------------------------------

struct jpeg_desc{
    char *filename;
    struct bitstream *stream;

    // from DQT
    uint8_t nb_quantization_tables;
    uint8_t **quantization_tables;

    // from DHT
    uint8_t *nb_huffman_tables;
    struct huff_table **huffman_table_AC;
    struct huff_table **huffman_table_DC;

    // from Frame Header SOF0
    uint16_t *image_size;
    uint8_t nb_components;
    uint8_t *frame_component_id;
    uint8_t *frame_component_sampling_factor_H;
    uint8_t *frame_component_sampling_factor_V;
    uint8_t *frame_component_quant_index;

    // from Scan Header SOS
    uint8_t *scan_component_id;
    uint8_t *scan_component_huffman_index_AC;
    uint8_t *scan_component_huffman_index_DC;
};


// --------Ouverture, fermeture et fonctions générales--------

struct jpeg_desc *jpeg_read(const char *filename){
    /* ouvre le fichier JPEG, lit tous les entêtes de sections rencontrés */
    /* et stocke les informations requises dans le descripteur retourné.  */
    /* La lecture est stoppée au début des données brutes codant l'image  */
    /* (après l'entête de la section SOS).                                */

    struct jpeg_desc *jpeg = malloc(sizeof(struct jpeg_desc));
    jpeg->filename = (char *) filename;
    struct bitstream *stream = bitstream_create(filename);
    
    jpeg->quantization_tables = malloc(4 * sizeof(uint8_t *));
    jpeg->nb_huffman_tables = malloc(2 * sizeof(uint8_t));
    jpeg->huffman_table_AC = malloc(4 * sizeof(struct huff_table *));
    jpeg->huffman_table_DC = malloc(4 * sizeof(struct huff_table *));
    
    jpeg->image_size = malloc(2 * sizeof(uint16_t));
    jpeg->frame_component_id = malloc(3 * sizeof(uint8_t));
    jpeg->frame_component_sampling_factor_H = malloc(3 * sizeof(uint8_t));
    jpeg->frame_component_sampling_factor_V = malloc(3 * sizeof(uint8_t));
    jpeg->frame_component_quant_index = malloc(4 * sizeof(uint8_t));
    jpeg->scan_component_id = malloc(3 * sizeof(uint8_t));
    jpeg->scan_component_huffman_index_AC = malloc(3 * sizeof(uint8_t));
    jpeg->scan_component_huffman_index_DC = malloc(3 * sizeof(uint8_t));

    uint32_t byte = 0;
    bitstream_read(stream, 8 * 2, &byte, false);
    if (byte != 0xffd8) {
        fprintf(stderr, "Mauvais format de fichier...\n");
        exit(1);
    } bitstream_read(stream, 8 * 2, &byte,false);

    for(uint8_t index = 0; index < 4; index++){
        jpeg->quantization_tables[index] = NULL;
        jpeg->huffman_table_AC[index] = NULL;
        jpeg->huffman_table_DC[index] = NULL;
        jpeg->frame_component_quant_index[index] = 0;
        if (index != 3) {
            jpeg->frame_component_id[index] = 0;
            jpeg->frame_component_sampling_factor_H[index] = 0;
            jpeg->frame_component_sampling_factor_V[index] = 0;
            jpeg->scan_component_id[index] = 0;
            jpeg->scan_component_huffman_index_AC[index] = 0;
            jpeg->scan_component_huffman_index_DC[index] = 0;
            if (index != 2) {
                jpeg->nb_huffman_tables[index] = 0;
                jpeg->image_size[index] = 0;
            }
        }
    }

    bool not_EOF = true;
    while (not_EOF) {
        while (!bitstream_is_empty(stream) && byte != 0xff) { // On repère une nouvelle section
            bitstream_read(stream, 8, &byte, false);
        } bitstream_read(stream, 8, &byte, false);

        if (byte == 0xe0) { // Sectioon APPx
            continue;
        } else if (byte == 0xfe) { // Section COM
            continue;
        } else if (byte == 0xdb) { // Section DQT
            bitstream_read(stream, 8 * 2, &byte, false);
            uint8_t nb_quantization_tables = (byte - 2) / 65;
            jpeg->nb_quantization_tables = nb_quantization_tables;
            for (uint8_t i = 0; i < nb_quantization_tables; i++) {
                uint8_t *quantization_table = malloc(64 * sizeof(uint8_t));
                bitstream_read(stream, 4, &byte, false);
                bitstream_read(stream, 4, &byte, false);
                uint8_t index = byte;
                for(uint8_t j = 0; j < 64 ;j++){
                    bitstream_read(stream, 8, &byte, false);
                    quantization_table[j] = byte;
                } jpeg->quantization_tables[index] = quantization_table;
            }
        } else if (byte == 0xc0) { // Section SOF0
            bitstream_read(stream, 8 * 3, &byte, false);
            bitstream_read(stream, 8 * 2, &byte, false);
            jpeg->image_size[1]= byte;
            bitstream_read(stream, 8 * 2, &byte, false);
            jpeg->image_size[0]= byte;
            bitstream_read(stream, 8, &byte, false);
            jpeg->nb_components = byte;
            for (uint8_t i = 0; i < jpeg->nb_components; i++) {
                bitstream_read(stream, 8, &byte, false);
                jpeg->frame_component_id[i] = byte;
                bitstream_read(stream, 4, &byte, false);
                jpeg->frame_component_sampling_factor_H[i] = byte;
                bitstream_read(stream, 4, &byte, false);
                jpeg->frame_component_sampling_factor_V[i] = byte;
                bitstream_read(stream, 8, &byte, false);
                jpeg->frame_component_quant_index[i] = byte;
            }
        } else if (byte == 0xc4) { // Section DHT
            bitstream_read(stream, 8 * 2, &byte,false);
            uint16_t longueur = byte - 2;
            uint16_t symboles_lus = 0;

            while (symboles_lus < longueur){
                bitstream_read(stream, 3, &byte, false);
                bitstream_read(stream, 1, &byte, false);
                uint8_t type = byte;
                uint16_t i = 0;
                uint16_t *nb_symboles_lus = &i ;
                jpeg -> nb_huffman_tables[type] += 1;
                bitstream_read(stream, 4, &byte, false);
                symboles_lus += 1;
                if (type == DC) {
                    jpeg->huffman_table_DC[byte] = huffman_load_table(stream, nb_symboles_lus);
                } else {
                    *nb_symboles_lus = 0;
                    jpeg->huffman_table_AC[byte] = huffman_load_table(stream, nb_symboles_lus);            
                } symboles_lus += *nb_symboles_lus;
            }
        } else if (byte == 0xda) { // Section SOS
            bitstream_read(stream, 8 * 2, &byte,false);
            bitstream_read(stream, 8, &byte,false);
            uint32_t N = byte;
            for (uint32_t i = 0; i < N; i++) {
                bitstream_read(stream, 8, &byte,false);
                jpeg->scan_component_id[i] = byte;
                bitstream_read(stream, 4, &byte,false);
                jpeg->scan_component_huffman_index_DC[i] = byte;
                bitstream_read(stream, 4, &byte,false);
                jpeg->scan_component_huffman_index_AC[i] = byte;
            } bitstream_read(stream, 8 * 3, &byte, false);
            jpeg->stream = stream;
            not_EOF = false;
        } else {
            fprintf(stderr, "Header non reconnu / cas non traité...\n");
            exit(1);
        }
    }
    return jpeg;
}

void jpeg_close(struct jpeg_desc *jpeg){
    /* Ferme un descripteur préalablement ouvert, en libérant toute la mémoire nécessaire */

    bitstream_close(jpeg->stream);
    free(jpeg->scan_component_huffman_index_DC);
    free(jpeg->scan_component_huffman_index_AC);
    free(jpeg->scan_component_id);
    free(jpeg->frame_component_quant_index);
    free(jpeg->frame_component_sampling_factor_V);
    free(jpeg->frame_component_sampling_factor_H);
    free(jpeg->frame_component_id);
    free(jpeg->image_size);
    free(jpeg->nb_huffman_tables);
    for(uint8_t index = 0; index < 4; index++){
        if(jpeg->huffman_table_AC[index] != NULL){
            huffman_free_table(jpeg->huffman_table_AC[index]);
        }
        if(jpeg->huffman_table_DC[index] != NULL){
            huffman_free_table(jpeg->huffman_table_DC[index]);
        }
        if(jpeg->quantization_tables[index] != NULL){
            free(jpeg->quantization_tables[index]);
        }
    } 
    free(jpeg->huffman_table_AC);
    free(jpeg->huffman_table_DC);
    free(jpeg->quantization_tables);
    free(jpeg);
}

char *jpeg_get_filename(const struct jpeg_desc *jpeg){
    /* retourne le nom de fichier de l'image ouverte */

    return jpeg->filename;
}

struct bitstream *jpeg_get_bitstream(const struct jpeg_desc *jpeg){
    /* retourne l'adresse d'une variable de type       */
    /* struct bitstream permettant de lire les données */
    /* brutes d'image encodées dans le flux            */
    return jpeg->stream;
}


// -----------------Tables de quantifications-----------------

extern uint8_t jpeg_get_nb_quantization_tables(const struct jpeg_desc *jpeg){
    /* retourne le nombre de tables de quantification */

    return jpeg->nb_quantization_tables;
}

extern uint8_t *jpeg_get_quantization_table(const struct jpeg_desc *jpeg, uint8_t index){
    /* retourne l'adresse mémoire de la i-ième table         */
    /* de quantification, un tableau de 64 octets non-signés */

    return jpeg->quantization_tables[index];
}


// ---------------------Tables de Huffman---------------------

uint8_t jpeg_get_nb_huffman_tables(const struct jpeg_desc *jpeg, enum sample_type acdc){
    /* retourne le nombre de tables de Huffman de type sample_type (DC ou AC) */

    return jpeg->nb_huffman_tables[acdc];
}

struct huff_table *jpeg_get_huffman_table(const struct jpeg_desc *jpeg, enum sample_type acdc, uint8_t index){
    /* retourne un pointeur vers la i-ième table de Huffman de type DC ou AC */

    if (acdc == AC) {
        return jpeg->huffman_table_AC[index];
    } else {
        return jpeg->huffman_table_DC[index];
    }
}


// ------------------Données du Frame Header-----------------

uint16_t jpeg_get_image_size(struct jpeg_desc *jpeg, enum direction dir){
    /* retourne la taille de l'image (nombre de pixels) dans */
    /* la direction dir (horizontale ou verticale)           */

    return jpeg->image_size[dir];
}

uint8_t jpeg_get_nb_components(const struct jpeg_desc *jpeg){
    /* retourne le nombre de composantes de couleur                         */
    /* (1 pour une image en niveau de gris, 3 pour une image en YCbCr, ...) */

    return jpeg->nb_components;
}

uint8_t jpeg_get_frame_component_id(const struct jpeg_desc *jpeg, uint8_t frame_comp_index){
    /* retourne l'identifiant de la i\ieme composante définie dans le Frame Header */

    return jpeg->frame_component_id[frame_comp_index];
}

uint8_t jpeg_get_frame_component_sampling_factor(const struct jpeg_desc *jpeg, enum direction dir, uint8_t frame_comp_index){
    /* retourne le facteur de sous-échantillonnage dans      */
    /* la direction dir de la i-i ème composante de couleur. */

    if (dir == H) {
        return jpeg->frame_component_sampling_factor_H[frame_comp_index];
    } else if (dir == V) {
        return jpeg->frame_component_sampling_factor_V[frame_comp_index];
    } else {
        return 0; // On fait quoi dans ce cas ? 
    }
}

uint8_t jpeg_get_frame_component_quant_index(const struct jpeg_desc *jpeg, uint8_t frame_comp_index){
    /* retourne l'index de la table de quantification */
    /* de la i-ième composante de couleur.            */

    return jpeg->frame_component_quant_index[frame_comp_index];
}


// -------------------Données du Scan Header------------------

uint8_t jpeg_get_scan_component_id(const struct jpeg_desc *jpeg, uint8_t scan_comp_index){
    /* retourne l'identifiant de la i-ième composante lue dans le scan */

    return jpeg->scan_component_id[scan_comp_index];
}

uint8_t jpeg_get_scan_component_huffman_index(const struct jpeg_desc *jpeg, enum sample_type acdc, uint8_t scan_comp_index){
    /* retourne l'index de la table de Huffman (DC ou AC) */
    /* associée à la i-ième composante lue dans le scan   */

    if (acdc == AC) {
        return jpeg->scan_component_huffman_index_AC[scan_comp_index];
    } else if (acdc == DC) {
        return jpeg->scan_component_huffman_index_DC[scan_comp_index];
    } else {
        return 0; // On fait quoi dans ce cas ? 
    }
}
