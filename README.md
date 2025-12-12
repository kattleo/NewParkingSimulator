# ParkingSimulator

_Team: Noah Wagner, Leo Kattenbeck_

A terminal-based parking lot simulation game written in C. Vehicles spawn, navigate, park, and exit in a ASCII-rendered map.


## Requirements
- Linux (tested)
- GCC (C99)
- SoX (Sound eXchange) with MP3 support: `sudo apt-get install sox libsox-fmt-mp3`

## Setup
1. **Build the project:**
   ```bash
   make
   ```
2. **Run the simulator:**
   ```bash
   ./main
   ```
   The game expects assets in the `assets/` directory (map, car sprites, sounds).

## Controls
- **Menu:** Use Up/Down arrows to select game mode, Enter to start.
- **Simulation:** The simulation runs automatically. Watch vehicles park, pay, and exit.
- **Sound:** Sound effects play automatically.

## Configuration
Edit `assets/config.txt` to adjust game experience.