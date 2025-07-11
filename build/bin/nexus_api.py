"""
Nexus Game Engine Python API

This module provides high-level Python interfaces for the Nexus Game Engine.
Use this for creating games and interactive applications.
"""

import nexus_engine as engine_core
import time
import os

class GameObject:
    """Base class for all game objects"""
    
    def __init__(self, name="GameObject"):
        self.name = name
        self.position = [0.0, 0.0, 0.0]
        self.rotation = [0.0, 0.0, 0.0]
        self.scale = [1.0, 1.0, 1.0]
        self.active = True
        
    def update(self, delta_time):
        """Override this method to add custom update logic"""
        pass
        
    def render(self, graphics):
        """Override this method to add custom rendering"""
        pass

class Scene:
    """Scene management for game objects"""
    
    def __init__(self, name="Scene"):
        self.name = name
        self.objects = []
        
    def add_object(self, obj):
        """Add a game object to the scene"""
        self.objects.append(obj)
        
    def remove_object(self, obj):
        """Remove a game object from the scene"""
        if obj in self.objects:
            self.objects.remove(obj)
            
    def update(self, delta_time):
        """Update all active objects in the scene"""
        for obj in self.objects:
            if obj.active:
                obj.update(delta_time)
                
    def render(self, graphics):
        """Render all active objects in the scene"""
        for obj in self.objects:
            if obj.active:
                obj.render(graphics)

class GameApplication:
    """Main application class for Nexus games"""
    
    def __init__(self, title="Nexus Game", width=1280, height=720):
        self.title = title
        self.width = width
        self.height = height
        self.engine = engine_core.Engine()
        self.current_scene = None
        self.running = False
        
    def initialize(self):
        """Initialize the game application"""
        if not self.engine.initialize():
            raise RuntimeError("Failed to initialize Nexus Engine")
            
        # Setup graphics features
        graphics = self.engine.get_graphics()
        graphics.enable_bloom(True)
        graphics.enable_shadows(True)
        graphics.set_bloom_threshold(0.8)
        graphics.set_bloom_intensity(1.2)
        
        self.running = True
        return True
        
    def run(self):
        """Run the main game loop"""
        if not self.running:
            if not self.initialize():
                return False
                
        print(f"Starting {self.title}...")
        
        # Main loop
        while self.engine.is_running() and self.running:
            delta_time = self.engine.get_delta_time()
            
            # Update current scene
            if self.current_scene:
                self.current_scene.update(delta_time)
                
            # Custom update logic
            self.update(delta_time)
            
            # Render current scene
            if self.current_scene:
                graphics = self.engine.get_graphics()
                self.current_scene.render(graphics)
                
            # Custom render logic
            self.render()
            
            # Small sleep to prevent 100% CPU usage
            time.sleep(0.001)
            
        self.shutdown()
        return True
        
    def shutdown(self):
        """Shutdown the game application"""
        self.running = False
        self.engine.shutdown()
        print("Game shutdown complete")
        
    def set_scene(self, scene):
        """Set the current active scene"""
        self.current_scene = scene
        
    def update(self, delta_time):
        """Override this method for custom game logic"""
        pass
        
    def render(self):
        """Override this method for custom rendering"""
        pass

# Graphics utilities
class GraphicsUtils:
    """Utility functions for graphics operations"""
    
    @staticmethod
    def enable_enhanced_graphics(graphics):
        """Enable all enhanced graphics features"""
        graphics.enable_bloom(True)
        graphics.enable_heat_haze(True)
        graphics.enable_shadows(True)
        graphics.set_bloom_threshold(0.7)
        graphics.set_bloom_intensity(1.5)
        graphics.set_shadow_map_size(2048)
        print("Enhanced graphics features enabled")
        
    @staticmethod
    def set_performance_mode(graphics):
        """Set graphics to performance mode"""
        graphics.enable_bloom(False)
        graphics.enable_heat_haze(False)
        graphics.enable_shadows(True)  # Keep shadows for gameplay
        graphics.set_shadow_map_size(512)
        print("Performance mode enabled")

# Example usage
if __name__ == "__main__":
    class TestGame(GameApplication):
        def __init__(self):
            super().__init__("Nexus Test Game")
            
        def initialize(self):
            if not super().initialize():
                return False
                
            # Enable enhanced graphics
            GraphicsUtils.enable_enhanced_graphics(self.engine.get_graphics())
            
            # Create a test scene
            scene = Scene("TestScene")
            
            # Add some test objects
            cube = GameObject("TestCube")
            scene.add_object(cube)
            
            self.set_scene(scene)
            return True
            
        def update(self, delta_time):
            # Print FPS every second
            if int(time.time()) % 2 == 0:
                fps = self.engine.get_fps()
                if fps > 0:
                    print(f"FPS: {fps:.1f}")
    
    # Create and run the test game
    game = TestGame()
    game.run()
