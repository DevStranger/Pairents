<h1 style="font-family: Arial, sans-serif; display: flex; align-items: center; gap: 10px;">
  <img src="assets/logo.png" alt="logo" width="40" style="vertical-align: middle;" />
  Pairents - A Cooperative Networked Game
</h1>

<p>
  Pairents is a lightweight, networked, 1v1 cooperative game with elements of a social application, inspired by Tamagotchi-style virtual pets. The game encourages collaboration between two players without direct communication, focusing on taking care of a virtual pet. Players are matched randomly, and through real-time interactions, they guide the creature through different stages of growth. 
</p>

## Project Overview

### Goal
The goal of the project is to create a simple yet engaging game where two players take care of a virtual pet by collaborating. It incorporates elements of logic, cooperation, and trust, with the ultimate objective of achieving synchronized teamwork between the players.

### Features
- **Real-time collaboration** between two players
- **Matchmaking system** for pairing compatible players
- **Retro pixel art style** with minimalist GUI elements
- **Mini-games** that require cooperation, rewarding players with benefits in the main game
- **Synchronization** of pet state (e.g., hunger, happiness, sleep, etc.) between clients
- **Communication** using TCP sockets for real-time data exchange
- **Client-server architecture** for managing sessions and interactions

### Key Functionalities
- Cooperative care of a virtual pet
- Collaborative gameplay without direct communication
- Real-time synchronization of actions and pet status
- Server-side logic for matchmaking and room management
- Minimalistic graphical user interface (GUI) using SDL2
- Multiplayer support with a server-client model

---

## Project Setup

### Prerequisites
- **C compiler** (e.g., GCC)
- **SDL2** library (for graphical user interface)
- **POSIX sockets** for TCP communication
- **Make** tool (to build the project)
- **Git** for version control

### Installation

1. Clone the repository:
   ```bash
   git clone https://github.com/DevStranger/Pairents.git
   cd Pairents/game

2. Install necessary dependencies (e.g., SDL2):
   For Ubuntu:
   ```bash
   sudo apt install git gcc make
   sudo apt-get install libsdl2-dev libsdl2-ttf-dev

3. Build the project using make:
   ```bash
   make

## Running the Game

### Step 1: Start the Server

1. Open the first terminal window or tab.
2. Run the server with the following command:
    ```bash
    ./server
    ```

### Step 2: Start the Client

1. Open the second terminal window or tab.
2. Run the client with the following command:
    ```bash
    ./client <server_IP_address> <port>
    ```
   The client will attempt to connect to the server and join a pair.

### Step 3: Play the Game

Players will now collaborate to take care of the virtual pet!

## Demo

A demo video showcasing the game is available on my Google Drive: [Watch the demo](https://drive.google.com/file/d/1T58LjBJ3A5LPXXVrDNoZDngJuQAxStRn/view?usp=sharing)

## Disclaimer
Comments in the code and server logs are written in Polish because this was a project for a course at a Polish uni, I am Polish — and also simply because I can, lol.

---

| <img src="assets/logo.png" alt="logo" width="40"/> | **Pairents** | **Maja Chlipała** | **VI.2025** |
|:--:|:--:|:--:|:--:|
