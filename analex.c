/*
 * TP4 - Implementation d'automates
 * Analyseur lexical pour : si, alors, sinon, oprel, id, nb
 *
 * Flux suivi (cours, slides 64-106) :
 *   texte -> ER -> AFN (Thompson) -> AFD (determinisation) -> AFD minimal -> programme
 * Ce programme simule les 6 AFD minimaux (numerotation continue des etats) :
 *   1 etat de l'AFD minimal = 1 case du switch,
 *   1 transition = 1 if,
 *   transition absente = fin du lexeme (etat final) ou Echec() (etat non final).
 * Echec() passe a l'automate suivant : si -> sinon -> alors -> oprel -> id -> nb -> erreur.
 *
 * Compilation : gcc -Wall -o analex analex.c
 * Execution   : ./analex test.txt      (ou ./analex  -> lit le clavier)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define TAILLE_TAMPON 10000
#define MAX_LEXEME    100
#define MAX_ID        100

/* ---------- Unites lexicales ---------- */
typedef enum { SI, ALORS, SINON, OPREL, ID, NB, FIN, ERREUR } TypeUnite;
typedef enum { LT, LE, EQ, NE, GT, GE } TypeOprel;

const char *nomUnite[] = { "SI", "ALORS", "SINON", "OPREL", "ID", "NB", "FIN", "ERREUR" };
const char *nomOprel[] = { "LT", "LE", "EQ", "NE", "GT", "GE" };

typedef struct {
    TypeUnite type;
    int attribut;               /* OPREL : LT..GE ; ID : indice dans la table */
    char lexeme[MAX_LEXEME];
} Unite;

/* ---------- Tampon d'entree ---------- */
char tampon[TAILLE_TAMPON];
int debut = 0;                  /* debut du lexeme courant */
int avant = 0;                  /* pointeur avant (prochain caractere a lire) */

int etat = 0;                   /* etat courant */
int start = 0;                  /* etat initial de l'automate en cours d'essai */
int fin_nb = 0;                 /* derniere position ou le nb etait complet */

/* ---------- Table des identificateurs ---------- */
char table_id[MAX_ID][MAX_LEXEME];
int nb_id = 0;

/* ---------- Fonctions utilitaires ---------- */
char carSuivant(void) { return tampon[avant++]; }
void reculer(int n)   { avant -= n; }

int estLettre(char c)  { return isalpha((unsigned char)c); }
int estChiffre(char c) { return isdigit((unsigned char)c); }
int estLettreOuChiffre(char c) { return estLettre(c) || estChiffre(c); }
int estBlanc(char c)   { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; }

/* Copie le lexeme [debut, avant[ et prepare le lexeme suivant */
Unite fabriquer(TypeUnite type, int attribut)
{
    Unite u;
    int n = avant - debut;
    if (n >= MAX_LEXEME) n = MAX_LEXEME - 1;
    strncpy(u.lexeme, tampon + debut, n);
    u.lexeme[n] = '\0';
    u.type = type;
    u.attribut = attribut;
    debut = avant;
    return u;
}

/* Ajoute l'id dans la table s'il n'y est pas, et renvoie son indice */
int installer_id(const char *lex)
{
    for (int i = 0; i < nb_id; i++)
        if (strcmp(table_id[i], lex) == 0) return i;
    if (nb_id < MAX_ID) strcpy(table_id[nb_id++], lex);
    return nb_id - 1;
}

Unite fabriquerId(void)
{
    Unite u = fabriquer(ID, 0);
    u.attribut = installer_id(u.lexeme);
    return u;
}

/* L'AFD minimal d'oprel a fusionne <=, <>, =, >= dans un seul etat final :
   l'attribut se lit donc dans le lexeme [debut, avant[ */
int attributOprel(void)
{
    char a = tampon[debut];
    char b = (avant - debut == 2) ? tampon[debut + 1] : '\0';
    if (a == '<' && b == '=') return LE;
    if (a == '<' && b == '>') return NE;
    if (a == '<')             return LT;
    if (a == '>' && b == '=') return GE;
    if (a == '>')             return GT;
    return EQ;
}

/*
 * Echec : l'automate courant ne reconnait pas le lexeme.
 * On remet le pointeur avant au debut du lexeme,
 * puis on passe a l'etat initial de l'automate suivant.
 *   si (0) -> sinon (3) -> alors (9) -> oprel (15) -> id (19) -> nb (21) -> erreur (-1)
 */
int Echec(void)
{
    avant = debut;                 /* on relit le lexeme depuis le debut */
    switch (start) {
        case 0: start = 3; break;     /* si -> sinon */
        case 3: start = 9; break;     /* sinon -> alors */
        case 9: start = 15; break;    /* alors -> oprel */
        case 15: start = 19; break;   /* oprel -> id */
        case 19: start = 21; break;   /* id -> nb */
        case 21: start = -1; break;   /* nb -> erreur lexicale */
        default: start = -1; break;
    }
    return start;
}

/* ---------- Analyseur : renvoie la prochaine unite lexicale ---------- */
Unite uniteSuivante(void)
{
    char c;
    etat = 0;
    start = 0;

    while (1) {
        switch (etat) {

        /* ===== AFD minimal : si (etats 0-2) ===== */
        case 0:   /* si : etat A */
            c = carSuivant();
            if (estBlanc(c)) { debut = avant; break; }          /* on saute les blancs */
            if (c == '\0') { reculer(1); return fabriquer(FIN, 0); }
            if (c == 's') etat = 1;
            else etat = Echec();
            break;
        case 1:   /* si : etat B */
            c = carSuivant();
            if (c == 'i') etat = 2;
            else etat = Echec();
            break;
        case 2:   /* si : etat C (final) */
            c = carSuivant();
            if (estLettreOuChiffre(c)) etat = Echec();   /* ex : sinus -> pas un mot-cle */
            else { reculer(1); return fabriquer(SI, 0); }
            break;

        /* ===== AFD minimal : sinon (etats 3-8) ===== */
        case 3:   /* sinon : etat A */
            c = carSuivant();
            if (c == 's') etat = 4;
            else etat = Echec();
            break;
        case 4:   /* sinon : etat B */
            c = carSuivant();
            if (c == 'i') etat = 5;
            else etat = Echec();
            break;
        case 5:   /* sinon : etat C */
            c = carSuivant();
            if (c == 'n') etat = 6;
            else etat = Echec();
            break;
        case 6:   /* sinon : etat D */
            c = carSuivant();
            if (c == 'o') etat = 7;
            else etat = Echec();
            break;
        case 7:   /* sinon : etat E */
            c = carSuivant();
            if (c == 'n') etat = 8;
            else etat = Echec();
            break;
        case 8:   /* sinon : etat F (final) */
            c = carSuivant();
            if (estLettreOuChiffre(c)) etat = Echec();   /* ex : sinus -> pas un mot-cle */
            else { reculer(1); return fabriquer(SINON, 0); }
            break;

        /* ===== AFD minimal : alors (etats 9-14) ===== */
        case 9:   /* alors : etat A */
            c = carSuivant();
            if (c == 'a') etat = 10;
            else etat = Echec();
            break;
        case 10:   /* alors : etat B */
            c = carSuivant();
            if (c == 'l') etat = 11;
            else etat = Echec();
            break;
        case 11:   /* alors : etat C */
            c = carSuivant();
            if (c == 'o') etat = 12;
            else etat = Echec();
            break;
        case 12:   /* alors : etat D */
            c = carSuivant();
            if (c == 'r') etat = 13;
            else etat = Echec();
            break;
        case 13:   /* alors : etat E */
            c = carSuivant();
            if (c == 's') etat = 14;
            else etat = Echec();
            break;
        case 14:   /* alors : etat F (final) */
            c = carSuivant();
            if (estLettreOuChiffre(c)) etat = Echec();   /* ex : sinus -> pas un mot-cle */
            else { reculer(1); return fabriquer(ALORS, 0); }
            break;

        /* ===== AFD minimal : oprel (etats 15-18) ===== */
        case 15:   /* oprel : etat A */
            c = carSuivant();
            if (c == '<') etat = 16;
            else if (c == '=') etat = 17;
            else if (c == '>') etat = 18;
            else etat = Echec();
            break;
        case 16:   /* oprel : etat B (final) */
            c = carSuivant();
            if (c == '=' || c == '>') etat = 17;
            else { reculer(1); return fabriquer(OPREL, attributOprel()); }
            break;
        case 17:   /* oprel : etat C/E/F/G (final) */
            return fabriquer(OPREL, attributOprel());
        case 18:   /* oprel : etat D (final) */
            c = carSuivant();
            if (c == '=') etat = 17;
            else { reculer(1); return fabriquer(OPREL, attributOprel()); }
            break;

        /* ===== AFD minimal : id (etats 19-20) ===== */
        case 19:   /* id : etat A */
            c = carSuivant();
            if (estLettre(c)) etat = 20;
            else etat = Echec();
            break;
        case 20:   /* id : etat B/C/D (final) */
            c = carSuivant();
            if (estLettre(c) || estChiffre(c)) etat = 20;
            else { reculer(1); return fabriquerId(); }
            break;

        /* ===== AFD minimal : nb (etats 21-27) ===== */
        case 21:   /* nb : etat A */
            c = carSuivant();
            if (estChiffre(c)) etat = 22;
            else etat = Echec();
            break;
        case 22:   /* nb : etat B/C (final) */
            fin_nb = avant;
            c = carSuivant();
            if (estChiffre(c)) etat = 22;
            else if (c == '.') etat = 23;
            else if (c == 'E') etat = 24;
            else { avant = fin_nb; return fabriquer(NB, 0); }
            break;
        case 23:   /* nb : etat D */
            c = carSuivant();
            if (estChiffre(c)) etat = 25;
            else { avant = fin_nb; return fabriquer(NB, 0); }
            break;
        case 24:   /* nb : etat E */
            c = carSuivant();
            if (estChiffre(c)) etat = 26;
            else if (c == '+' || c == '-') etat = 27;
            else { avant = fin_nb; return fabriquer(NB, 0); }
            break;
        case 25:   /* nb : etat F/J (final) */
            fin_nb = avant;
            c = carSuivant();
            if (estChiffre(c)) etat = 25;
            else if (c == 'E') etat = 24;
            else { avant = fin_nb; return fabriquer(NB, 0); }
            break;
        case 26:   /* nb : etat G/K (final) */
            fin_nb = avant;
            c = carSuivant();
            if (estChiffre(c)) etat = 26;
            else { avant = fin_nb; return fabriquer(NB, 0); }
            break;
        case 27:   /* nb : etat H/I */
            c = carSuivant();
            if (estChiffre(c)) etat = 26;
            else { avant = fin_nb; return fabriquer(NB, 0); }
            break;

        /* ===== Aucun automate : erreur lexicale (mode panique) ===== */
        case -1:
            avant = debut + 1;                     /* on ignore 1 caractere */
            return fabriquer(ERREUR, 0);
        }
    }
}

/* ---------- Programme principal ---------- */
int main(int argc, char *argv[])
{
    FILE *f = stdin;
    if (argc > 1) {
        f = fopen(argv[1], "r");
        if (f == NULL) { printf("Fichier introuvable : %s\n", argv[1]); return 1; }
    }
    size_t n = fread(tampon, 1, TAILLE_TAMPON - 1, f);
    tampon[n] = '\0';
    if (f != stdin) fclose(f);

    Unite u;
    do {
        u = uniteSuivante();
        if (u.type == OPREL)
            printf("<%s, %s>\t\"%s\"\n", nomUnite[u.type], nomOprel[u.attribut], u.lexeme);
        else if (u.type == ID)
            printf("<%s, %d>\t\"%s\"\n", nomUnite[u.type], u.attribut, u.lexeme);
        else if (u.type == ERREUR)
            printf("<%s>\t\"%s\" : caractere non reconnu\n", nomUnite[u.type], u.lexeme);
        else if (u.type == FIN)
            printf("<%s>\n", nomUnite[u.type]);
        else
            printf("<%s>\t\"%s\"\n", nomUnite[u.type], u.lexeme);
    } while (u.type != FIN);

    printf("\nTable des identificateurs :\n");
    for (int i = 0; i < nb_id; i++)
        printf("  %d : %s\n", i, table_id[i]);
    return 0;
}
