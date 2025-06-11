#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <MLV/MLV_all.h>

#define TAILLE 5
#define TAILLE_CASE 100
#define MAX_MOUVEMENTS 100

typedef enum {
    BLANC,
    NOIR
} Couleur;

typedef enum {
    CHEVALIER,
    ESPION
} Type;

typedef struct _pion {
    Type type;
    Couleur couleur;
} Pion;

typedef struct {
    int x, y;
} Case;

typedef struct _mouvement {
    Case depart;
    Case arrivee;
    bool est_interrogation;
} Mouvement;

typedef struct _jeu {
    Pion *plateau[TAILLE][TAILLE];
    Couleur joueur;
    int nb_mouvements;
    Mouvement historique[MAX_MOUVEMENTS];
    bool partie_terminee;
} Jeu;

// Prototypes
bool sont_cases_adjacentes(Case c1, Case c2);
bool est_chateau(int x, int y, Couleur couleur);
void liberer_jeu(Jeu *jeu);

// Initialisation du jeu
Jeu *initialiser_jeu() {
    Jeu *jeu = malloc(sizeof(Jeu));
    if (!jeu) {
        fprintf(stderr, "Erreur d'allocation mémoire\n");
        return NULL;
    }

    for (int i = 0; i < TAILLE; i++) {
        for (int j = 0; j < TAILLE; j++) {
            jeu->plateau[i][j] = NULL;
        }
    }

    // Initialiser les pions noirs
    jeu->plateau[0][2] = malloc(sizeof(Pion));
    jeu->plateau[0][2]->couleur = NOIR;
    jeu->plateau[0][2]->type = CHEVALIER;

    jeu->plateau[0][3] = malloc(sizeof(Pion));
    jeu->plateau[0][3]->couleur = NOIR;
    jeu->plateau[0][3]->type = CHEVALIER;

    jeu->plateau[1][3] = malloc(sizeof(Pion));
    jeu->plateau[1][3]->couleur = NOIR;
    jeu->plateau[1][3]->type = CHEVALIER;

    jeu->plateau[1][4] = malloc(sizeof(Pion));
    jeu->plateau[1][4]->couleur = NOIR;
    jeu->plateau[1][4]->type = CHEVALIER;

    // Initialiser les pions blancs
    jeu->plateau[2][0] = malloc(sizeof(Pion));
    jeu->plateau[2][0]->couleur = BLANC;
    jeu->plateau[2][0]->type = CHEVALIER;

    jeu->plateau[3][0] = malloc(sizeof(Pion));
    jeu->plateau[3][0]->couleur = BLANC;
    jeu->plateau[3][0]->type = CHEVALIER;

    jeu->plateau[4][1] = malloc(sizeof(Pion));
    jeu->plateau[4][1]->couleur = BLANC;
    jeu->plateau[4][1]->type = CHEVALIER;

    jeu->plateau[4][2] = malloc(sizeof(Pion));
    jeu->plateau[4][2]->couleur = BLANC;
    jeu->plateau[4][2]->type = CHEVALIER;

    // Placer les espions
    jeu->plateau[2][4] = malloc(sizeof(Pion));
    jeu->plateau[2][4]->couleur = NOIR;
    jeu->plateau[2][4]->type = ESPION;

    jeu->plateau[3][1] = malloc(sizeof(Pion));
    jeu->plateau[3][1]->couleur = BLANC;
    jeu->plateau[3][1]->type = ESPION;

    jeu->joueur = BLANC;
    jeu->nb_mouvements = 0;
    jeu->partie_terminee = false;

    return jeu;
}

void afficher_plateau_ascii(const Jeu *jeu) {
    printf("-----------\n");
    for (int i = 0; i < TAILLE; i++) {
        printf("|");
        for (int j = 0; j < TAILLE; j++) {
            if (jeu->plateau[i][j] == NULL) {
                printf(" |");
            } else if (jeu->plateau[i][j]->couleur == BLANC) {
                printf("b|");
            } else {
                printf("n|");
            }
        }
        printf("\n-----------\n");
    }
}

void afficher_plateau_graphique(const Jeu *jeu) {
    MLV_clear_window(MLV_COLOR_WHITE);

    for (int i = 0; i < TAILLE; i++) {
        for (int j = 0; j < TAILLE; j++) {
            int x = j * TAILLE_CASE;
            int y = i * TAILLE_CASE;
            
            // Dessiner la case
            MLV_Color couleur_case = ((i + j) % 2 == 0) ? MLV_COLOR_LIGHT_GRAY : MLV_COLOR_WHITE;
            MLV_draw_filled_rectangle(x, y, TAILLE_CASE, TAILLE_CASE, couleur_case);
            
            // Dessiner le pion
            if (jeu->plateau[i][j] != NULL) {
                MLV_Color couleur_pion = (jeu->plateau[i][j]->couleur == BLANC) ? 
                    MLV_COLOR_WHITE_SMOKE : MLV_COLOR_BLACK;
                MLV_draw_filled_circle(x + TAILLE_CASE/2, y + TAILLE_CASE/2, 
                    TAILLE_CASE/3, couleur_pion);
                MLV_draw_circle(x + TAILLE_CASE/2, y + TAILLE_CASE/2, 
                    TAILLE_CASE/3, MLV_COLOR_BLACK);
            }
        }
    }
    
    MLV_actualise_window();
}

bool est_mouvement_valide(const Jeu *jeu, Case depart, Case arrivee) {
    // Vérification des limites du plateau
    if (depart.x < 0 || depart.x >= TAILLE || depart.y < 0 || depart.y >= TAILLE ||
        arrivee.x < 0 || arrivee.x >= TAILLE || arrivee.y < 0 || arrivee.y >= TAILLE)
        return false;

    // Vérification de la présence d'un pion et de sa couleur
    Pion *pion = jeu->plateau[depart.x][depart.y];
    if (pion == NULL || pion->couleur != jeu->joueur)
        return false;

    // Vérification que la case d'arrivée est libre
    if (jeu->plateau[arrivee.x][arrivee.y] != NULL)
        return false;

    // Vérification du château
    if (est_chateau(arrivee.x, arrivee.y, pion->couleur))
        return false;

    // Vérification du mouvement en ligne droite ou diagonale
    int dx = abs(arrivee.x - depart.x);
    int dy = abs(arrivee.y - depart.y);
    if (!((dx == 0 && dy > 0) || (dy == 0 && dx > 0) || (dx == dy)))
        return false;

    // Vérification qu'aucune pièce ne bloque le chemin
    int step_x = (dx == 0) ? 0 : (arrivee.x - depart.x) / dx;
    int step_y = (dy == 0) ? 0 : (arrivee.y - depart.y) / dy;
    
    int x = depart.x + step_x;
    int y = depart.y + step_y;
    while (x != arrivee.x || y != arrivee.y) {
        if (jeu->plateau[x][y] != NULL)
            return false;
        x += step_x;
        y += step_y;
    }

    return true;
}

bool sont_cases_adjacentes(Case c1, Case c2) {
    return (abs(c1.x - c2.x) == 1 && c1.y == c2.y) ||
           (abs(c1.y - c2.y) == 1 && c1.x == c2.x);
}

bool est_chateau(int x, int y, Couleur couleur) {
    return (couleur == BLANC && x == TAILLE-1 && y == 0) ||
           (couleur == NOIR && x == 0 && y == TAILLE-1);
}

bool interroger_piece(Jeu *jeu, Case interrogateur, Case cible) {
    if (!sont_cases_adjacentes(interrogateur, cible))
        return false;

    Pion *p_interrogateur = jeu->plateau[interrogateur.x][interrogateur.y];
    Pion *p_cible = jeu->plateau[cible.x][cible.y];

    if (!p_interrogateur || !p_cible || p_interrogateur->couleur == p_cible->couleur)
        return false;

    if (p_cible->type == ESPION) {
        jeu->partie_terminee = true;
        return true;
    }

    if (p_interrogateur->type == ESPION) {
        jeu->partie_terminee = true;
        printf("Joueur %s, vous avez interrogé un chevalier avec votre espion...\n",
               jeu->joueur == BLANC ? "blanc" : "noir");
    } else {
        printf("Joueur %s, vous n'avez pas trouvé l'espion de votre adversaire...\n",
               jeu->joueur == BLANC ? "blanc" : "noir");
        printf("Et en plus, votre chevalier s'est fait empoisonné...\n");
    }

    // Supprimer la pièce interrogatrice
    jeu->plateau[interrogateur.x][interrogateur.y] = NULL;
    free(p_interrogateur);
    return false;
}

bool demander_action_ascii(Jeu *jeu) {
    char action;
    printf("Joueur %s, voulez vous faire un déplacement ou une interrogation ? ('d' ou 'i')\n",
           jeu->joueur == BLANC ? "blanc" : "noir");
    scanf(" %c", &action);

    if (action == 'd') {
        printf("Quel déplacement pour le joueur %s ?\n", jeu->joueur == BLANC ? "blanc" : "noir");
        printf("Saisie sous la forme (a, b) --> (c, d).\n");
        Case depart, arrivee;
        if (scanf(" (%d, %d) --> (%d, %d)", &depart.x, &depart.y, &arrivee.x, &arrivee.y) != 4)
            return false;

        if (!est_mouvement_valide(jeu, depart, arrivee)) {
            printf("Déplacement non licite : un joueur ne peut pas amener un pion sur son chateau.\n");
            return false;
        }

        // Effectuer le déplacement
        jeu->plateau[arrivee.x][arrivee.y] = jeu->plateau[depart.x][depart.y];
        jeu->plateau[depart.x][depart.y] = NULL;
        jeu->joueur = (jeu->joueur == BLANC) ? NOIR : BLANC;
        return true;

    } else if (action == 'i') {
        Case interrogateur, cible;
        printf("Quel pion %s est l'interrogateur ?\n", jeu->joueur == BLANC ? "blanc" : "noir");
        printf("Saisie sous la forme (a, b)\n");
        scanf(" (%d, %d)", &interrogateur.x, &interrogateur.y);

        printf("Quel pion est questionné ?\n");
        printf("Saisie sous la forme (a, b)\n");
        scanf(" (%d, %d)", &cible.x, &cible.y);

        if (interroger_piece(jeu, interrogateur, cible)) {
            printf("L'espion a été trouvé !\n");
            return true;
        }
        jeu->joueur = (jeu->joueur == BLANC) ? NOIR : BLANC;
        return true;
    }

    return false;
}

void sauvegarder_partie(const Jeu *jeu, const char *nom_fichier) {
    FILE *f = fopen(nom_fichier, "w");
    if (!f) return;

    // Sauvegarder positions des espions
    for (int i = 0; i < TAILLE; i++) {
        for (int j = 0; j < TAILLE; j++) {
            if (jeu->plateau[i][j] && jeu->plateau[i][j]->type == ESPION) {
                fprintf(f, "%c %c%d\n", 
                    jeu->plateau[i][j]->couleur == BLANC ? 'B' : 'N',
                    'a' + j, TAILLE - i);
            }
        }
    }

    fprintf(f, "%c\n", jeu->joueur == BLANC ? 'B' : 'N');

    // Sauvegarder l'historique
    for (int i = 0; i < jeu->nb_mouvements; i++) {
        Mouvement m = jeu->historique[i];
        fprintf(f, "%c %c%d->%c%d\n",
                m.est_interrogation ? 'I' : 'D',
                'a' + m.depart.y, TAILLE - m.depart.x,
                'a' + m.arrivee.y, TAILLE - m.arrivee.x);
    }

    fclose(f);
}

Jeu *charger_partie(const char *nom_fichier) {
    FILE *f = fopen(nom_fichier, "r");
    if (!f) return NULL;

    Jeu *jeu = initialiser_jeu();
    if (!jeu) {
        fclose(f);
        return NULL;
    }

    char ligne[20];
    while (fgets(ligne, sizeof(ligne), f)) {
        ligne[strcspn(ligne, "\n")] = 0;

        if (ligne[0] == 'B' || ligne[0] == 'N') {
            if (ligne[1] == ' ') {
                // Position espion
                int y = ligne[2] - 'a';
                int x = TAILLE - (ligne[3] - '0');
                Couleur couleur = (ligne[0] == 'B') ? BLANC : NOIR;

              // Mettre à jour l'espion
                for (int i = 0; i < TAILLE; i++) {
                    for (int j = 0; j < TAILLE; j++) {
                        if (jeu->plateau[i][j] && jeu->plateau[i][j]->couleur == couleur) {
                            jeu->plateau[i][j]->type = CHEVALIER;
                        }
                    }
                }
                jeu->plateau[x][y]->type = ESPION;
            } else {
                // Joueur actuel
                jeu->joueur = (ligne[0] == 'B') ? BLANC : NOIR;
            }
        } else if (ligne[0] == 'D' || ligne[0] == 'I') {
            int depart_y = ligne[2] - 'a';
            int depart_x = TAILLE - (ligne[3] - '0');
            int arrivee_y = ligne[6] - 'a';
            int arrivee_x = TAILLE - (ligne[7] - '0');

            Mouvement m;
            m.depart.x = depart_x;
            m.depart.y = depart_y;
            m.arrivee.x = arrivee_x;
            m.arrivee.y = arrivee_y;
            m.est_interrogation = (ligne[0] == 'I');

            if (m.est_interrogation) {
                interroger_piece(jeu, m.depart, m.arrivee);
            } else {
                if (est_mouvement_valide(jeu, m.depart, m.arrivee)) {
                    jeu->plateau[m.arrivee.x][m.arrivee.y] = jeu->plateau[m.depart.x][m.depart.y];
                    jeu->plateau[m.depart.x][m.depart.y] = NULL;
                }
            }
            
            jeu->historique[jeu->nb_mouvements++] = m;
        }
    }

    fclose(f);
    return jeu;
}

bool demander_action_graphique(Jeu *jeu) {
    int x1, y1, x2, y2;
    static bool premiere_case = true;
    static Case depart;

    if (premiere_case) {
        int souris_x, souris_y;
        MLV_wait_mouse(&souris_x, &souris_y);
        
        x1 = souris_y / TAILLE_CASE;
        y1 = souris_x / TAILLE_CASE;

        if (x1 >= 0 && x1 < TAILLE && y1 >= 0 && y1 < TAILLE) {
            if (jeu->plateau[x1][y1] && jeu->plateau[x1][y1]->couleur == jeu->joueur) {
                depart.x = x1;
                depart.y = y1;
                MLV_draw_circle(y1 * TAILLE_CASE + TAILLE_CASE/2, 
                              x1 * TAILLE_CASE + TAILLE_CASE/2,
                              TAILLE_CASE/3, MLV_COLOR_RED);
                MLV_actualise_window();
                premiere_case = false;
                return false;
            }
        }
    } else {
        int souris_x, souris_y;
        MLV_wait_mouse(&souris_x, &souris_y);
        
        x2 = souris_y / TAILLE_CASE;
        y2 = souris_x / TAILLE_CASE;

        if (x2 >= 0 && x2 < TAILLE && y2 >= 0 && y2 < TAILLE) {
            Case arrivee = {x2, y2};
            
            if (sont_cases_adjacentes(depart, arrivee) && 
                jeu->plateau[x2][y2] && 
                jeu->plateau[x2][y2]->couleur != jeu->joueur) {
                // Interrogation
                interroger_piece(jeu, depart, arrivee);
                premiere_case = true;
                return true;
            } else if (est_mouvement_valide(jeu, depart, arrivee)) {
                // Déplacement
                jeu->plateau[x2][y2] = jeu->plateau[depart.x][depart.y];
                jeu->plateau[depart.x][depart.y] = NULL;
                
                Mouvement m = {depart, arrivee, false};
                jeu->historique[jeu->nb_mouvements++] = m;
                
                premiere_case = true;
                jeu->joueur = (jeu->joueur == BLANC) ? NOIR : BLANC;
                return true;
            }
        }
        premiere_case = true;
    }
    return false;
}

void liberer_jeu(Jeu *jeu) {
    if (!jeu) return;
    
    for (int i = 0; i < TAILLE; i++) {
        for (int j = 0; j < TAILLE; j++) {
            free(jeu->plateau[i][j]);
        }
    }
    free(jeu);
}

int main(int argc, char *argv[]) {
    bool mode_graphique = false;
    char *fichier_sauvegarde = NULL;
    char *fichier_chargement = NULL;

    // Traiter les arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-a") == 0) {
            mode_graphique = false;
        } else if (strcmp(argv[i], "-g") == 0) {
            mode_graphique = true;
        } else if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) {
            fichier_sauvegarde = argv[++i];
        } else if (strcmp(argv[i], "-c") == 0 && i + 1 < argc) {
            fichier_chargement = argv[++i];
        }
    }

    Jeu *jeu;
    if (fichier_chargement) {
        jeu = charger_partie(fichier_chargement);
    } else {
        jeu = initialiser_jeu();
    }

    if (!jeu) {
        fprintf(stderr, "Erreur d'initialisation du jeu\n");
        return EXIT_FAILURE;
    }

    printf("Bienvenue dans Incognito\n");
    printf("Rappel : (0, 0) designe le coin supérieur gauche et (4, 4) le coin inférieur droit\n");

    if (mode_graphique) {
        MLV_create_window("Jeu Incognito", "jeu", TAILLE * TAILLE_CASE, TAILLE * TAILLE_CASE);
        while (!jeu->partie_terminee) {
            afficher_plateau_graphique(jeu);
            if (demander_action_graphique(jeu)) {
                if (fichier_sauvegarde) {
                    sauvegarder_partie(jeu, fichier_sauvegarde);
                }
            }
        }
        MLV_free_window();
    } else {
        while (!jeu->partie_terminee) {
            afficher_plateau_ascii(jeu);
            if (demander_action_ascii(jeu)) {
                if (fichier_sauvegarde) {
                    sauvegarder_partie(jeu, fichier_sauvegarde);
                }
            }
        }
    }

    liberer_jeu(jeu);
    return EXIT_SUCCESS;
}