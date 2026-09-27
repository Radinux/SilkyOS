#ifndef CONFIG_H
#define CONFIG_H

#define PIN_POWER_ON         15
#define PIN_LCD_BL           38
#define ENCODER_PIN_A         2
#define ENCODER_PIN_B         1
#define ENCODER_PUSH_BUTTON  21
// ---------- Radio SI4732 (broches du firmware ATS-Mini d'origine, cf. Common.h) ----------
#define SI_RESET_PIN    16    // Reset du SI4732
#define SI_I2C_SCL      17    // Horloge I2C
#define SI_I2C_SDA      18    // Données I2C
#define AUDIO_MUTE_PIN   3    // Mute matériel, piloté par la bibliothèque (1 = son coupé)
#define AMP_EN_PIN      10    // Ampli audio (1 = actif)
// Orientation "naturelle" du PCB pour LovyanGFX (0-3).
// C'est le 0° vu par l'utilisateur.
#define ROTATION_BASE  2
#define SILKY_VERSION  "0.3"  // Version affichée dans le menu "A propos"
#define SILKY_BUILD  3      // Entier qui augmente à chaque version publiée

#endif