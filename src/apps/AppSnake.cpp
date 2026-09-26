#include <Arduino.h>
#include <Preferences.h>
#include <esp_heap_caps.h>
#include "../App.h"
#include "../Lvgl.h"
#include "../Theme.h"

// ================= Plateau =================
static const int CASE      = 8;           // Taille d'une case en pixels
static const int MAX_COTE  = 40;          // Au plus 40 cases de côté (320 px)
static const int MAX_CASES = MAX_COTE * MAX_COTE;

static int       cols, rows;              // Dimensions du plateau en cases
static int       largeurPx;               // Largeur de l'image en pixels
static uint16_t *pixels = nullptr;        // Image du plateau (RGB565), en PSRAM, allouée une fois
static uint16_t  cFond, cCorps, cTete, cFruit;

// ================= Serpent =================
// File circulaire des cases occupées (une case = x + y * cols) : la tête avance, la queue suit
static uint16_t corps[MAX_CASES];
static uint16_t iTete, iQueue;
static uint8_t  occupe[MAX_CASES];        // 1 = case occupée par le serpent
static uint16_t fruit;
static uint8_t  dir;                      // 0 droite, 1 bas, 2 gauche, 3 haut
static int8_t   virages;                  // Virages demandés, pas encore appliqués

enum { PRET, JEU, PAUSE, PERDU };
static uint8_t  etat = PRET;
static uint16_t score, record;
static uint32_t periode, dernierPas;

static lv_obj_t   *canvas, *labelScore, *labelMessage;
static lv_timer_t *timerJeu = nullptr;

// ================= Record (espace NVS propre au jeu) =================
static void chargerRecord() {
  Preferences p;
  p.begin("snake", true);                 // true = lecture seule
  record = p.getUShort("record", 0);
  p.end();
}

static void sauverRecord() {
  Preferences p;
  p.begin("snake", false);
  p.putUShort("record", record);
  p.end();
}

// ================= Dessin =================
static void dessinerCase(uint16_t c, uint16_t couleur) {
  int x0 = (c % cols) * CASE;
  int y0 = (c / cols) * CASE;
  for (int y = 0; y < CASE - 1; y++) {    // CASE - 1 : 1 px d'espace entre les cases
    uint16_t *ligne = pixels + (y0 + y) * largeurPx + x0;
    for (int x = 0; x < CASE - 1; x++) ligne[x] = couleur;
  }
}

static void majScore() {
  lv_label_set_text_fmt(labelScore, "Score %u     Record %u", score, record);
}

static void afficherMessage(const char *texte) {
  if (!texte) { lv_obj_set_hidden(labelMessage, true); return; }
  lv_label_set_text(labelMessage, texte);
  lv_obj_set_hidden(labelMessage, false);
}

// ================= Logique du jeu =================
static void placerFruit() {
  if (score + 3 >= (uint16_t)(cols * rows)) return;     // Plateau plein : partie gagnée !
  do { fruit = random(cols * rows); } while (occupe[fruit]);
  dessinerCase(fruit, cFruit);
}

static void nouvellePartie() {
  memset(occupe, 0, sizeof(occupe));
  for (int i = 0; i < largeurPx * rows * CASE; i++) pixels[i] = cFond;

  // Un serpent de 3 cases au milieu, tourné vers la droite
  int cx = cols / 2, cy = rows / 2;
  iQueue = 0;
  iTete  = 2;
  for (int i = 0; i < 3; i++) {
    uint16_t c = (cx - 2 + i) + cy * cols;
    corps[i] = c;
    occupe[c] = 1;
    dessinerCase(c, i == 2 ? cTete : cCorps);
  }

  dir = 0;
  virages = 0;
  score = 0;
  periode = 160;                          // ms entre deux pas : ça accélérera
  placerFruit();
  majScore();
  lv_obj_invalidate(canvas);
}

// Avance d'une case. Renvoie false si le serpent meurt.
static bool avancer() {
  // Un seul virage par pas : impossible de faire demi-tour d'un coup
  if (virages > 0)      { dir = (dir + 1) % 4; virages--; }
  else if (virages < 0) { dir = (dir + 3) % 4; virages++; }

  uint16_t t = corps[iTete];
  int x = t % cols, y = t / cols;
  switch (dir) {
    case 0:  x++; break;
    case 1:  y++; break;
    case 2:  x--; break;
    default: y--; break;
  }
  if (x < 0 || y < 0 || x >= cols || y >= rows) return false;    // Mur !

  uint16_t n = x + y * cols;
  bool mange = (n == fruit);

  // La queue avance AVANT le test de collision : on a le droit d'entrer
  // dans la case qu'elle est en train de libérer
  if (!mange) {
    uint16_t q = corps[iQueue];
    occupe[q] = 0;
    dessinerCase(q, cFond);
    iQueue = (iQueue + 1) % MAX_CASES;
  }
  if (occupe[n]) return false;                                  // Il se mord la queue !

  dessinerCase(t, cCorps);                // L'ancienne tête devient du corps
  iTete = (iTete + 1) % MAX_CASES;
  corps[iTete] = n;
  occupe[n] = 1;
  dessinerCase(n, cTete);

  if (mange) {
    score++;
    if (periode > 70) periode -= 4;       // Chaque fruit accélère le jeu
    majScore();
    placerFruit();
  }

  lv_obj_invalidate(canvas);              // Le canvas sera renvoyé à l'écran
  return true;
}

// Boucle du jeu : lit les entrées toutes les 20 ms, avance au rythme de "periode"
static void boucle(lv_timer_t *) {
  int32_t r = lvglPopRotation();
  if (etat == JEU && r != 0) {
    virages += r;                         // Chaque cran = un quart de tour
    if (virages > 2)  virages = 2;
    if (virages < -2) virages = -2;
  }

  if (lvglPopClic()) {
    switch (etat) {
      case PRET:
      case PERDU:
        nouvellePartie();
        etat = JEU;
        afficherMessage(nullptr);
        dernierPas = millis();
        break;
      case JEU:
        etat = PAUSE;
        afficherMessage("Pause");
        break;
      case PAUSE:
        etat = JEU;
        afficherMessage(nullptr);
        dernierPas = millis();
        break;
    }
  }

  if (etat == JEU && millis() - dernierPas >= periode) {
    dernierPas = millis();
    if (!avancer()) {
      etat = PERDU;
      if (score > record) {
        record = score;
        sauverRecord();
        majScore();
        afficherMessage("Nouveau record !\nClic pour rejouer");
      } else {
        afficherMessage("Perdu !\nClic pour rejouer");
      }
    }
  }
}

// ================= API de l'app =================
void snakeCreate(lv_obj_t *contenu) {
  // Couleurs du thème actif, converties au format de l'image
  cFond  = lv_color_to_u16(lv_color_hex(COUL_CARTE));
  cCorps = lv_color_to_u16(lv_color_hex(COUL_ACCENT));
  cTete  = lv_color_to_u16(lv_color_hex(COUL_TEXTE));
  cFruit = lv_color_to_u16(lv_color_hex(COUL_ROUGE));

  chargerRecord();

  labelScore = lv_label_create(contenu);
  lv_obj_set_style_text_font(labelScore, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(labelScore, lv_color_hex(COUL_TEXTE_2), 0);

  // Taille du plateau : tout l'espace restant, en nombre entier de cases.
  // On force le calcul de la mise en page pour connaître cet espace.
  lv_obj_update_layout(contenu);
  int32_t w = lv_obj_get_content_width(contenu);
  int32_t h = lv_obj_get_content_height(contenu) - lv_obj_get_height(labelScore) - 8;
  cols = min((int)(w / CASE), MAX_COTE);
  rows = min((int)(h / CASE), MAX_COTE);
  largeurPx = cols * CASE;

  // Image du plateau en PSRAM, allouée une seule fois pour la taille max.
  // On ne la libère jamais : l'écran de l'app reste affiché pendant l'animation
  // de sortie, et lirait sinon de la mémoire déjà rendue.
  if (!pixels) {
    pixels = (uint16_t *)heap_caps_malloc(MAX_COTE * CASE * MAX_COTE * CASE * sizeof(uint16_t),
                                          MALLOC_CAP_SPIRAM);
  }

  canvas = lv_canvas_create(contenu);
  lv_canvas_set_buffer(canvas, pixels, largeurPx, rows * CASE, LV_COLOR_FORMAT_RGB565);

  // Message par-dessus le plateau
  labelMessage = lv_label_create(canvas);
  lv_obj_set_style_text_align(labelMessage, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_color(labelMessage, lv_color_hex(COUL_TEXTE), 0);
  lv_obj_set_style_bg_color(labelMessage, lv_color_hex(COUL_FOND), 0);
  lv_obj_set_style_bg_opa(labelMessage, LV_OPA_80, 0);
  lv_obj_set_style_pad_all(labelMessage, 8, 0);
  lv_obj_set_style_radius(labelMessage, 8, 0);
  lv_obj_center(labelMessage);

  lvglCaptureEncodeur(true);              // L'encodeur est à nous !
  nouvellePartie();
  etat = PRET;
  afficherMessage("Clic pour jouer");

  timerJeu = lv_timer_create(boucle, 20, nullptr);
}

void snakeExit() {
  lvglCaptureEncodeur(false);             // On rend l'encodeur à LVGL
  if (timerJeu) { lv_timer_delete(timerJeu); timerJeu = nullptr; }
  etat = PRET;
}