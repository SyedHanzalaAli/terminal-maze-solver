#include <iostream>
#include <iomanip>
#include <vector>
#include <chrono>
#include <algorithm>
#include <stack>
#include <random>
#include <queue>
#include <map>
#include <conio.h>
#include <windows.h> // For Sleep function

// --- ANSI Color Codes ---
#define BG_WALL "\033[40m"       // Black background
#define BG_PATH "\033[0m"        // Default/Reset (no background)
#define BG_VISITED "\033[43m"    // Yellow background
#define C_PLAYER "\033[97;44m"   // White on Blue
#define C_OPPONENT "\033[97;41m" // White on Red
#define C_EXIT "\033[0m"         // Default/Reset (no background)
#define C_START "\033[97;44m"    // White on Blue
#define RESET "\033[0m"

using namespace std;

struct Position
{
    int x;
    int y;

    bool operator==(const Position &other) const
    {
        return x == other.x && y == other.y;
    }
    bool operator!=(const Position &other) const
    {
        return !(*this == other);
    }
    bool operator<(const Position &other) const
    {
        return y < other.y || (y == other.y && x < other.x);
    }
};

auto seed = chrono::high_resolution_clock::now().time_since_epoch().count();
mt19937 g(seed);

void setupConsole()
{
    system("cls");
    cout << "\033[?25l"; // Hide cursor
}

void cleanupConsole()
{
    cout << "\033[?25h"; // Show cursor
}

void renderGame(const vector<vector<char>> &maze, Position player, Position opponent, Position start, Position exit)
{
    vector<vector<char>> displayGrid = maze;

    displayGrid[start.y][start.x] = 'S';
    displayGrid[exit.y][exit.x] = 'E';
    displayGrid[player.y][player.x] = 'P';
    displayGrid[opponent.y][opponent.x] = 'O';

    for (int y = 0; y < displayGrid.size(); ++y)
    {
        for (int x = 0; x < displayGrid[y].size(); ++x)
        {
            char cell = displayGrid[y][x];

            if (cell == 'P')
            {
                cout << C_PLAYER << "P " << RESET;
            }
            else if (cell == 'O')
            {
                cout << C_OPPONENT << "O " << RESET;
            }
            else if (cell == 'E')
            {
                cout << C_EXIT << "E " << RESET;
            }
            else if (cell == 'S')
            {
                cout << C_START << "S " << RESET;
            }
            else if (cell == '#')
            {
                cout << BG_WALL << "  " << RESET;
            }
            else if (cell == ' ')
            {
                cout << BG_PATH << "  " << RESET;
            }
            else
            {
                cout << cell << " ";
            }
        }
        cout << endl;
    }

    cout << "\033[" << displayGrid.size() << "A";
    cout << "\033[" << displayGrid[0].size() * 2 << "D";
}

/**
 * @brief Generates a more open maze with multiple pathways
 */
vector<vector<char>> generateMaze(int width, int height, Position &start, Position &exit)
{
    if (width % 2 == 0)
        width++;
    if (height % 2 == 0)
        height++;

    vector<vector<char>> maze(height, vector<char>(width));

    // Initialize with walls
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            maze[y][x] = '#';
        }
    }

    stack<Position> cell_stack;

    // Generate base maze
    start = {1, 1};
    maze[start.y][start.x] = ' ';
    cell_stack.push(start);

    int dirs[4][2] = {{0, 2}, {0, -2}, {2, 0}, {-2, 0}};

    while (!cell_stack.empty())
    {
        Position current = cell_stack.top();
        vector<int> neighbors;

        for (int i = 0; i < 4; ++i)
        {
            int nx = current.x + dirs[i][0];
            int ny = current.y + dirs[i][1];

            if (nx > 0 && nx < width - 1 && ny > 0 && ny < height - 1 && maze[ny][nx] == '#')
            {
                neighbors.push_back(i);
            }
        }

        if (!neighbors.empty())
        {
            shuffle(neighbors.begin(), neighbors.end(), g);
            int chosen_dir_index = neighbors[0];

            int nx = current.x + dirs[chosen_dir_index][0];
            int ny = current.y + dirs[chosen_dir_index][1];

            int wallX = current.x + dirs[chosen_dir_index][0] / 2;
            int wallY = current.y + dirs[chosen_dir_index][1] / 2;
            maze[wallY][wallX] = ' ';

            maze[ny][nx] = ' ';
            cell_stack.push({nx, ny});
        }
        else
        {
            cell_stack.pop();
        }
    }

    // --- ENHANCED: Create more open spaces and alternative routes ---
    
    // 1. Remove walls to create larger open areas (rooms)
    int roomsToCreate = (width * height) / 150; // Create several rooms
    uniform_int_distribution<> distX(3, width - 4);
    uniform_int_distribution<> distY(3, height - 4);
    
    for (int r = 0; r < roomsToCreate; ++r)
    {
        int roomX = distX(g);
        int roomY = distY(g);
        int roomWidth = 3 + (g() % 4);  // 3-6 wide
        int roomHeight = 3 + (g() % 3); // 3-5 tall
        
        for (int y = roomY; y < min(roomY + roomHeight, height - 1); ++y)
        {
            for (int x = roomX; x < min(roomX + roomWidth, width - 1); ++x)
            {
                if (x > 0 && y > 0)
                {
                    maze[y][x] = ' ';
                }
            }
        }
    }

    // 2. Create additional connections between paths (more loops/choices)
    int connectionsToAdd = (width * height) / 30; // Add many alternative routes
    for (int i = 0; i < connectionsToAdd; ++i)
    {
        int rx = 1 + (g() % (width - 2));
        int ry = 1 + (g() % (height - 2));
        
        // Only remove walls that have paths on both sides
        if (maze[ry][rx] == '#')
        {
            int pathCount = 0;
            if (rx > 0 && maze[ry][rx - 1] == ' ') pathCount++;
            if (rx < width - 1 && maze[ry][rx + 1] == ' ') pathCount++;
            if (ry > 0 && maze[ry - 1][rx] == ' ') pathCount++;
            if (ry < height - 1 && maze[ry + 1][rx] == ' ') pathCount++;
            
            // If there are paths nearby, open this wall
            if (pathCount >= 2)
            {
                maze[ry][rx] = ' ';
            }
        }
    }

    // 3. Remove random wall segments to add variety
    int wallsToRemove = (width * height) / 15;
    for (int i = 0; i < wallsToRemove; ++i)
    {
        int rx = 1 + (g() % (width - 2));
        int ry = 1 + (g() % (height - 2));
        if (maze[ry][rx] == '#')
        {
            maze[ry][rx] = ' ';
        }
    }

    // Set start and exit positions
    start = {1, 1};
    exit = {width - 2, height - 2};
    maze[start.y][start.x] = ' ';
    maze[exit.y][exit.x] = ' ';

    return maze;
}

void handleInput(const vector<vector<char>> &maze, Position &player)
{
    if (!_kbhit())
        return;

    // Flush keyboard buffer - only get the LAST key pressed
    char input = 0;
    while (_kbhit())
    {
        input = _getch();
    }

    Position nextPos = player;

    if (input == 72 || input == 'w' || input == 'W')
    { // Up Arrow or W
        nextPos.y--;
    }
    else if (input == 80 || input == 's' || input == 'S')
    { // Down Arrow or S
        nextPos.y++;
    }
    else if (input == 75 || input == 'a' || input == 'A')
    { // Left Arrow or A
        nextPos.x--;
    }
    else if (input == 77 || input == 'd' || input == 'D')
    { // Right Arrow or D
        nextPos.x++;
    }

    // Check for collision
    if (nextPos.x > 0 && nextPos.x < maze[0].size() - 1 &&
        nextPos.y > 0 && nextPos.y < maze.size() - 1 &&
        maze[nextPos.y][nextPos.x] != '#')
    {
        player = nextPos;
    }
}

Position getNextStepBFS(const vector<vector<char>> &maze, Position start, Position end)
{
    queue<Position> q;
    map<Position, Position> parent;
    map<Position, bool> visited;

    q.push(start);
    visited[start] = true;

    Position pathFoundEnd;
    bool pathFound = false;

    int dirs[4][2] = {{0, 1}, {0, -1}, {1, 0}, {-1, 0}};

    while (!q.empty())
    {
        Position current = q.front();
        q.pop();

        if (current == end)
        {
            pathFound = true;
            pathFoundEnd = current;
            break;
        }

        for (int i = 0; i < 4; ++i)
        {
            Position next = {current.x + dirs[i][0], current.y + dirs[i][1]};

            if (next.x > 0 && next.x < maze[0].size() - 1 &&
                next.y > 0 && next.y < maze.size() - 1 &&
                maze[next.y][next.x] != '#' &&
                !visited[next])
            {
                visited[next] = true;
                parent[next] = current;
                q.push(next);
            }
        }
    }

    if (!pathFound)
    {
        return start;
    }

    // Backtrack to find the first step
    Position step = pathFoundEnd;
    while (parent.find(step) != parent.end() && parent[step] != start)
    {
        step = parent[step];
    }
    return step;
}

/**
 * @brief Calculates Manhattan distance between two positions
 */
int manhattanDistance(Position a, Position b)
{
    return abs(a.x - b.x) + abs(a.y - b.y);
}

/**
 * @brief Bot AI that tries to avoid the opponent while reaching the exit
 * Uses A* with danger zones around the opponent
 */
Position getBotNextMove(const vector<vector<char>> &maze, Position bot, Position opponent, Position exit)
{
    // If we're at the exit, don't move
    if (bot == exit)
        return bot;

    // Priority queue: (priority, position)
    // Lower priority = better path
    priority_queue<pair<int, Position>, vector<pair<int, Position>>, greater<pair<int, Position>>> pq;
    map<Position, Position> parent;
    map<Position, int> cost; // g-cost (actual cost from start)
    map<Position, bool> visited;

    pq.push({0, bot});
    cost[bot] = 0;
    parent[bot] = bot;

    Position pathFoundEnd;
    bool pathFound = false;

    int dirs[4][2] = {{0, 1}, {0, -1}, {1, 0}, {-1, 0}};

    while (!pq.empty())
    {
        Position current = pq.top().second;
        pq.pop();

        if (visited[current])
            continue;
        visited[current] = true;

        if (current == exit)
        {
            pathFound = true;
            pathFoundEnd = current;
            break;
        }

        for (int i = 0; i < 4; ++i)
        {
            Position next = {current.x + dirs[i][0], current.y + dirs[i][1]};

            if (next.x > 0 && next.x < maze[0].size() - 1 &&
                next.y > 0 && next.y < maze.size() - 1 &&
                maze[next.y][next.x] != '#' &&
                !visited[next])
            {
                // Calculate danger penalty based on distance to opponent
                int distToOpponent = manhattanDistance(next, opponent);
                int dangerPenalty = 0;

                // Heavy penalty for being close to opponent
                if (distToOpponent <= 2)
                    dangerPenalty = 100; // Very dangerous
                else if (distToOpponent <= 4)
                    dangerPenalty = 50; // Dangerous
                else if (distToOpponent <= 6)
                    dangerPenalty = 20; // Slightly dangerous

                // g-cost: actual cost to reach this node
                int newCost = cost[current] + 1 + dangerPenalty;

                // Only update if we found a better path
                if (cost.find(next) == cost.end() || newCost < cost[next])
                {
                    cost[next] = newCost;
                    parent[next] = current;

                    // f-cost = g-cost + h-cost (heuristic to exit)
                    int hCost = manhattanDistance(next, exit);
                    int fCost = newCost + hCost;

                    pq.push({fCost, next});
                }
            }
        }
    }

    if (!pathFound)
    {
        // If no path found, try to move away from opponent
        Position best = bot;
        int maxDist = manhattanDistance(bot, opponent);

        for (int i = 0; i < 4; ++i)
        {
            Position next = {bot.x + dirs[i][0], bot.y + dirs[i][1]};

            if (next.x > 0 && next.x < maze[0].size() - 1 &&
                next.y > 0 && next.y < maze.size() - 1 &&
                maze[next.y][next.x] != '#')
            {
                int dist = manhattanDistance(next, opponent);
                if (dist > maxDist)
                {
                    maxDist = dist;
                    best = next;
                }
            }
        }
        return best;
    }

    // Backtrack to find the first step
    Position step = pathFoundEnd;
    while (parent.find(step) != parent.end() && parent[step] != bot)
    {
        step = parent[step];
    }
    return step;
}

void updateOpponent(const vector<vector<char>> &maze, Position player, Position &opponent)
{
    opponent = getNextStepBFS(maze, opponent, player);
}

int main()
{
    setupConsole();

    // Slightly larger maze for more strategic gameplay
    int width = 55;
    int height = 27;

    Position start, exit;
    vector<vector<char>> maze = generateMaze(width, height, start, exit);

    Position player = start;
    Position opponent = {width - 2, 1}; // Start opponent in top-right

    bool gameOver = false;
    bool gameWon = false;
    int gameTick = 0; // Counter for game ticks
    bool botMode = false;
    bool modeSelected = false;

    cout << "=== MAZE CHASE ===" << endl;
    cout << "Press 'B' to watch BOT play" << endl;
    cout << "Press Arrow Keys or WASD to play as HUMAN" << endl;
    cout << "Reach the EXIT (E) before the OPPONENT (O) catches you!" << endl;

    // Main Game Loop
    while (!gameOver && !gameWon)
    {
        renderGame(maze, player, opponent, start, exit);
        
        // Mode selection on first input
        if (!modeSelected)
        {
            if (_kbhit())
            {
                char input = 0;
                // Flush buffer and get last key
                while (_kbhit())
                {
                    input = _getch();
                }

                if (input == 'b' || input == 'B')
                {
                    botMode = true;
                    modeSelected = true;
                }
                else if (input == 72 || input == 80 || input == 75 || input == 77 || // Arrow keys
                         input == 'w' || input == 'W' || input == 's' || input == 'S' ||
                         input == 'a' || input == 'A' || input == 'd' || input == 'D')
                {
                    botMode = false;
                    modeSelected = true;
                    // Process this first input for player movement
                    Position nextPos = player;
                    if (input == 72 || input == 'w' || input == 'W')
                        nextPos.y--;
                    else if (input == 80 || input == 's' || input == 'S')
                        nextPos.y++;
                    else if (input == 75 || input == 'a' || input == 'A')
                        nextPos.x--;
                    else if (input == 77 || input == 'd' || input == 'D')
                        nextPos.x++;

                    if (nextPos.x > 0 && nextPos.x < maze[0].size() - 1 &&
                        nextPos.y > 0 && nextPos.y < maze.size() - 1 &&
                        maze[nextPos.y][nextPos.x] != '#')
                    {
                        player = nextPos;
                    }
                }
            }
        }
        else
        {
            // Game mode is selected, play accordingly
            if (botMode)
            {
                // Bot moves every tick
                player = getBotNextMove(maze, player, opponent, exit);
            }
            else
            {
                // Human player controls
                handleInput(maze, player);
            }
        }
        
        // Opponent only moves every 4 ticks (slower) and only after mode is selected
        if (modeSelected && gameTick % 1 == 0)
        {
            updateOpponent(maze, player, opponent);
        }

        if (player == opponent)
        {
            gameOver = true;
        }
        if (player == exit)
        {
            gameWon = true;
        }

        gameTick++;
        Sleep(75); // Half of 150ms = faster game updates
    }

    renderGame(maze, player, opponent, start, exit);

    cout << "\033[" << (height + 2) << "B" << endl;

    if (gameWon)
    {
        cout << C_PLAYER << "********************" << RESET << endl;
        if (botMode)
        {
            cout << C_PLAYER << "**   BOT WINS!    **" << RESET << endl;
        }
        else
        {
            cout << C_PLAYER << "**   YOU WIN!     **" << RESET << endl;
        }
        cout << C_PLAYER << "**  ESCAPED!      **" << RESET << endl;
        cout << C_PLAYER << "********************" << RESET << endl;
    }
    else
    {
        cout << C_OPPONENT << "********************" << RESET << endl;
        if (botMode)
        {
            cout << C_OPPONENT << "**  BOT CAUGHT!   **" << RESET << endl;
        }
        else
        {
            cout << C_OPPONENT << "**  GAME OVER!    **" << RESET << endl;
        }
        cout << C_OPPONENT << "**   CAUGHT!      **" << RESET << endl;
        cout << C_OPPONENT << "********************" << RESET << endl;
    }

    cleanupConsole();
    return 0;
}