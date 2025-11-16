#include <iostream>
#include <thread>
#include <chrono>
#include <termios.h>
#include <unistd.h>
#include <sys/select.h>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <vector>
#include <ncurses.h>

#include "Logger.h"
#include "Map.h"
#include "Action.h"
#include "ThreadSafeQueue.h"
#include "Admin.h"
#include "GameAI.h"
#include "Renderer.h"
#include "Human.h"
#include "GameCtrl.h"

/*static termios old_t;
void reset_terminal() {
    tcsetattr(STDIN_FILENO, TCSANOW, &old_t);
}*/

// Load users from file
std::vector<PlayerInfo> loadUsers(const std::string& filename) {
    std::vector<PlayerInfo> users;
    std::ifstream file(filename);
    if (!file.is_open()) return users;
    std::string line;
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        PlayerInfo p;
        std::string token;
        if (std::getline(ss, token, ',')) p.id = std::stoi(token);
        if (std::getline(ss, token, ',')) p.name = token;
        if (std::getline(ss, token, ',')) p.color = std::stoi(token);
        if (std::getline(ss, token, ',')) p.level = std::stoi(token);
        users.push_back(p);
    }
    file.close();
    return users;
}

// Save users to file
void saveUsers(const std::string& filename, const std::vector<PlayerInfo>& users) {
    std::ofstream file(filename);
    if (!file.is_open()) return;
    for (const auto& p : users) {
        file << p.id << "," << p.name << "," << p.color << "," << p.level << "\n";
    }
    file.close();
}

int main() {
    bool b_start_game = false;
    bool b_exit_game = false;

    // cmd line UI
    std::cout << "Welcome to Generals AI! " << std::endl;
    std::cout << "Please input your name:" << std::endl;

    // get or create user
    PlayerInfo player;
    std::getline(std::cin, player.name);
    std::vector<PlayerInfo> users = loadUsers("user-data.txt"); // Load existing users
    bool found = false;
    int maxId = 0;
    for (const auto& u : users) { // Check if user exists
        if (u.name == player.name) {
            player = u;  // Copy all info
            found = true;
            break;
        }
        if (u.id > maxId) maxId = u.id;
    }
    if (!found) {
        // Create new user
        player.id = maxId + 1;
        player.color = 0;  // Default color
        player.level = 1;  // Default level
        users.push_back(player);
        saveUsers("user-data.txt", users);
        std::cout << "New user created and saved." << std::endl;
    } else {
        std::cout << "Welcome back, " << player.name << "!" << std::endl;
    }

    // gen map params based on level
    //int map_width = 10 + player.level * 3;
    //int map_height = 5 + player.level * 3;
    //int map_seed = static_cast<int>(time(nullptr)) % 10000;

    // start all sub-thread && Human
    Renderer renderer;
    renderer.start();
    Admin admin(renderer, player.level); //map_width, map_height, map_seed);
    admin.start();
    GameAI ai(admin, 1);
    ai.start();
    CHuman human(admin, renderer);
    //GameCtrl gameCtrl(admin, renderer, ai, human);
    //gameCtrl.AddPlayer(player);
    auto gameCtrl = std::make_unique<GameCtrl>(admin, renderer, ai, human);
    gameCtrl->AddPlayer(player);

    std::cout << "Hello, " << player.name << "! Preparing to start the game..." << std::endl;
    sleep(1);
    b_start_game = true; // default
    
    // enter game
    if (b_start_game){
        /*struct termios new_t;
        tcgetattr(STDIN_FILENO, &old_t); // save original settings
        atexit(reset_terminal);  // ensure restoration on exit
        new_t = old_t;
        new_t.c_lflag &= ~(ICANON | ECHO); // disable echo and canonical mode
        tcsetattr(STDIN_FILENO, TCSANOW, &new_t);*/
        initscr(); // init ncurses mode
        keypad(stdscr, TRUE); // start enabling function keys (including arrow keys)
        noecho(); // Don't echo input characters on the screen
        gameCtrl->Init();
        LOG_INFOF("Entering main loop...");

        // loop keyboard input
        fd_set fds;
        //struct timeval tv;
        while (true) {
            //tv.tv_sec = 1; // wait 1 second
            //tv.tv_usec = 0;
            FD_ZERO(&fds);
            FD_SET(STDIN_FILENO, &fds);
            if (select(STDIN_FILENO + 1, &fds, NULL, NULL, NULL) > 0) { // &tv
                int c = getchar();
                int ret = gameCtrl->ProcessUserInput(c);
                LOG_INFOF("GameCtrl input processed: %c ret-%d", c, ret);
                if (ret == 1) // human game input
                {
                    int r = human.ProcessUserInput(c);
                    LOG_INFOF("Human input processed: %c r-%d", c, r);
                }
                else if (ret == -1) {
                    b_exit_game = true;
                }

                if (b_exit_game) {
                    LOG_INFOF("Exiting main loop...");
                    break;
                }
            }
            else {
                LOG_ERRORF("Select error occurred.");
                break;
            }
        }
        endwin(); // End ncurses mode
    }

    // stop all
    ai.stop();
    admin.stop();
    renderer.stop();

    std::cout << "Happy every day! Bye:)" << std::endl;
    return 0;
}
