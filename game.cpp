#define GL_SILENCE_DEPRECATION
#include <GLUT/glut.h>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <iostream>
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>



// Global sound variables
Mix_Music* backgroundMusic = nullptr;
Mix_Chunk* collectSound = nullptr;
Mix_Chunk* collideSound = nullptr;
Mix_Chunk* gameWon = nullptr;
Mix_Chunk* gameLost = nullptr;

// Function to initialize SDL_mixer
void init() {
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) == -1) {
        std::cerr << "SDL_mixer could not initialize! SDL_mixer Error: " << Mix_GetError() << std::endl;
    }

    // Load sounds
    backgroundMusic = Mix_LoadMUS("/Users/macbookpro/Downloads/Background_sound.mp3");
    collectSound= Mix_LoadWAV("/Users/macbookpro/Downloads/collecting_collectable_sound.wav");
    collideSound = Mix_LoadWAV("/Users/macbookpro/Downloads/hit_obstacle.wav");
    gameWon = Mix_LoadWAV("/Users/macbookpro/Downloads/game_won.wav");
    gameLost = Mix_LoadWAV("/Users/macbookpro/Downloads/game_lost.wav");
    
    if (!backgroundMusic || !collectSound || !collideSound || !gameWon || !gameLost) {
            std::cerr << "Error loading sound files! SDL_mixer Error: " << Mix_GetError() << std::endl;
        }
    
}

// Player struct definition
typedef struct {
    float x, y; // Position of the player
    float width, height; // Size of the player
} Player;

Player player; // Global player object

// Obstacle struct definition
typedef struct {
    float x, y; // Position of the obstacle
    float width, height; // Size of the obstacle
} Obstacle;


//Player stats
int lives = 5;
int score = 0;

std::vector<Obstacle> obstacles; // Vector to hold multiple obstacles

typedef struct {
    float x, y; // Position of the collectable
    float size; // Size of the collectable
    float angle; // Rotation angle for the collectable
} Collectable;

std::vector<Collectable> collectables;


typedef struct {
    float x, y;   // Position of the cloud
    float scale;  // Size of the cloud
} Cloud;

std::vector<Cloud> clouds; // Vector to hold clouds

const float COLLECTABLE_SIZE = 15.0f; // Set a fixed size for collectables

typedef struct {
    float x, y; // Position of the power-up
    float size; // Size of the power-up
    float angle; // Rotation angle for the power-up
    bool isActive; // Whether the power-up is currently active
    float duration; // Duration for which the power-up effect lasts
    float timer; // Timer to track the duration
    int type; // 0 for speed boost, 1 for invulnerability
} PowerUp;

std::vector<PowerUp> powerUps; // Vector to hold multiple power-ups




// Speed control
float obstacleSpeed = 0.5f; // Start with slower speed
float speedIncreaseRate = 0.01f; // Amount by which speed increases each update

int lastObstaclePosition = 300; // Track the last position of the generated obstacle
float downwardVelocity = 0.0f; // Downward velocity when ducking
float maxDuckHeight = 10.0f; // Minimum height of the player when ducking
float groundLevel = 50.0f; // Ground level
float groundLevelObs = 30.0f; // Ground level

float elapsedTime = 0.0f; // Keep track of elapsed time
float timeIncrement = 1.0f; // Time increment for speed increase (in seconds)
bool gameOver = false;
bool canTakeDamage = true;
float damageCooldown = 1.0f; // 1 second cooldown between taking damage
float damageCooldownTimer = 0.0f;
bool isInvulnerable = false;   // Flag to track invulnerability state
float invulnerabilityDuration = 5.0f; // 1 second invulnerability
float invulnerabilityTimer = 0.0f;

float elapsedTimeSinceLastTick = 0.0f; // Time since the last tick
int totalTime = 45; // Total time for the countdown (60 seconds)
bool isTimerRunning = true;
bool gameEnd;

float powerUpSpawnTimer = 0.0f; // Timer to track time since last power-up spawn
float powerUpSpawnInterval = 10.0f; // Time interval between power-ups






void updateTimer(float deltaTime) {
    if(!(gameEnd || gameOver)){
        if (isTimerRunning) {
            elapsedTimeSinceLastTick += deltaTime; // Increment elapsed time

            // Check if one second has passed
            if (elapsedTimeSinceLastTick >= 1.0f) {
                totalTime--; // Decrease total time by 1 second
                elapsedTimeSinceLastTick -= 1.0f; // Subtract 1 second

                // Check if the timer has reached zero
                if (totalTime <= 0) {
                    isTimerRunning = false; // Stop the timer
                    gameEnd = true; // Set the game over flag due to time up
                }
            }
        }
    }
    
}

// Function to draw the player
void drawPlayer() {
    // Draw body (Quad)
    glBegin(GL_QUADS);
    glColor3f(0.0f, 0.0f, 1.0f); // Blue for the body
    glVertex2f(player.x - player.width / 2, player.y); // Bottom-left
    glVertex2f(player.x + player.width / 2, player.y); // Bottom-right
    glVertex2f(player.x + player.width / 2, player.y + player.height); // Top-right
    glVertex2f(player.x - player.width / 2, player.y + player.height); // Top-left
    glEnd();

    // Draw head
    glBegin(GL_TRIANGLE_FAN);
    glColor3f(1.0f, 0.8f, 0.6f); // Skin color for the head
    glVertex2f(player.x, player.y + player.height + 10); // Center of the head
    for (int i = 0; i <= 20; i++) {
        glVertex2f(
            player.x + 10 * cos(i * 2.0f * M_PI / 20),
            player.y + player.height + 10 + 10 * sin(i * 2.0f * M_PI / 20)
        );
    }
    glEnd();

    // Draw arms
    glLineWidth(3.0f);
    glBegin(GL_LINES);
    glColor3f(1.0f, 0.8f, 0.6f);

    // Left arm
    glVertex2f(player.x - player.width / 2, player.y + player.height * 0.75f);
    glVertex2f(player.x - player.width / 2 - 15, player.y + player.height * 0.75f - 15);

    // Right arm
    glVertex2f(player.x + player.width / 2, player.y + player.height * 0.75f);
    glVertex2f(player.x + player.width / 2 + 15, player.y + player.height * 0.75f - 15);
    glEnd();

    // Draw legs
    glBegin(GL_TRIANGLES);
    glColor3f(0.0f, 0.0f, 0.0f); // Black for the legs

    // Left leg
    glVertex2f(player.x - player.width / 6, player.y); // Bottom-left
    glVertex2f(player.x - player.width / 6 + 5, player.y - 20); // Point of the leg
    glVertex2f(player.x - player.width / 6 + 10, player.y); // Bottom-right

    // Right leg
    glVertex2f(player.x + player.width / 6, player.y); // Bottom-left
    glVertex2f(player.x + player.width / 6 - 5, player.y - 20); // Point of the leg
    glVertex2f(player.x + player.width / 6 - 10, player.y); // Bottom-right
    glEnd();

    // Draw mouth
    glBegin(GL_LINE_STRIP);
    glColor3f(0.0f, 0.0f, 0.0f); // Black color for the mouth
    glVertex2f(player.x - 5, player.y + player.height + 7);  // Left point of the mouth
    glVertex2f(player.x, player.y + player.height + 5);       // Middle point (slightly lower for curve)
    glVertex2f(player.x + 5, player.y + player.height + 7);   // Right point of the mouth
    glEnd();

    // Left eye
    glBegin(GL_TRIANGLE_FAN);
    glColor3f(0.0f, 0.0f, 0.0f); // Black for the eyes
    glVertex2f(player.x - 4, player.y + player.height + 12); // Center of the left eye
    for (int i = 0; i <= 20; i++) {
        glVertex2f(
        player.x - 4 + 2 * cos(i * 2.0f * M_PI / 20),
        player.y + player.height + 12 + 2 * sin(i * 2.0f * M_PI / 20)
        );
    }
    glEnd();

    // Right eye
    glBegin(GL_TRIANGLE_FAN);
    glColor3f(0.0f, 0.0f, 0.0f); // Black for the eyes
    glVertex2f(player.x + 4, player.y + player.height + 12); // Center of the right eye
    for (int i = 0; i <= 20; i++) {
        glVertex2f(
                   player.x + 4 + 2 * cos(i * 2.0f * M_PI / 20),
                   player.y + player.height + 12 + 2 * sin(i * 2.0f * M_PI / 20)
                   );
    }
    glEnd();
}


//draw hearts (for health)
void drawHeartWithOutline(float x, float y, float scale) {
    // Draw the heart (polygon)
    glBegin(GL_POLYGON);
    glColor3f(1.0f, 0.0f, 0.0f); // Red for the heart
    for (int i = 0; i < 360; i++) {
        float angle = i * M_PI / 180;
        float heartX = scale * (16 * pow(sin(angle), 3));
        float heartY = scale * (13 * cos(angle) - 5 * cos(2 * angle) - 2 * cos(3 * angle) - cos(4 * angle));
        glVertex2f(x + heartX, y + heartY);
    }
    glEnd();

    // Draw the outline (using lines)
    glBegin(GL_LINE_LOOP);
    glColor3f(0.0f, 0.0f, 0.0f); // Black outline for the heart
    for (int i = 0; i < 360; i++) {
        float angle = i * M_PI / 180;
        float heartX = scale * (16 * pow(sin(angle), 3));
        float heartY = scale * (13 * cos(angle) - 5 * cos(2 * angle) - 2 * cos(3 * angle) - cos(4 * angle));
        glVertex2f(x + heartX, y + heartY);
    }
    glEnd();
}

void drawSun() {
    float sunX = 250.0f; // X position for the sun (top-right)
    float sunY = 230.0f; // Y position for the sun (top-right)
    float sunRadius = 20.0f; // Increased radius of the sun core

    // Draw sun core (circle)
    glColor3f(1.0f, 1.0f, 0.0f); // Yellow color for the sun core
    glBegin(GL_POLYGON);
    for (int i = 0; i <= 360; i++) {
        float angle = i * M_PI / 180.0f;
        glVertex2f(sunX + sunRadius * cos(angle), sunY + sunRadius * sin(angle));
    }
    glEnd();

    // Draw sun rays (smaller triangles with base inward)
    glColor3f(1.0f, 1.0f, 0.0f); // Yellow for sun rays
    float rayLength = 15.0f; // Length of the sun rays
    float rayWidth = 10.0f;   // Width of the sun rays

    for (int i = 0; i < 360; i += 45) { // Create rays every 45 degrees
        float angle = i * M_PI / 180.0f;
        glBegin(GL_TRIANGLES);
        // Base of the triangle near the sun
        glVertex2f(sunX + (sunRadius - rayWidth) * cos(angle - 0.1f), sunY + (sunRadius - rayWidth) * sin(angle - 0.1f)); // Left base point
        glVertex2f(sunX + (sunRadius - rayWidth) * cos(angle + 0.1f), sunY + (sunRadius - rayWidth) * sin(angle + 0.1f)); // Right base point
        // Tip of the triangle pointing outwards
        glVertex2f(sunX + (sunRadius + rayLength) * cos(angle), sunY + (sunRadius + rayLength) * sin(angle)); // Tip point
        glEnd();
    }
}

// draw the health bar (hearts)
void drawHealthBar() {
    float heartSpacing = 10.0f; // Spacing between hearts
    float startX = 10.0f; // Initial X position for hearts
    float startY = 250.0f; // Y position at the top of the screen
    float heartScale = 0.15f; // Scale for hearts

    for (int i = 0; i < lives; i++) {
        drawHeartWithOutline(startX + i * heartSpacing, startY, heartScale); // Draw heart with outline
    }
}

void drawCloud(float x, float& y, float scale) {
    glColor3f(1.0f, 1.0f, 1.0f); // White color for clouds

    // Draw multiple circles to form a cloud
    // Circle 1
    glBegin(GL_TRIANGLE_FAN);
    for (int i = 0; i <= 360; i += 10) {
        float angle = i * M_PI / 180.0f;
        glVertex2f(x + scale * cos(angle), y + scale * sin(angle));
    }
    glEnd();

    // Circle 2
    glBegin(GL_TRIANGLE_FAN);
    for (int i = 0; i <= 360; i += 10) {
        float angle = i * M_PI / 180.0f;
        glVertex2f(x + scale * 0.6f + scale * cos(angle), y + scale * 0.2f + scale * sin(angle));
    }
    glEnd();

    // Circle 3
    glBegin(GL_TRIANGLE_FAN);
    for (int i = 0; i <= 360; i += 10) {
        float angle = i * M_PI / 180.0f;
        glVertex2f(x - scale * 0.6f + scale * cos(angle), y + scale * 0.2f + scale * sin(angle));
    }
    glEnd();
}



// Flag to track jumping state
bool isJumping = false; // Keep track of whether the player is mid-jump
bool canJump = true; // Allows consecutive jumps
float jumpHeight = 40.0f; // Maximum height the player will jump
float velocity = 0.0f; // The speed at which the player is jumping or falling
float gravity = 0.2f;


// Function to draw the obstacle
void drawObstacle(const Obstacle& obstacle) {
    // Draw a rectangular obstacle (Quad)
    glBegin(GL_QUADS);
    glColor3f(0.0f, 1.0f, 0.0f); // Green color for the rectangular obstacle
    glVertex2f(obstacle.x - obstacle.width / 2, obstacle.y); // Bottom-left
    glVertex2f(obstacle.x + obstacle.width / 2, obstacle.y); // Bottom-right
    glVertex2f(obstacle.x + obstacle.width / 2, obstacle.y + obstacle.height); // Top-right
    glVertex2f(obstacle.x - obstacle.width / 2, obstacle.y + obstacle.height); // Top-left
    glEnd();

    // Draw a triangular spike on top (Triangle)
    glBegin(GL_TRIANGLES);
    glColor3f(1.0f, 0.0f, 0.0f); // Red color for the spike
    glVertex2f(obstacle.x - obstacle.width / 2, obstacle.y + obstacle.height); // Left point
    glVertex2f(obstacle.x + obstacle.width / 2, obstacle.y + obstacle.height); // Right point
    glVertex2f(obstacle.x, obstacle.y + obstacle.height + 20); // Top point (spike)
    glEnd();
}

// Function to draw a collectable
void drawCollectable(const Collectable& collectable) {
    glPushMatrix(); // Save the current transformation matrix

    glTranslatef(collectable.x, collectable.y, 0); // Move to the position of the collectable
    glRotatef(collectable.angle, 0.0f, 0.0f, 1.0f); // Rotate around the Z-axis

    // Draw the hexagon part of the collectable (using POLYGON)
    glBegin(GL_POLYGON);
    glColor3f(1.0f, 0.5f, 0.0f); // Orange color for the hexagon
    for (int i = 0; i < 6; i++) { // Hexagon has 6 sides
        float angle = i * 2.0f * M_PI / 6; // Divide the circle into 6 segments for the hexagon
        glVertex2f((collectable.size / 2) * cos(angle), (collectable.size / 2) * sin(angle));
    }
    glEnd();

    // Draw triangles coming out of the hexagon (using TRIANGLES)
    glBegin(GL_TRIANGLES);
    glColor3f(1.0f, 1.0f, 0.0f); // Yellow color for the triangles
    for (int i = 0; i < 360; i += 60) {
        float angle = i * M_PI / 180.0f;
        float nextAngle = (i + 20) * M_PI / 180.0f; // Size of each triangle base
        glVertex2f((collectable.size / 2) * cos(angle), (collectable.size / 2) * sin(angle)); // First point on the hexagon
        glVertex2f((collectable.size / 2) * cos(nextAngle), (collectable.size / 2) * sin(nextAngle)); // Second point on the hexagon
        glVertex2f((collectable.size / 2 + 10) * cos((angle + nextAngle) / 2), (collectable.size / 2 + 10) * sin((angle + nextAngle) / 2)); // Tip of the triangle
    }
    glEnd();

    // Draw the square inside the hexagon (using QUADS)
    glBegin(GL_QUADS);
    glColor3f(0.0f, 0.0f, 1.0f); // Blue color for the square
    glVertex2f(-collectable.size / 4, -collectable.size / 4); // Bottom-left
    glVertex2f(collectable.size / 4, -collectable.size / 4); // Bottom-right
    glVertex2f(collectable.size / 4, collectable.size / 4); // Top-right
    glVertex2f(-collectable.size / 4, collectable.size / 4); // Top-left
    glEnd();

    glPopMatrix(); // Restore the previous transformation matrix
}



//generate multiple obstacles on the ground and above the ground
void generateObstacles() {
    while (obstacles.size() < 3) { // Keep generating until there are 3 obstacles
        Obstacle obs;
        obs.width = 20.0f;
        obs.height = rand() % 30 + 20; // Random height

        // Set the obstacle either on the ground or above the ground
        if (rand() % 2 == 0) {
            obs.y = groundLevelObs; // Place the obstacle on the ground level
        } else {
            obs.y = groundLevelObs + rand() % 100 + 30; // Random height above the ground
        }

        obs.x = lastObstaclePosition + rand() % 100 + 150; // Random X position spaced out
        obstacles.push_back(obs); // Add to the vector

        lastObstaclePosition = obs.x; // Update the last obstacle position
    }
}


bool isPlayerCollidingWithObstacle(const Obstacle& obs) {
    // Check if the player's X and Y coordinates overlap with the obstacle
    bool isCollidingX = (player.x + player.width / 2 >= obs.x - obs.width / 2) &&
                        (player.x - player.width / 2 <= obs.x + obs.width / 2);
    bool isCollidingY = (player.y <= obs.y + obs.height) &&
                        (player.y + player.height >= obs.y);

    return isCollidingX && isCollidingY;
}



void updateObstacles() {
    if (gameOver || gameEnd) return;
    if (!gameOver) {

        // Iterate through obstacles and check for collisions
        for (auto it = obstacles.begin(); it != obstacles.end(); ) {
            it->x -= obstacleSpeed; // Move the obstacle to the left

            // Remove obstacle if it goes off-screen
            if (it->x < -it->width) {
                it = obstacles.erase(it); // Remove the obstacle and continue
                continue; // Move to the next obstacle
            }

            // Check for collision with player
            if (!isInvulnerable && isPlayerCollidingWithObstacle(*it)) {
                // Only decrease lives if the player is not invulnerable
                lives -= 1; // Decrease lives on collision
                Mix_PlayChannel(-1, collideSound, 0); // Play collision sound


                // Remove the specific obstacle after collision
                it = obstacles.erase(it); // Remove the obstacle

                // If no lives are left, trigger game over
                if (lives <= 0) {
                    gameOver = true; // Set the game over flag
                    Mix_PlayChannel(-1, gameLost, 0);
                }

                continue; // Move to the next obstacle
            }

            ++it; // Move to the next obstacle
        }

        // Gradually increase speed
        elapsedTime += timeIncrement;
        if (elapsedTime >= 2.0f) {
            obstacleSpeed += speedIncreaseRate; // Increase obstacle speed
            elapsedTime = 0.0f; // Reset elapsed time
        }

        // Generate new obstacles if fewer than 3 exist
        if (obstacles.size() < 3) {
            generateObstacles(); // Call your obstacle generation function
        }
    }
}



void renderGameOver() {
    glColor3f(1.0f, 0.0f, 0.0f); // Red color for Game Over text
    glRasterPos2f(120.0f, 150.0f); // Position the text on the screen

    const char* gameOverText = "GAME OVER"; // The Game Over text
    for (const char* c = gameOverText; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c); // Render each character
    }
}


void specialKeyboard(int key, int x, int y) {
    switch (key) {
        case GLUT_KEY_UP: // Up arrow key for jump
            if (canJump) {
                isJumping = true;
                velocity = 5.0f; // Set upward velocity
                canJump = false; // Prevent multiple jumps mid-air

            }
            break;
        case GLUT_KEY_DOWN: // Down arrow key for duck
            player.height = maxDuckHeight; // Make the player smaller while ducking
            player.y = groundLevel - player.height; // Adjust Y position for ducking
            break;
    }
}

// Keyboard input handling for key release (stop ducking)
void specialKeyboardUp(int key, int x, int y) {
    switch (key) {
        case GLUT_KEY_DOWN: // Down arrow key released
            player.y += (20.0f - maxDuckHeight); // Adjust Y position to prevent going under ground
            player.height = 20.0f; // Reset to original height
            break;
    }
}


// Update player position
void updatePlayer() {
    if (isJumping) {
        player.y += velocity; // Move the player up or down
        velocity -= gravity; // Gravity pulls the player down

        // If the player reaches the ground, stop the jump
        if (player.y <= groundLevel) {
            player.y = groundLevel; // Ensure the player stays on the ground
            isJumping = false; // End the jump
            canJump = true; // Allow the player to jump again
            velocity = 0.0f; // Reset the velocity after landing
        }
    }
}


void generateClouds() {
    clouds.clear();
    for (int i = 0; i < 3; i++) {
        Cloud cloud;
        cloud.x = rand() % 300;   // Random X position
        cloud.y = 150 + rand() % 50; // Random Y position (higher in the sky)
        cloud.scale = 20 + rand() % 10; // Random size
        clouds.push_back(cloud);
    }
}
void updateClouds() {
    for (Cloud& cloud : clouds) {
        cloud.x -= 0.1f; // Move the cloud downward slightly

        // If the cloud moves too far down, reset its position
        if (cloud.y < 100.0f) {
            cloud.y = 250.0f + rand() % 50; // Reset to the top
            cloud.x = rand() % 300;         // Randomize X position
        }
    }
}



// Function to check if a collectable overlaps with any obstacle
bool isOverlappingWithObstacle(const Collectable& col) {
    float margin = 10.0f; // Extra margin to prevent touching

    for (const Obstacle& obs : obstacles) {
        bool overlapX = (col.x + col.size / 2 + margin >= obs.x - obs.width / 2 - margin) &&
                        (col.x - col.size / 2 - margin <= obs.x + obs.width / 2 + margin);
        
        bool overlapY = (col.y <= obs.y + obs.height + margin) &&
                        (col.y >= obs.y - margin);

        if (overlapX && overlapY) {
            return true; // Overlap detected
        }
    }
    return false; // No overlap
}


bool isOverlappingWithObstacle(const PowerUp& powerUp) {
    float margin = 10.0f; // Extra margin to prevent touching

    for (const Obstacle& obs : obstacles) {
        bool overlapX = (powerUp.x + powerUp.size / 2 + margin >= obs.x - obs.width / 2 - margin) &&
                        (powerUp.x - powerUp.size / 2 - margin <= obs.x + obs.width / 2 + margin);

        bool overlapY = (powerUp.y <= obs.y + obs.height + margin) &&
                        (powerUp.y >= obs.y - margin);

        if (overlapX && overlapY) {
            return true; // Overlap detected
        }
    }
    return false; // No overlap
}


bool isOverlappingWithCollectable(const PowerUp& powerUp) {
    float margin = 10.0f; // Hardcoded margin

    for (const Collectable& col : collectables) {
        // Add margin to the overlap check
        bool overlapX = (powerUp.x + powerUp.size / 2 + margin >= col.x - col.size / 2 - margin) &&
                        (powerUp.x - powerUp.size / 2 - margin <= col.x + col.size / 2 + margin);
        bool overlapY = (powerUp.y + margin >= col.y - col.size / 2) &&
                        (powerUp.y - margin <= col.y + col.size / 2);
        if (overlapX && overlapY) {
            return true; // Overlap detected
        }
    }
    return false; // No overlap
}

bool isOverlappingWithPowerUp(const Collectable& col) {
    float margin = 10.0f; // Hardcoded margin

    for (const PowerUp& powerUp : powerUps) {
        // Add margin to the overlap check
        bool overlapX = (col.x + col.size / 2 + margin >= powerUp.x - powerUp.size / 2 - margin) &&
                        (col.x - col.size / 2 - margin <= powerUp.x + powerUp.size / 2 + margin);
        bool overlapY = (col.y + margin >= powerUp.y - powerUp.size / 2) &&
                        (col.y - margin <= powerUp.y + powerUp.size / 2);
        if (overlapX && overlapY) {
            return true; // Overlap detected
        }
    }
    return false; // No overlap
}



// Function to generate a new collectable with obstacle overlap prevention
Collectable generateNewCollectable() {
    Collectable col;
    col.size = COLLECTABLE_SIZE; // Use the fixed size

    // Regenerate position if overlapping with obstacles or power-ups
    do {
        col.x = 300 + rand() % 100; // Random X position to the right
        col.y = groundLevel + 10;   // Place collectable just above the ground
    } while (isOverlappingWithObstacle(col) || isOverlappingWithPowerUp(col)); // Ensure no overlap

    return col;
}



void updateCollectables() {
    if (gameOver || gameEnd) return;
    for (auto it = collectables.begin(); it != collectables.end(); ) {
        // Move the collectable to the left (simulate movement)
        it->x -= obstacleSpeed;

        // Update the rotation angle
        it->angle += 5.0f; // Adjust the speed of rotation as needed

        // If the collectable goes off-screen, reposition it
        if (it->x < -it->size) {
            *it = generateNewCollectable(); // Generate a new collectable
        }

        // Check collision with player
        if (fabs(player.x - it->x) < player.width / 2 + it->size / 2 &&
            fabs(player.y - it->y) < player.height / 2 + it->size / 2) {
            // Increase the score when a collectable is collected
            score += 1;
            Mix_PlayChannel(-1, collectSound, 0); // Play collect sound


            // Remove the specific collectable and add a new one
            it = collectables.erase(it);
            collectables.push_back(generateNewCollectable());
        } else {
            ++it; // Move to the next collectable
        }
    }
}




// Function to draw mountains in the background
void drawMountains() {
    // Hill 1
    glBegin(GL_TRIANGLES);
    glColor3f(0.4f, 0.26f, 0.13f); // Brownish color for the mountain
    glVertex2f(30.0f, 30.0f); // Bottom-left
    glVertex2f(100.0f, 150.0f); // Peak
    glVertex2f(170.0f, 30.0f); // Bottom-right
    glEnd();
    
    // Hill 2 (bigger one)
    glBegin(GL_TRIANGLES);
    glColor3f(0.5f, 0.3f, 0.15f); // Darker brown for variety
    glVertex2f(120.0f, 30.0f); // Bottom-left
    glVertex2f(220.0f, 170.0f); // Peak
    glVertex2f(300.0f, 30.0f); // Bottom-right
    glEnd();
    
    // Hill 3 (smaller one for layering effect)
    glBegin(GL_TRIANGLES);
    glColor3f(0.45f, 0.28f, 0.14f); // Another shade of brown
    glVertex2f(200.0f, 30.0f); // Bottom-left
    glVertex2f(280.0f, 120.0f); // Peak
    glVertex2f(340.0f, 30.0f); // Bottom-right
    glEnd();
}

// Function to render text on the screen
void renderText(float x, float y, const char* text) {
    glRasterPos2f(x, y); // Position to start the text
    for (const char* c = text; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c); // Render each character
    }
}

// Function to draw the upper border
void drawUpperBorder() {
    // First primitive: A large rectangle at the top
    glBegin(GL_QUADS);
    glColor3f(0.6f, 0.3f, 0.2f); // Brown color
    glVertex2f(0.0f, 290.0f);
    glVertex2f(300.0f, 290.0f);
    glVertex2f(300.0f, 280.0f);
    glVertex2f(0.0f, 280.0f);
    glEnd();
    
    // Second primitive: A line below the rectangle
    glBegin(GL_LINES);
    glColor3f(1.0f, 1.0f, 1.0f); // White color
    glVertex2f(0.0f, 280.0f);
    glVertex2f(300.0f, 280.0f);
    glEnd();
    
    // Third primitive: A series of small triangles for decoration
    glBegin(GL_TRIANGLES);
    glColor3f(0.8f, 0.4f, 0.1f); // Orange color
    for (int i = 0; i < 300; i += 20) {
        glVertex2f(i, 280.0f); // Bottom-left of triangle
        glVertex2f(i + 10.0f, 270.0f); // Top point of triangle
        glVertex2f(i + 20.0f, 280.0f); // Bottom-right of triangle
    }
    glEnd();
    
    // Fourth primitive: Small hexagons along the top edge
    for (int i = 10; i < 300; i += 30) {
        glBegin(GL_POLYGON);
        glColor3f(1.0f, 0.5f, 0.0f); // Orange color
        float size = 5.0f; // Size of the hexagon
        
        // Define the vertices for a hexagon
        for (int j = 0; j < 6; ++j) {
            float angle = j * (M_PI / 3); // 60 degrees in radians
            glVertex2f(i + size * cos(angle), 295.0f + size * sin(angle));
        }
        glEnd();
    }}

void drawLowerBorder() {
    // First primitive: A large rectangle at the bottom
    glBegin(GL_QUADS);
    glColor3f(0.4f, 0.2f, 0.0f); // Dark brown color
    glVertex2f(0.0f, 5.0f);
    glVertex2f(300.0f, 5.0f);
    glVertex2f(300.0f, 0.0f);
    glVertex2f(0.0f, 0.0f);
    glEnd();

    // Second primitive: A line above the rectangle
    glBegin(GL_LINES);
    glColor3f(1.0f, 1.0f,1.0f); // White color
    glVertex2f(0.0f, 5.0f);
    glVertex2f(300.0f, 5.0f);
    glEnd();

    // Third primitive: A zig-zag pattern with lines
    glBegin(GL_LINES);
    glColor3f(0.8f, 0.5f, 0.3f); // Light brown color
    for (int i = 0; i < 300; i += 20) {
        glVertex2f(i, 5.0f);
        glVertex2f(i + 10.0f, 15.0f);
        glVertex2f(i + 10.0f, 5.0f);
        glVertex2f(i + 20.0f, 15.0f);
    }
    glEnd();

    // Fourth primitive: Small triangles along the bottom
    glBegin(GL_TRIANGLES);
    glColor3f(0.5f, 0.3f, 0.1f); // Dark orange color
    for (int i = 0; i < 300; i += 20) {
        glVertex2f(i, 0.0f); // Bottom-left of triangle
        glVertex2f(i + 10.0f, 10.0f); // Top point of triangle
        glVertex2f(i + 20.0f, 0.0f); // Bottom-right of triangle
    }
    glEnd();

    // Fifth primitive: Small hexagons along the bottom
    for (int i = 10; i < 300; i += 30) {
        glBegin(GL_POLYGON);
        glColor3f(1.0f, 0.5f, 0.0f); // Orange color
        float size = 5.0f; // Size of the hexagon

        // Define the vertices for a hexagon
        for (int j = 0; j < 6; ++j) {
            float angle = j * (M_PI / 3); // 60 degrees in radians
            glVertex2f(i + size * cos(angle), size + size * sin(angle)); // Adjusting y to place hexagon above the bottom
        }
        glEnd();
    }
}

void renderGameEnd() {
    glColor3f(1.0f, 0.0f, 0.0f); // Red color for Game End text
    glRasterPos2f(120.0f, 150.0f); // Position the text on the screen

    const char* gameEndText = "GAME END"; // The Game End text
    for (const char* c = gameEndText; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c); // Render each character
    }
    Mix_PlayChannel(-1, gameWon, 0);
}
void drawPowerUp(const PowerUp& powerUp) {
    static float pulseFactor = 1.0f; // Pulsating factor for size
    static bool growing = true; // Track if it's growing or shrinking

    // Update pulsating size
    if (growing) {
        pulseFactor += 0.01f;
        if (pulseFactor >= 1.2f) growing = false; // Switch to shrinking
    } else {
        pulseFactor -= 0.01f;
        if (pulseFactor <= 0.8f) growing = true; // Switch to growing
    }

    if (powerUp.type == 1) { // Type 1 Power-Up (e.g., green circle)
        // 1. Draw the pulsating circle part
        glBegin(GL_TRIANGLE_FAN);
        glColor3f(0.0f, 1.0f, 0.0f); // Green color
        glVertex2f(powerUp.x, powerUp.y); // Center of the circle
        for (int i = 0; i <= 360; i += 10) {
            float angle = i * M_PI / 180.0f;
            glVertex2f(powerUp.x + (powerUp.size / 2) * pulseFactor * cos(angle),
                       powerUp.y + (powerUp.size / 2) * pulseFactor * sin(angle));
        }
        glEnd();

        // 2. Draw the outline of the circle
        glBegin(GL_LINE_LOOP);
        glColor3f(0.0f, 0.5f, 0.0f); // Darker green for outline
        for (int i = 0; i <= 360; i += 10) {
            float angle = i * M_PI / 180.0f;
            glVertex2f(powerUp.x + (powerUp.size / 2) * pulseFactor * cos(angle),
                       powerUp.y + (powerUp.size / 2) * pulseFactor * sin(angle));
        }
        glEnd();

        // 3. Draw a triangle above the circle (keep same size, no pulsation)
        glBegin(GL_TRIANGLES);
        glColor3f(0.0f, 1.0f, 0.5f); // Light green color
        glVertex2f(powerUp.x, powerUp.y + (powerUp.size / 2) + 10); // Top point
        glVertex2f(powerUp.x - 5, powerUp.y + (powerUp.size / 2)); // Bottom-left
        glVertex2f(powerUp.x + 5, powerUp.y + (powerUp.size / 2)); // Bottom-right
        glEnd();

        // 4. Draw a line across the circle (no pulsation)
        glBegin(GL_LINES);
        glColor3f(0.0f, 0.3f, 0.0f); // Dark green color
        glVertex2f(powerUp.x - (powerUp.size / 2), powerUp.y);
        glVertex2f(powerUp.x + (powerUp.size / 2), powerUp.y);
        glEnd();
        // 5. Draw a small square inside the circle (no pulsation)
        glBegin(GL_QUADS);
        glColor3f(0.5f, 1.0f, 0.0f); // Light green color for the square
        glVertex2f(powerUp.x - 5, powerUp.y - 5); // Bottom-left
        glVertex2f(powerUp.x + 5, powerUp.y - 5); // Bottom-right
        glVertex2f(powerUp.x + 5, powerUp.y + 5); // Top-right
        glVertex2f(powerUp.x - 5, powerUp.y + 5); // Top-left
        glEnd();


    } else if (powerUp.type == 2) { // Type 2 Power-Up (e.g., red square)
        // 1. Draw the pulsating square part
        glBegin(GL_QUADS);
        glColor3f(1.0f, 0.0f, 0.0f); // Red color
        glVertex2f(powerUp.x - (powerUp.size / 2) * pulseFactor, powerUp.y - (powerUp.size / 2) * pulseFactor);
        glVertex2f(powerUp.x + (powerUp.size / 2) * pulseFactor, powerUp.y - (powerUp.size / 2) * pulseFactor);
        glVertex2f(powerUp.x + (powerUp.size / 2) * pulseFactor, powerUp.y + (powerUp.size / 2) * pulseFactor);
        glVertex2f(powerUp.x - (powerUp.size / 2) * pulseFactor, powerUp.y + (powerUp.size / 2) * pulseFactor);
        glEnd();

        // 2. Draw the outline of the square
        glBegin(GL_LINE_LOOP);
        glColor3f(0.5f, 0.0f, 0.0f); // Darker red for outline
        glVertex2f(powerUp.x - (powerUp.size / 2) * pulseFactor, powerUp.y - (powerUp.size / 2) * pulseFactor);
        glVertex2f(powerUp.x + (powerUp.size / 2) * pulseFactor, powerUp.y - (powerUp.size / 2) * pulseFactor);
        glVertex2f(powerUp.x + (powerUp.size / 2) * pulseFactor, powerUp.y + (powerUp.size / 2) * pulseFactor);
        glVertex2f(powerUp.x - (powerUp.size / 2) * pulseFactor, powerUp.y + (powerUp.size / 2) * pulseFactor);
        glEnd();

        // 3. Draw a triangle above the square (keep same size, no pulsation)
        glBegin(GL_TRIANGLES);
        glColor3f(1.0f, 0.5f, 0.5f); // Light red color
        glVertex2f(powerUp.x, powerUp.y + (powerUp.size / 2) + 10); // Top point
        glVertex2f(powerUp.x - 5, powerUp.y + (powerUp.size / 2)); // Bottom-left
        glVertex2f(powerUp.x + 5, powerUp.y + (powerUp.size / 2)); // Bottom-right
        glEnd();

        // 4. Draw a line across the square (no pulsation)
        glBegin(GL_LINES);
        glColor3f(0.7f, 0.0f, 0.0f); // Darker red color
        glVertex2f(powerUp.x - (powerUp.size / 2), powerUp.y);
        glVertex2f(powerUp.x + (powerUp.size / 2), powerUp.y);
        glEnd();
    }
}



PowerUp generateNewPowerUp() {
    PowerUp powerUp;
    powerUp.size = COLLECTABLE_SIZE; // Use the same size for simplicity
    powerUp.isActive = true;          // Initially active
    powerUp.timer = 0.0f;             // Reset the timer
    powerUp.duration = 5.0f;          // Set duration for the power-up effect

    // Randomize type (1 for speed boost, 2 for invulnerability)
    powerUp.type = (rand() % 2) + 1; // Generate either 1 or 2

    // Regenerate position if overlapping with obstacles
    do {
        powerUp.x = 300 + rand() % 100; // Random X position to the right
        powerUp.y = groundLevel + 10;   // Place power-up on the ground
    } while (isOverlappingWithObstacle(powerUp)|| isOverlappingWithCollectable(powerUp)); // Ensure no overlap with obstacles

    return powerUp;
}

bool isPlayerCollidingWithPowerUp(const PowerUp& powerUp) {
    bool isCollidingX = (player.x + player.width / 2 >= powerUp.x - powerUp.size / 2) &&
                        (player.x - player.width / 2 <= powerUp.x + powerUp.size / 2);
    bool isCollidingY = (player.y <= powerUp.y + powerUp.size / 2) &&
                        (player.y + player.height >= powerUp.y - powerUp.size / 2);

    return isCollidingX && isCollidingY;
}


void applyPowerUpEffect(PowerUp& powerUp) {
    if (powerUp.type == 1) { // Speed boost
        obstacleSpeed *= 0.5f; // Slow down the game
        powerUp.duration = 5.0f; // Set duration for the speed boost effect
    } else if (powerUp.type == 2) { // Invulnerability
        isInvulnerable = true;
        invulnerabilityTimer = 0.0f; // Reset the invulnerability timer
        powerUp.duration = 5.0f; // Set duration for invulnerability effect
    }

    // Activate the power-up
    powerUp.isActive = true;
    powerUp.timer = 0.0f; // Start the timer
}


void generatePowerUps() {
    while (powerUps.size() < 2) { // Ensure at least 2 power-ups are displayed
        powerUps.push_back(generateNewPowerUp());
    }
}


void updatePowerUps(float deltaTime) {
    powerUpSpawnTimer += deltaTime; // Increment spawn timer

    // Generate a new power-up if the timer exceeds the interval and if the game is not over
    if (powerUpSpawnTimer >= powerUpSpawnInterval && !gameOver && !gameEnd) {
        powerUps.push_back(generateNewPowerUp());
        powerUpSpawnTimer = 0.0f; // Reset the spawn timer
    }

    for (auto it = powerUps.begin(); it != powerUps.end(); ) {
        // Move the power-up to the left (simulate movement)
        it->x -= obstacleSpeed; // or any specific speed for the power-up

        // Update the timer
        it->timer += deltaTime;

        // Check if the power-up duration has expired
        if (it->isActive && it->timer >= it->duration) {
            if (it->type == 1) { // If it was the speed boost
                obstacleSpeed /= 0.5f; // Restore original speed
            } else if (it->type == 2) { // If it was the invulnerability
                isInvulnerable = false; // End invulnerability
            }

            // Deactivate the power-up
            it->isActive = false;
        }

        // Check for collision with player
        if (isPlayerCollidingWithPowerUp(*it)) {
            // Apply the effect of the power-up
            applyPowerUpEffect(*it);
            Mix_PlayChannel(-1, collectSound, 0); // Play collect sound


            // Remove the specific power-up
            it = powerUps.erase(it);
            continue; // Move to the next power-up
        }

        // Remove the power-up if it goes off-screen
        if (it->y < -it->size) {
            it = powerUps.erase(it); // Remove the power-up
        } else {
            ++it; // Move to the next power-up
        }
    }
}

void updateInvulnerability(float deltaTime) {
    if (isInvulnerable) {
        invulnerabilityTimer += deltaTime; // Increment the timer

        // Check if invulnerability duration has expired
        if (invulnerabilityTimer >= invulnerabilityDuration) {
            isInvulnerable = false; // Disable invulnerability
            invulnerabilityTimer = 0.0f; // Reset timer
        }
    }
}



// Main display function
void Display() {
    glClear(GL_COLOR_BUFFER_BIT); // Clear the screen

    // Draw the sky in sky blue color
    glBegin(GL_QUADS);
    glColor3f(0.53f, 0.81f, 0.98f); // Sky blue color (R,G,B)
    glVertex2f(0.0f, 300.0f);       // Top-left corner of the window
    glVertex2f(300.0f, 300.0f);     // Top-right corner of the window
    glVertex2f(300.0f, 30.0f);      // Bottom-right corner (90% height)
    glVertex2f(0.0f, 30.0f);        // Bottom-left corner (90% height)
    glEnd();

    // Draw the ground in dark brown color
    glBegin(GL_QUADS);
    glColor3f(0.4f, 0.2f, 0.0f); // Dark brown color for the ground
    glVertex2f(0.0f, 30.0f);      // Bottom-left corner
    glVertex2f(300.0f, 30.0f);    // Bottom-right corner
    glVertex2f(300.0f, 0.0f);     // Very bottom-right corner
    glVertex2f(0.0f, 0.0f);       // Very bottom-left corner
    glEnd();

    // Reset the color to white (for other objects, like the player)
    glColor3f(1.0f, 1.0f, 1.0f);

    drawUpperBorder(); // Draw upper border
    drawLowerBorder(); // Draw lower border
    
    for (Cloud& cloud : clouds) {
            drawCloud(cloud.x, cloud.y, cloud.scale); // Draw each cloud
        }

    drawMountains();
    drawPlayer();
    drawHealthBar();
    drawSun();
    // Render the timer
    char timerText[20]; // Buffer to hold the timer string
    sprintf(timerText, "Time: %d", totalTime); // Convert the timer to a string
    renderText(10.0f, 230.0f, timerText);   // Display it below the score

    char scoreText[20]; // Buffer to hold the score string
    sprintf(scoreText, "Score: %d", score); // Convert the score to a string
    renderText(10.0f, 260.0f, scoreText);   // Display it below the health bar

    
    for (const Obstacle& obs : obstacles) {
        drawObstacle(obs);
    }
    for (const Collectable& col : collectables) {
            drawCollectable(col); // Draw each collectable
    }
    for (const PowerUp& powerUp : powerUps) {
            drawPowerUp(powerUp); // Draw each power-up
        }
    if (gameOver) {
            renderGameOver(); // Call the function to render "Game Over"
        }
    if(gameEnd){
        renderGameEnd();
    }


    glFlush(); // Render everything
}


// Main function
int main(int argc, char** argv) {
    SDL_Init(SDL_INIT_AUDIO);
    init();
    srand(time(0)); // Seed the random number generator
    glutInit(&argc, argv);
    glutInitWindowSize(500, 500);
    glutInitWindowPosition(150, 150);
    glutCreateWindow("Graphics 2D game");

    // Player initial setup
    player.x = 50; // Initial X position
    player.y = groundLevel; // Initial Y position (ground level)
    player.width = 15; // Width of the player
    player.height = 20; // Height of the player
    
    Mix_PlayMusic(backgroundMusic, -1);
    
    glEnable(GL_COLOR_MATERIAL);


    // Register callback functions
    glutDisplayFunc(Display);
    glutIdleFunc([]() {
            static float lastTime = glutGet(GLUT_ELAPSED_TIME) / 1000.0f; // Get current time in seconds
            float currentTime = glutGet(GLUT_ELAPSED_TIME) / 1000.0f; // Get current time
            float deltaTime = currentTime - lastTime; // Calculate the time difference
            lastTime = currentTime; // Update lastTime to current

            // Call your update functions
            updateClouds();
            updateObstacles(); // Update obstacles continuously
            updatePlayer(); // Update player position
            updateCollectables();
            updatePowerUps(deltaTime);
            updateTimer(deltaTime); // Update timer with the time difference
            updateInvulnerability(deltaTime);
            glutPostRedisplay();
    });
    glutSpecialUpFunc(specialKeyboardUp);
    glutSpecialFunc(specialKeyboard); // Register the special keyboard callback
    //glutKeyboardFunc(keyUp); // Register key up callback
    
    generateClouds();

    // Generate initial obstacles
    generateObstacles(); // Call to generate obstacles
    collectables.push_back(generateNewCollectable());
    // Generate initial power-ups
    powerUps.push_back(generateNewPowerUp());



    // Set OpenGL display mode
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f); // Black background if needed
    gluOrtho2D(0.0, 300, 0.0, 300);

    glutMainLoop(); // Enter the event-processing loop
    
    Mix_FreeMusic(backgroundMusic);
    Mix_FreeChunk(collectSound);
    Mix_FreeChunk(collideSound);
    Mix_FreeChunk(gameWon);
    Mix_FreeChunk(gameLost);
    Mix_CloseAudio();
    SDL_Quit();
        
    return 0; // Return success
}
