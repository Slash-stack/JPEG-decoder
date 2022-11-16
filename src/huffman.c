#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "bitstream.h"

/*------------------------------------------------------------------------------------------------------------*/
/*                                                                                                            */
/*                                                                                                            */
/*                                                  HUFFMAN                                                   */
/*                                                                                                            */
/*                                                                                                            */
/*------------------------------------------------------------------------------------------------------------*/

// -----------------------------------------------------------

struct huff_table {
    // Arbre binaire
    struct huff_table **fils;
    // Valeur que reprèsente le chemin suivi pour atteindre ce noeuds
    uint8_t valeur;
    // booléen qui nous renvoie si le noeuds où on se trouve contient un mot de code ou pas
    bool est_mot_de_code;
};

// ---------------------Fonctions Annexes---------------------

static struct huff_table *init_huff_table() {
    /* Initialise une table de huffman sans fils ni valeurs */

    struct huff_table *nouvelle_table = malloc(sizeof(struct huff_table));
    nouvelle_table->fils = malloc(2*sizeof(struct huff_table*));
    nouvelle_table->fils[0] = NULL;
    nouvelle_table->fils[1] = NULL;
    nouvelle_table->valeur = 0;
    nouvelle_table->est_mot_de_code = false;
    return nouvelle_table;
}

static uint8_t calcule_hauteur_max(uint8_t *nb_valeurs) {
    /* Retourne la hauteur maximale des codes dans l'arbre */

    uint8_t hauteur_max = 0;
    for(uint8_t i = 0; i < 16; i ++)
        if(nb_valeurs[i] != 0)
            hauteur_max = i;
    return hauteur_max +1;
}

struct huff_table *prochain_noeud_vide(struct huff_table **ensemble_noeud_etage_courant, uint8_t taille_ensemble_noeud_etage_courant) {
    /* Trouve le prochain noeuds vide l'initialise et le renvoie (renvoie NULL sinon) */
    
    for(uint8_t i = 0; i < taille_ensemble_noeud_etage_courant; i ++) {
        for(uint8_t j = 0; j < 2; j ++)
            if(ensemble_noeud_etage_courant[i]->fils[j] == NULL) {
                ensemble_noeud_etage_courant[i]->fils[j] = init_huff_table();
                return ensemble_noeud_etage_courant[i] -> fils[j];
            }
    }
    return NULL;
}


// --------------------------Création-------------------------

struct huff_table *huffman_load_table(struct bitstream *stream, uint16_t *nb_byte_read) {
    /* parcourt le flux de bits stream pour construire une table de Huffman propre à l'image en cours de décodage */

    struct huff_table *nouvelle_table = init_huff_table();
    *nb_byte_read = 0;

    uint32_t byte = 0;
    uint8_t nb_symboles[16]; // nombre de symboles de même longueur allant de 1 à 16
    for(uint8_t i = 0; i < 16; i ++) {
        *nb_byte_read += bitstream_read(stream, 8, &byte, false) / 8;
        nb_symboles[i] = byte;
    }

    // On lit ensuite les valeurs en construisant l'arbre de Huffman
    struct huff_table **ensemble_noeud_courant = malloc(sizeof(struct huff_table*));

    *ensemble_noeud_courant = nouvelle_table;

    uint16_t taille_ensemble_noeud_etage_courant = 1;

    struct huff_table *nouveau_noeud;
    uint8_t hauteur_max = calcule_hauteur_max(nb_symboles);

    for(uint8_t i = 0; i < hauteur_max; i ++) {
        // On boucle sur la hauteur max afin d'éviter d'avoir une table de huffman avec plein de npeuds vides inutiles
        for(uint8_t j = 0; j < nb_symboles[i]; j ++) {

            *nb_byte_read += bitstream_read(stream, 8, &byte, false) / 8;
            nouveau_noeud = prochain_noeud_vide(ensemble_noeud_courant, taille_ensemble_noeud_etage_courant);
            // On trouve le prochain noeuds à remplir
            if(nouveau_noeud != NULL) {
                nouveau_noeud->valeur = (uint8_t) byte;
                nouveau_noeud->est_mot_de_code = true;
            } else {
                printf("[HUFFMAN] : Erreur lors de la création de la table de Huffman.\n");
            }
        }

        if(i != hauteur_max -1) {
            // on calcule le nombre de noeuds vide de l'etage courant
            uint16_t nb_noeuds_vide = 2 * taille_ensemble_noeud_etage_courant - nb_symboles[i];

            struct huff_table **ensemble_noeud_suivant = malloc(sizeof(struct huff_table*)*nb_noeuds_vide);
            uint16_t k = 0;
            // on complete l'étage courant avec des noeuds vide construire l'étage suivant
            struct huff_table *noeud_pour_completer_etage = prochain_noeud_vide(ensemble_noeud_courant, taille_ensemble_noeud_etage_courant);
            while(noeud_pour_completer_etage != NULL) {
                ensemble_noeud_suivant[k] = noeud_pour_completer_etage;
                noeud_pour_completer_etage = prochain_noeud_vide(ensemble_noeud_courant, taille_ensemble_noeud_etage_courant);
                k ++;
            }
            // on remplace notre etage
            free(ensemble_noeud_courant);
            ensemble_noeud_courant = ensemble_noeud_suivant;
            taille_ensemble_noeud_etage_courant = nb_noeuds_vide;
        }
    }

    free(ensemble_noeud_courant);
    return nouvelle_table;
}

// --------------Lecture de la prochaine valeur---------------

int8_t huffman_next_value(struct huff_table *table, struct bitstream *stream) {
    /* Renvoie une valeur si obtenue en parcourant l'arbre parallèlement au fichier */

    uint32_t byte = 0;
    while(!table->est_mot_de_code) {
        // tant qu'on ne se trouve pas dans un mot de code
        bitstream_read(stream, 1, &byte, true); // on lit le bit suivant
        // on avance dans l'arbre en vérifiant qu'il n'y a pas d'erreur dans le fichier
        if((byte != 0 && byte != 1) || table -> fils[byte] == NULL) {
            printf("[HUFFMAN] : Erreur lors du décodage de Huffman.\n");
            exit(1);
        } else table = table->fils[byte];
    }
    return table->valeur;
}

// ------------------------Destruction------------------------

void huffman_free_table(struct huff_table *table) {
    /* free la table de huffman donnée en paramètre */

    if(table != NULL) {
        huffman_free_table(table -> fils[1]);
        huffman_free_table(table -> fils[0]);
        free(table -> fils);
        free(table);
    }
}