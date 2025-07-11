"""
Nexus Engine Python Game - 2D Platformer
Demonstrates Python game development with the Nexus Engine
"""

import nexus_engine as engine
import math
import time

class Player:
    def __init__(self):
        self.position = engine.Vector3(0, 0, 0)
        self.velocity = engine.Vector3(0, 0, 0)
        self.size = engine.Vector3(1, 1, 1)
        self.on_ground = False
        self.speed = 8.0
        self.jump_force = 12.0
        
    def update(self, delta_time, input_manager):
        # Horizontal movement
        if input_manager.is_key_down(ord('A')):
            self.velocity.x = -self.speed
        elif input_manager.is_key_down(ord('D')):
            self.velocity.x = self.speed
        else:
            self.velocity.x *= 0.8  # Friction
            
        # Jumping
        if input_manager.is_key_pressed(ord(' ')) and self.on_ground:
            self.velocity.y = self.jump_force
            self.on_ground = False
            
        # Gravity
        self.velocity.y -= 25.0 * delta_time
        
        # Update position
        self.position.x += self.velocity.x * delta_time
        self.position.y += self.velocity.y * delta_time
        
        # Ground collision
        if self.position.y <= 0:
            self.position.y = 0
            self.velocity.y = 0
            self.on_ground = True
            
    def render(self, graphics):
        # Draw player as orange cube
        graphics.draw_cube(self.position, self.size, (1.0, 0.5, 0.0, 1.0))

class Platform:
    def __init__(self, x, y, z, width, height, depth):
        self.position = engine.Vector3(x, y, z)
        self.size = engine.Vector3(width, height, depth)
        
    def render(self, graphics):
        graphics.draw_cube(self.position, self.size, (0.5, 0.5, 0.5, 1.0))

class PlatformerGame:
    def __init__(self):
        self.player = Player()
        self.platforms = []
        self.camera_offset = engine.Vector3(0, 5, -10)
        self.score = 0
        self.game_time = 0
        
        # Create level
        self.create_level()
        
    def create_level(self):
        # Ground platforms
        for i in range(-20, 21, 4):
            self.platforms.append(Platform(i, -2, 0, 4, 1, 4))
            
        # Floating platforms
        self.platforms.append(Platform(8, 3, 0, 4, 0.5, 4))
        self.platforms.append(Platform(16, 6, 0, 4, 0.5, 4))
        self.platforms.append(Platform(-8, 4, 0, 4, 0.5, 4))
        self.platforms.append(Platform(-16, 7, 0, 4, 0.5, 4))
        
    def update(self, delta_time):
        self.game_time += delta_time
        
        # Get input
        input_manager = engine.get_input()
        
        # Update player
        self.player.update(delta_time, input_manager)
        
        # Update score based on height
        height_score = max(0, int(self.player.position.y * 10))
        self.score = max(self.score, height_score)
        
        # Check if player fell
        if self.player.position.y < -10:
            self.player.position = engine.Vector3(0, 0, 0)
            self.player.velocity = engine.Vector3(0, 0, 0)
            
    def render(self):
        graphics = engine.get_graphics()
        
        # Clear screen
        graphics.clear(0.4, 0.6, 1.0, 1.0)  # Sky blue
        
        # Render platforms
        for platform in self.platforms:
            platform.render(graphics)
            
        # Render player
        self.player.render(graphics)
        
        # Render UI
        self.render_ui(graphics)
        
    def render_ui(self, graphics):
        # Score
        graphics.draw_text(f"Score: {self.score}", 10, 10, (1, 1, 1, 1))
        
        # Time
        graphics.draw_text(f"Time: {self.game_time:.1f}s", 10, 30, (1, 1, 1, 1))
        
        # Controls
        graphics.draw_text("A/D: Move, Space: Jump", 10, 50, (1, 1, 1, 1))
        
        # Player position
        pos = self.player.position
        graphics.draw_text(f"Position: ({pos.x:.1f}, {pos.y:.1f})", 10, 70, (1, 1, 1, 1))

def main():
    print("Starting Python Platformer Game...")
    
    # Initialize engine
    game_engine = engine.Engine()
    if not game_engine.initialize():
        print("Failed to initialize engine!")
        return
        
    print("Engine initialized successfully!")
    
    # Create game
    game = PlatformerGame()
    
    print("Game created! Use A/D to move, Space to jump, ESC to quit.")
    
    # Game loop
    clock = time.time()
    
    try:
        while game_engine.is_running():
            # Calculate delta time
            current_time = time.time()
            delta_time = current_time - clock
            clock = current_time
            
            # Update game
            game.update(delta_time)
            
            # Render game
            graphics = game_engine.get_graphics()
            graphics.begin_frame()
            game.render()
            graphics.end_frame()
            graphics.present()
            
            # Limit FPS
            time.sleep(max(0, 1/60 - delta_time))
            
    except KeyboardInterrupt:
        print("Game interrupted by user")
    
    # Cleanup
    game_engine.shutdown()
    print("Game ended. Thanks for playing!")

if __name__ == "__main__":
    main()