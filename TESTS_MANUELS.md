# WaveSketch — Batterie de tests manuels

Ces vérifications demandent une oreille humaine et Ableton Live : elles complètent les tests
automatiques (`build.bat` les lance) et pluginval. Coche chaque case une fois le test réussi,
et note ce qui ne va pas dans la colonne « Remarques » de la fin.

**Conseils**
- Écoute au casque, volume **modéré** : les clics et l'aliasing s'entendent mieux, mais un bug peut produire un son fort.
- Pour « voir » le son, ajoute l'effet **Spectrum** d'Ableton (Audio Effects → Utilities) juste après WaveSketch.
- Sauf mention contraire, réglages par défaut : Attack 5 ms, Decay 300 ms, Sustain 80 %, Release 300 ms, Volume -6 dB.

---

## 0. Préparation

- [ ] `build.bat` se termine par « Build termine » et « 100% tests passed ».
- [ ] Copier **tout le dossier** `build\WaveSketch_artefacts\Release\VST3\WaveSketch.vst3` dans `C:\Program Files\Common Files\VST3` (Windows demande une autorisation administrateur).
- [ ] Ableton : *Options → Préférences → Plug-ins* : activer **« Utiliser les dossiers système des plug-ins VST3 »**, puis cliquer sur **« Rescan »**.

## 1. Chargement dans Ableton

- [ ] WaveSketch apparaît dans le navigateur : *Plug-ins → VST3 → WaveSketch Audio → WaveSketch*.
- [ ] Glissé sur une **piste MIDI** : il se place comme un instrument (pas comme un effet audio).
- [ ] La fenêtre s'ouvre (icône clé à molette) : interface sombre, sinus affiché, aucun message d'erreur.
- [ ] Fermer puis rouvrir la fenêtre plusieurs fois : aucun plantage, le dessin est toujours là.
- [ ] Supprimer la piste, puis *Ctrl+Z* dans Ableton : le plugin revient avec son dessin.

## 2. Jeu MIDI

- [ ] Une note jouée au clavier MIDI (ou au clavier d'ordinateur d'Ableton, touche **M**) produit un sinus.
- [ ] La hauteur est juste : le La 3 (A3, note 69) doit sonner comme un diapason à 440 Hz (comparer avec un accordeur ou un autre synthé).
- [ ] **Vélocité** : jouer doucement puis fort → le volume suit clairement.
- [ ] **Accord** de 4 à 6 notes : toutes les notes sonnent, pas de saturation à Volume -6 dB.
- [ ] **Plus de 16 notes** (pédale de sustain + glissando, ou un clip MIDI avec 20 notes simultanées) : les plus anciennes s'éteignent en douceur, **sans clic** ni coupure brutale.
- [ ] **Pédale de sustain** : les notes relâchées continuent tant que la pédale est enfoncée, puis s'éteignent au relâchement.
- [ ] **Pitch bend** avec Bend = 2 : la molette monte/descend d'un ton. Avec Bend = 12 : d'une octave. Retour exact au centre.
- [ ] Clip MIDI avec des notes très rapprochées (doubles croches à 180 BPM) : chaque note démarre au bon moment, rien ne « traîne ».
- [ ] Arrêter la lecture pendant des notes tenues : aucune note ne reste bloquée.
- [ ] **Clavier virtuel** de WaveSketch (en bas de la fenêtre) : joue des notes, plus fort quand on clique bas sur la touche.

## 3. Enveloppe ADSR et volume

- [ ] **Attack** à 2 s : le son monte progressivement.
- [ ] **Decay** 1 s + **Sustain** 20 % : le son baisse puis se stabilise.
- [ ] **Release** à 3 s : le son traîne après le relâchement de la touche.
- [ ] Tourner **Sustain** pendant une note tenue : le volume suit **sans clic**.
- [ ] Tourner **Volume** rapidement pendant un accord : pas de clic ni de grésillement « en escalier ».
- [ ] Notes très courtes (Attack 1 ms, Release 5 ms) sur une forme carrée : petit « tic » d'attaque possible mais pas de craquement aléatoire.

## 4. Dessin (mode main levée)

- [ ] Dessiner lentement : la courbe suit exactement le curseur.
- [ ] Dessiner **très vite** d'un bord à l'autre : courbe continue, **aucun trou** ni pointe isolée.
- [ ] Un simple clic modifie un petit point de la courbe sans abîmer le reste.
- [ ] **Dessiner pendant qu'une note tient** (clip en boucle) : le timbre change en continu, **sans clic ni craquement**, même en griffonnant frénétiquement.
- [ ] Boutons **Sinus / Triangle / Dents de scie / Carré** : la forme est remplacée, le son change sans clic.
- [ ] **Lisser** (plusieurs clics sur un carré) : les angles s'arrondissent, le son devient plus doux.
- [ ] **Inverser** : la courbe se retourne ; sur une note seule le son ne change presque pas (c'est normal).
- [ ] **Réinitialiser** : retour au sinus.
- [ ] Dessin minuscule près de la ligne du zéro : sonne aussi fort qu'un sinus plein (normalisation).
- [ ] Dessin entièrement au-dessus du zéro : pas de « boum » au début ni à la fin des notes (retrait du DC).
- [ ] Dessin complètement plat : silence, aucun bruit étrange.

## 5. Mode points

- [ ] Bouton **Points** : la courbe actuelle est convertie en 16 points orange.
- [ ] Clic sur une zone vide : un point est ajouté, la courbe passe par lui.
- [ ] Glisser un point : la courbe suit en douceur ; un point ne peut pas dépasser ses voisins horizontalement.
- [ ] Clic droit ou double-clic sur un point : il est supprimé (il en reste toujours au moins 2).
- [ ] La courbe ne fait jamais de « bosse » au-delà des points.
- [ ] Déplacer un point pendant qu'une note tient : pas de clic.
- [ ] Revenir en **Main levée** puis repasser en **Points** sans rien modifier : les mêmes points reviennent.
- [ ] Le bouton **Lisser** est grisé en mode points (la courbe est déjà lisse).

## 6. Annuler / rétablir

- [ ] Faire 3 modifications (un trait, une forme, un point déplacé), puis **Ctrl+Z** trois fois : retour à l'état de départ, étape par étape.
- [ ] **Ctrl+Y** (ou **Ctrl+Maj+Z**) rétablit chaque étape.
- [ ] Un long trait d'un seul geste s'annule en **un seul** Ctrl+Z.
- [ ] Les boutons **Annuler / Rétablir** font la même chose et sont grisés quand il n'y a rien à faire ; leur info-bulle indique l'action.
- [ ] ⚠ Ableton peut intercepter certains raccourcis : cliquer d'abord dans la fenêtre de WaveSketch. Si Ctrl+Z annule une action d'Ableton au lieu du dessin, le noter (les boutons restent utilisables).

## 7. Presets et fichiers

- [ ] Menu **Presets** : chaque preset (14) se charge, sonne, et s'annule avec Ctrl+Z. Les presets « (points) » passent en mode points.
- [ ] **Enregistrer…** : propose `Documents\WaveSketch\waveform.wavesketch` ; le fichier est créé.
- [ ] Modifier le dessin, puis **Ouvrir…** ce fichier : le dessin enregistré revient à l'identique (y compris en mode points).
- [ ] Ouvrir un fichier qui n'est pas une waveform (renommer un `.txt` en `.wavesketch`) : message d'erreur clair, dessin inchangé, pas de plantage.
- [ ] Le fichier `.wavesketch` fonctionne aussi dans une autre instance / un autre projet.

## 8. Automatisation dans Ableton

- [ ] En vue Arrangement, sélecteur d'automatisation de la piste : **WaveSketch** propose Attack, Decay, Sustain, Release, Volume, Pitch Bend Range.
- [ ] Dessiner une rampe d'automatisation sur **Volume** : le volume suit en douceur, sans « marches ».
- [ ] Automatiser **Sustain** pendant une note tenue : pas de clic.
- [ ] Pendant la lecture, les potentiomètres de WaveSketch bougent avec l'automatisation.
- [ ] Toucher un potentiomètre en mode enregistrement d'automatisation : Ableton enregistre le mouvement.
- [ ] Le nom des paramètres et les valeurs affichées dans Ableton sont lisibles (ms, s, %, dB).

## 9. Sauvegarde, fermeture et réouverture du projet

- [ ] Préparer : dessin personnel en **mode points**, ADSR et Volume modifiés, fenêtre agrandie. Écouter une note de référence.
- [ ] Sauvegarder (*Ctrl+S*), **quitter Ableton**, relancer et rouvrir le projet.
- [ ] Le **son** est identique avant ouverture de la fenêtre du plugin (jouer la note de référence).
- [ ] À l'ouverture de la fenêtre : même dessin, même mode (points), mêmes points, mêmes potentiomètres, **même taille de fenêtre**.
- [ ] Deux pistes WaveSketch avec des dessins différents : chacune retrouve le sien.
- [ ] **Dupliquer** une piste WaveSketch (*Ctrl+D*) : la copie a le même dessin, et modifier l'une ne change pas l'autre.
- [ ] *Ctrl+Z* juste après la réouverture ne fait rien d'étrange (l'historique du dessin repart de zéro).

## 10. Qualité du son : aliasing (forme carrée dans l'aigu)

- [ ] Choisir **Carré**, ajouter **Spectrum** après WaveSketch.
- [ ] Jouer une gamme montante de **C6 à C8** : on n'entend qu'une note qui monte, **sans sifflements** qui descendent ou partent dans d'autres directions.
- [ ] Sur le Spectrum : des raies verticales régulièrement espacées qui **montent toutes** avec la note. Une raie qui descend quand la note monte = aliasing (à signaler).
- [ ] Même test avec **Dents de scie** et un dessin très « hérissé » fait à main levée.
- [ ] **Pitch bend** (Bend = 12) sur une note aiguë de dents de scie : montée continue, sans craquement au passage des octaves.
- [ ] Répéter à **44,1 kHz**, **48 kHz** et **96 kHz** (*Préférences → Audio → Fréquence d'échantillonnage*) : hauteur toujours juste, pas d'aliasing.

## 11. Robustesse et performances

- [ ] Tailles de buffer **64, 256, 512, 2048** (*Préférences → Audio → Taille du buffer*) : son identique, pas de craquements.
- [ ] **Geler** (*Freeze*) puis **aplatir** une piste WaveSketch : le rendu correspond à ce qu'on entendait.
- [ ] **Exporter** l'audio (*Fichier → Exporter audio/vidéo*) : le fichier exporté est correct.
- [ ] Accord de 16 notes avec Release 10 s : la charge CPU d'Ableton reste raisonnable (noter la valeur).
- [ ] 8 instances de WaveSketch jouant en même temps : pas de craquement.
- [ ] Changer de carte son / de pilote audio pendant que le projet est ouvert : pas de plantage.

## 12. Affichage Windows (mise à l'échelle)

Pour chaque échelle : *Paramètres Windows → Système → Écran → Échelle*, puis relancer Ableton.

| Test | 100 % | 125 % | 150 % |
|---|---|---|---|
| Textes nets, rien de coupé | [ ] | [ ] | [ ] |
| Le trait dessiné apparaît exactement sous le curseur | [ ] | [ ] | [ ] |
| Les points se saisissent facilement | [ ] | [ ] | [ ] |
| Redimensionnement fluide par le coin | [ ] | [ ] | [ ] |
| Fenêtre entièrement visible à l'ouverture | [ ] | [ ] | [ ] |

- [ ] Dans Ableton, clic droit sur la barre de titre du plugin → **« Auto-Scale Plug-In Window »** activé puis désactivé : l'affichage reste correct dans les deux cas.

## 13. Version Standalone

- [ ] `build\WaveSketch_artefacts\Release\Standalone\WaveSketch.exe` démarre, *Options* permet de choisir la sortie audio et l'entrée MIDI.
- [ ] Fermer puis relancer : le dernier dessin est retrouvé.

## 14. Distribution sur un autre PC

- [ ] Sur un PC **sans Visual Studio**, copier `WaveSketch.vst3` dans `C:\Program Files\Common Files\VST3` : le plugin se charge dans Ableton sans demander d'installer quoi que ce soit (pas d'erreur « VCRUNTIME140.dll manquant »).
- [ ] Un projet Ableton créé sur ton PC s'ouvre sur l'autre PC avec le même son et le même dessin.

## 15. Finition de l'interface et distribution

**Interface**
- [ ] Les potentiomètres répondent au glisser vertical **et** horizontal ; leur arc bleu suit la valeur.
- [ ] **Double-clic** sur un potentiomètre : retour à sa valeur par défaut (Attack 5 ms, Decay 300 ms, Sustain 80 %, Release 300 ms, Volume -6 dB, Bend ±2 st).
- [ ] **Clic sur la valeur** sous un potentiomètre, taper `1.5 s` (ou `250 ms`) puis Entrée : la valeur est appliquée.
- [ ] Les info-bulles apparaissent au survol des boutons (environ une demi-seconde).
- [ ] Le bouton de mode actif (Main levée / Points) est surligné en bleu ; « Lisser » est grisé en mode points.
- [ ] L'aide en haut à gauche de la zone de dessin change selon le mode et reste lisible même quand la courbe passe dessous.
- [ ] Taille **minimale** (réduire la fenêtre au maximum) : tous les boutons restent lisibles, rien ne se chevauche.
- [ ] Taille **maximale** / plein écran : la zone de dessin s'agrandit, le clavier virtuel occupe toute la largeur.
- [ ] Redimensionner pendant qu'une note joue : aucun craquement audio.
- [ ] Dessiner vite en grande taille de fenêtre : l'affichage reste fluide (pas de saccades visibles).
- [ ] Fermer la fenêtre en tenant une note du clavier virtuel : la note s'arrête (pas de note bloquée).

**Distribution**
- [ ] `package.bat` crée `dist\WaveSketch-0.1.0-win64.zip` contenant `WaveSketch.vst3`, `WaveSketch.exe` et `INSTALLATION.txt`.
- [ ] En suivant **uniquement** `INSTALLATION.txt` (sur un autre PC si possible), le plugin s'installe et apparaît dans Ableton.
- [ ] `validate.bat` affiche « pluginval : SUCCES (rigueur 10) ».
- [ ] Le `README.md` suffit à une autre personne pour cloner, compiler et installer le projet.

---

## Remarques

| Test (n°) | Ce qui s'est passé | Version de Live / Windows |
|---|---|---|
| | | |
