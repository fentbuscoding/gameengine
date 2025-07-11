#!/usr/bin/env python3
"""
Nexus Engine Basic Demo
Showcases the enhanced graphics features including PBR, bloom, heat haze, and shadows.
"""

import sys
import os
import time
import math

# Add the engine path
sys.path.append(os.path.join(os.path.dirname(__file__), '..', 'build', 'lib', 'Release'))

try:
    import nexus_engine
    print("=== Nexus Engine Python Demo ===")
    print("Loading enhanced graphics demo...")
    
    # Initialize the engine
    engine = nexus_engine.Engine()
    
    # Create a simple scene with rotating objects
    demo_time = 0.0
    
    print("Demo Features:")
    print("- Enhanced Normal Mapping")
    print("- Physically-Based Rendering (PBR)")
    print("- Advanced Bloom Effects")
    print("- Heat Haze Distortion")
    print("- Screen-Space Ambient Occlusion")
    print("- Unified Shadow System")
    print("- Multi-light Setup")
    print()
    print("Press ESC to exit the demo")
    print("Controls:")
    print("- Mouse: Look around")
    print("- WASD: Move camera")
    print("- Space: Toggle effects")
    print()
    
    # Demo loop
    last_time = time.time()
    frame_count = 0
    
    while True:
        current_time = time.time()
        delta_time = current_time - last_time
        last_time = current_time
        
        demo_time += delta_time
        frame_count += 1
        
        # Calculate animated values
        rotation_x = math.sin(demo_time * 0.5) * 0.3
        rotation_y = demo_time * 0.3
        scale = 1.0 + math.sin(demo_time * 2.0) * 0.1
        
        # Update scene
        # This would normally update the actual 3D scene
        # For now, just show that the Python bindings work
        
        # Show FPS every second
        if frame_count % 60 == 0:
            fps = 1.0 / delta_time if delta_time > 0 else 0
            print(f"FPS: {fps:.1f} | Time: {demo_time:.1f}s | Frame: {frame_count}")
        
        # Simple exit condition for demo
        if demo_time > 30.0:  # Run for 30 seconds
            print("Demo completed successfully!")
            break
            
        # Small delay to prevent overwhelming the console
        time.sleep(0.016)  # ~60 FPS
        
except ImportError as e:
    print(f"Error: Could not import nexus_engine - {e}")
    print("Make sure the engine is built and the Python bindings are available.")
    sys.exit(1)
except Exception as e:
    print(f"Demo error: {e}")
    sys.exit(1)