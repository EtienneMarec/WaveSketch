# WaveSketch

Synthétiseur **VST3** pour Windows (64 bits) dont le son vient d'une waveform **dessinée à la souris**.
Conçu pour Ableton Live, utilisable dans tout logiciel compatible VST3, et livré aussi en version autonome
(Standalone) pour tester sans logiciel de musique.

![Interface de WaveSketch](docs/capture.png)

## Fonctionnalités

- **Dessin d'une période** de la waveform, à main levée (sans trous même si la souris va vite)
  ou en **mode points** (points reliés par une courbe lisse, ajout / déplacement / suppression).
- Formes de base en un clic (sinus, triangle, dents de scie, carré), **lisser**, **inverser** (phase),
  **réinitialiser**, **annuler / rétablir** (Ctrl+Z / Ctrl+Y).
- 14 **presets** intégrés, **ouverture / enregistrement** de fichiers `.wavesketch`.
- Oscillateur wavetable 2048 points avec interpolation d'Hermite, **16 voix** avec vol de voix,
  vélocité, pitch bend (plage réglable), pédale de sustain.
- **Enveloppe ADSR** par voix et **volume** général, tous automatisables dans Ableton, lissés pour éviter les clics.
- **Anti-aliasing** par tables à bande limitée calculées par FFT (une par octave), **retrait du DC**
  et **normalisation** du niveau, sans modifier le dessin affiché.
- Modifier la waveform pendant qu'une note joue ne provoque **ni clic ni plantage** (fondu de 20 ms).
- Tout est **enregistré dans le projet** Ableton (waveform, points, réglages, taille de la fenêtre).
- Interface sombre, **redimensionnable**, entièrement vectorielle (nette à 125 % et 150 %).

---

## 1. Prérequis

| Outil | Version | Installation (PowerShell) |
|---|---|---|
| Windows | 10 ou 11, 64 bits | — |
| Git | toute version récente | `winget install --id Git.Git -e` |
| CMake | 3.22 ou plus | `winget install --id Kitware.CMake -e` |
| Visual Studio 2022 **ou** Build Tools 2022, avec « Développement Desktop en C++ » | 17.x | voir ci-dessous |

Build Tools 2022 (compilateur seul, sans l'éditeur Visual Studio) :

```powershell
winget install --id Microsoft.VisualStudio.2022.BuildTools -e --override "--passive --wait --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"
```

> Laisse la fenêtre de l'installateur ouverte jusqu'à la fin et accepte la demande d'autorisation Windows.
> JUCE n'a pas besoin d'être installé : il est téléchargé automatiquement à la première compilation
> (connexion Internet nécessaire une seule fois).

## 2. Compiler

```bat
git clone https://github.com/EtienneMarec/WaveSketch.git
cd WaveSketch
build.bat
```

`build.bat` configure le projet, compile en **Release** et lance les tests automatiques.
Il **ne copie rien** dans `C:\Program Files` : pas besoin de le lancer en administrateur.

Fichiers produits :

| Fichier | Chemin |
|---|---|
| Plugin VST3 | `build\WaveSketch_artefacts\Release\VST3\WaveSketch.vst3` |
| Version autonome | `build\WaveSketch_artefacts\Release\Standalone\WaveSketch.exe` |
| Tests | `build\WaveSketchTests_artefacts\Release\WaveSketchTests.exe` |

<details>
<summary>Commandes équivalentes, sans le script</summary>

```bat
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```
</details>

> ⚠ Si la compilation échoue avec « LNK1104 : impossible d'ouvrir le fichier WaveSketch.vst3 »,
> c'est qu'un logiciel (Ableton, pluginval) utilise le plugin : ferme-le puis recompile.

## 3. Installer le plugin

Un plugin VST3 est un **dossier** nommé `WaveSketch.vst3`. Copie-le **en entier** dans le dossier
système des plugins VST3 :

```
C:\Program Files\Common Files\VST3
```

Avec l'Explorateur : copier-coller le dossier, puis accepter la demande d'autorisation administrateur.
Ou dans un PowerShell **lancé en administrateur** :

```powershell
Copy-Item -Recurse -Force "build\WaveSketch_artefacts\Release\VST3\WaveSketch.vst3" "C:\Program Files\Common Files\VST3\"
```

> Pense à fermer Ableton avant de remplacer une ancienne version du plugin.

## 4. Faire apparaître WaveSketch dans Ableton Live

1. *Options → Préférences → Plug-ins*.
2. Activer **« Utiliser les dossiers système des plug-ins VST3 »** (*Use VST3 Plug-in System Folders*).
3. Cliquer sur **« Rescan »** (relance l'analyse des plugins).
4. Dans le navigateur : *Plug-ins → VST3 → WaveSketch Audio → WaveSketch*.
5. Glisser WaveSketch sur une **piste MIDI** et jouer des notes (clavier MIDI, ou touche **M** d'Ableton
   pour utiliser le clavier de l'ordinateur).

Le plugin n'apparaît pas ? Vérifie que le dossier copié s'appelle bien `WaveSketch.vst3` et contient
`Contents\x86_64-win\WaveSketch.vst3`, puis relance un *Rescan* (maintenir *Alt* en cliquant force une
analyse complète).

## 5. Utilisation rapide

| Action | Comment |
|---|---|
| Dessiner | Mode **Main levée**, cliquer-glisser dans la zone centrale |
| Mode points | Bouton **Points** · clic = ajouter · glisser = déplacer · clic droit ou double-clic = supprimer |
| Annuler / rétablir | **Ctrl+Z** / **Ctrl+Y** (ou Ctrl+Maj+Z), ou les boutons en haut à droite |
| Remettre un potentiomètre à sa valeur par défaut | Double-clic sur le potentiomètre |
| Saisir une valeur | Clic sur la valeur affichée sous le potentiomètre, puis taper (`250 ms`, `1.5 s`, `-12`) et Entrée |
| Waveforms enregistrées | `Documents\WaveSketch\*.wavesketch` par défaut |

## 6. Vérifier

- **Tests automatiques** : lancés par `build.bat` (wavetable, interpolation, mipmaps et aliasing mesuré,
  enveloppe, voix, dessin, mode points, sauvegarde / restauration de l'état, annulation).
- **pluginval** (validation de plugin, niveau de rigueur maximal 10) : télécharger `pluginval_Windows.zip`
  depuis les [releases de Tracktion/pluginval](https://github.com/Tracktion/pluginval/releases),
  extraire `pluginval.exe` dans `tools\pluginval\`, puis lancer `validate.bat`.
- **Captures de l'interface** (100 %, 125 %, 150 %) sans DAW :
  `build\WaveSketchTests_artefacts\Release\WaveSketchTests.exe --snapshots captures`
- **Tests manuels** (écoute, Ableton, affichage, distribution) : voir [TESTS_MANUELS.md](TESTS_MANUELS.md).

## 7. Distribuer à une autre personne

```bat
package.bat
```

Crée `dist\WaveSketch-<version>-win64.zip` contenant le plugin, la version autonome et une notice
d'installation ([packaging/INSTALLATION.txt](packaging/INSTALLATION.txt)).
Le runtime C++ de Microsoft est **lié en statique** : la personne n'a rien d'autre à installer.
Le numéro de version se règle dans `CMakeLists.txt` (`project(WaveSketch VERSION ...)`).

> Avant de distribuer, vérifie les conditions de licence de JUCE (voir *Licence* ci-dessous).

## 8. Organisation du code

```
Source/
  PluginProcessor.*   cœur du plugin : processBlock, paramètres, sauvegarde de l'état, annulation
  PluginEditor.*      fenêtre principale (boutons, potentiomètres, clavier virtuel)
  Parameters.*        paramètres automatisables (identifiants à ne jamais renommer)
  dsp/                moteur audio (aucune allocation ni verrou dans le thread audio)
    Wavetable.*         tables à bande limitée par FFT, DC, normalisation, interpolation
    Voice.*, Envelope.h une voix : oscillateur + ADSR + vélocité
    SynthEngine.*       16 voix, MIDI, vol de voix, fondu entre waveforms
    TablePublisher.h    passage des tables vers l'audio par échange atomique, sans libération côté audio
    WavetableBuilder.h  thread d'arrière-plan qui calcule les tables
    WaveformOps.*, WaveShapes.*  trait, lissage, inversion, formes de base
  model/              état de la waveform (mode, courbe, points), sérialisation, presets
  ui/                 zone de dessin et thème graphique
Tests/                tests automatiques (+ outil de captures d'écran)
packaging/            fichiers ajoutés à l'archive de distribution
```

**Circulation d'une modification :** l'interface modifie la waveform → un thread d'arrière-plan calcule
les tables (FFT) → elles sont publiées par un échange atomique → le thread audio les récupère au bloc
suivant et fait un fondu de 20 ms depuis l'ancienne. Les anciennes tables sont rendues à l'interface,
qui les libère : le thread audio ne libère jamais de mémoire.

## Licence

Aucune licence n'a encore été choisie pour ce projet : en l'absence de fichier de licence, le code est
visible mais tous les droits restent réservés à son auteur.

WaveSketch utilise **JUCE**, distribué sous double licence : **AGPLv3** ou **licence commerciale JUCE**
(voir [juce.com/legal/juce-9-licence](https://juce.com/legal/juce-9-licence/) et le
[LICENSE.md de JUCE](https://github.com/juce-framework/JUCE/blob/master/LICENSE.md)).
Le choix de la licence de WaveSketch doit en tenir compte avant toute distribution.
