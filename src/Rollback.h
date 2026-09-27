#ifndef ROLLBACK_H
#define ROLLBACK_H

// Filet de sécurité des mises à jour : un nouveau firmware qui plante au démarrage
// est automatiquement remplacé par l'ancien (l'autre emplacement OTA).

void rollbackVerifier();    // Tout au début de setup() : compte les démarrages d'essai
void rollbackArmer();       // Juste après une mise à jour réussie, avant le redémarrage
void rollbackTick();        // Dans loop() : valide le firmware après 30 s sans problème
bool rollbackEffectue();    // Vrai si on vient de revenir à l'ancien firmware (pour l'annoncer)

#endif