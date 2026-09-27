#ifndef UI_INTERNE_H
#define UI_INTERNE_H

#include <lvgl.h>

// Réservé aux modules de l'interface (Ui.cpp + dossier ui/).
// Les apps, elles, n'utilisent que Ui.h.

// ---------- Cœur (Ui.cpp) ----------
lv_obj_t *uiCreerEcran(const char *titre, lv_obj_t **ecranOut);   // titre = nullptr → logo SilkyOS
void      uiChargerEcran(lv_obj_t *ecran, lv_obj_t *contenu, lv_screen_load_anim_t anim);
void      uiOuvrirApp(int8_t index, lv_screen_load_anim_t anim);
void      uiAfficherMenu(lv_screen_load_anim_t anim);
lv_screen_load_anim_t uiAnimEntree();
lv_screen_load_anim_t uiAnimSortie();
lv_obj_t *uiCentrer(lv_obj_t *obj);     // Style "colonne, tout centré" sur un objet existant
lv_obj_t *uiCreerVoile();               // Voile plein écran sur le calque supérieur
lv_obj_t *uiCreerLogo(lv_obj_t *parent, const lv_font_t *police);   // "Silky" + "OS" en accent

// ---------- Launcher ----------
void launcherAfficher(int8_t selection, lv_screen_load_anim_t anim);

// ---------- Barres (état + appui) ----------
void barresCreer();
void barresMontrer();
void barresMaj();

// ---------- Veille ----------
void veilleMaj(bool autorisee);
void veilleReveiller();

// ---------- Superpositions (mise à jour + alertes) ----------
void superpositionsCreer();
void superpositionsMaj();
bool superpositionBloque();    // Mise à jour ou alerte à l'écran : navigation suspendue
void superpositionsNouvelEcran();   // À appeler quand un nouvel écran vide le groupe de focus

// ---------- Démarrage ----------
void demarrageAfficher();

#endif