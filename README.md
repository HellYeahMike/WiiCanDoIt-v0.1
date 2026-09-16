# WiiCanDoIt

> **An achievement system made for the Nintendo Wii.**

WiiCanDoIt is a homebrew achievement system for the Nintendo Wii, inspired by the achievement systems found on consoles such as the PlayStation 3 and Xbox 360.

The goal is simple:

**If the PS3 and Xbox 360 had achievements, why wouldn't the Wii deserve them as well? **

---

##  What is WiiCanDoIt?

WiiCanDoIt is designed to give Wii games their own achievement system without requiring the original games to have built-in achievements.

The project aims to provide:

*  Game-specific achievements
*  A personal achievement profile
*  A game library
*  Persistent achievement progress
*  Automatic game recognition
*  Offline functionality
*  Future online functionality
*  A custom WiiCanDoIt interface
*  Eventually, an in-game achievement menu accessible through the Wii HOME button

The project is being developed as Wii homebrew using **devkitPro**, **devkitPPC**, and **libogc**.

---

##  Current Development Status

WiiCanDoIt is currently in **early development**.

### Working

* [x] Homebrew Channel application
* [x] Wii Remote input
* [x] Basic Wii video/console initialization
* [x] Main menu
* [x] Games section
* [x] Profile section
* [x] Settings section
* [x] Persistent SD-card save data
* [x] Game library persistence
* [x] NSMBW title detection
* [x] Support for multiple NSMBW regions
* [x] NSMBW save-file existence detection
* [x] Permanent achievement unlocking
* [x] Achievement list
* [x] Achievement details
* [x] Achievement counter

### Planned

* [ ] More Wii games
* [ ] Automatic in-game achievement notifications
* [ ] HOME-button achievement overlay
* [ ] Better graphical interface
* [ ] User profiles
* [ ] Settings customization
* [ ] Online accounts
* [ ] Online achievement synchronization
* [ ] Leaderboards
* [ ] Community features

---

#  First Supported Game

The first game being implemented is:

## New Super Mario Bros. Wii

WiiCanDoIt currently recognizes the following official Wii title IDs:

| Region      | Title ID |
| ----------- | -------- |
|  USA    | `SMNE01` |
|  Europe | `SMNP01` |
|  Japan  | `SMNJ01` |
|  Taiwan | `SMNW01` |
|  Korea  | `SMNK01` |

This means the achievement system is intended to work regardless of which official regional release of New Super Mario Bros. Wii is being used.
---

#  Save System

WiiCanDoIt maintains its own save data on the SD card.

The current structure is:

```text
SD:/
└── WiiCanDoIt/
    └── save.dat
```

The WiiCanDoIt save stores information such as:

* WiiCanDoIt save version
* Recognized games
* Detected NSMBW title ID
* Achievement states

Achievement progress is **permanent**.

For example, if an achievement is unlocked because a save file exists, deleting that game's save file does not remove the WiiCanDoIt achievement.

---

#  Game Recognition

WiiCanDoIt uses the Wii's ES title database to recognize games.

For NSMBW, it checks the Wii's registered title IDs rather than assuming that the game exists simply because WiiCanDoIt supports it.

This allows the game library to represent games that have actually been recognized by the console.

Game recognition and save-file detection are intentionally separate systems.

```text
Wii title database
        │
        ▼
  Game recognized?
        │
        ├── YES ──► Add to WiiCanDoIt library
        │
        └── NO  ──► Do not add game
```

The existence of an NSMBW save file is then checked separately for achievement purposes.

---

#  Building

WiiCanDoIt is built using:

* **devkitPro**
* **devkitPPC**
* **libogc**
* **GCC**
* **Make**

The development environment currently targets Windows with the project located at:

```text
C:\Users\HYMike\Documents\Wii\WiiCanDoIt
```

The resulting application is a standard Wii `.dol`.

---

#  Homebrew Channel Structure

The application can be installed on an SD card as:

```text
SD:/
└── apps/
    └── WiiCanDoIt/
        ├── boot.dol
        ├── icon.png
        └── meta.xml
```

It can then be launched through the Homebrew Channel.

---

#  Philosophy

WiiCanDoIt is built around a simple idea:

> **The Wii deserves its own achievement system.**

The project is intended to preserve the feeling of playing Wii games while adding a modern achievement layer around them.

Rather than replacing the games themselves, WiiCanDoIt acts as a separate system that tracks what the player accomplishes.

Simple idea.

Simple interface.

Real achievements.

**Made with Love. **

---

#  License

WiiCanDoIt is currently a personal development project.

Copyright © 2026 HellYeahMike.

See the repository for the current licensing status and project terms.

---

#  Project Status

**Version:** `0.1`
**Platform:** Nintendo Wii / Wii U vWii
**Status:** 🚧 In Development
**First Game:** New Super Mario Bros. Wii

**WiiCanDoIt is only getting started.**
