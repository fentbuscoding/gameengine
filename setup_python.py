import sys
import subprocess
import os

def install_pybind11():
    """Install pybind11 using pip"""
    print("Installing pybind11...")
    try:
        subprocess.check_call([sys.executable, "-m", "pip", "install", "pybind11[global]", "--quiet"])
        print("pybind11 installed successfully!")
        return True
    except subprocess.CalledProcessError as e:
        print(f"Failed to install pybind11: {e}")
        return False

def check_python_installation():
    """Check if Python is properly installed"""
    print(f"Python version: {sys.version}")
    print(f"Python executable: {sys.executable}")
    
    # Check if we can import necessary modules
    try:
        import pybind11
        print(f"pybind11 version: {pybind11.__version__}")
        print(f"pybind11 location: {pybind11.__file__}")
    except ImportError:
        print("pybind11 not found, installing...")
        if not install_pybind11():
            return False
        
        # Try importing again
        try:
            import pybind11
            print(f"pybind11 version: {pybind11.__version__}")
        except ImportError:
            print("Failed to import pybind11 after installation")
            return False
    
    return True

def setup_environment():
    """Setup environment variables for CMake"""
    python_root = os.path.dirname(os.path.dirname(sys.executable))
    
    print(f"Setting Python3_ROOT_DIR to: {python_root}")
    os.environ['Python3_ROOT_DIR'] = python_root
    
    # Set DirectX SDK path
    directx_path = r"C:\Users\Doggle\DirectX"
    if os.path.exists(directx_path):
        os.environ['DXSDK_DIR'] = directx_path
        print(f"Setting DXSDK_DIR to: {directx_path}")
    else:
        print(f"Warning: DirectX SDK not found at {directx_path}")
    
    # Also set PYTHONPATH to include our project
    project_root = os.path.dirname(os.path.abspath(__file__))
    python_path = os.path.join(project_root, 'python')
    
    if 'PYTHONPATH' in os.environ:
        os.environ['PYTHONPATH'] = python_path + os.pathsep + os.environ['PYTHONPATH']
    else:
        os.environ['PYTHONPATH'] = python_path
    
    print(f"PYTHONPATH set to include: {python_path}")

if __name__ == "__main__":
    print("=== Nexus Engine Python Setup ===")
    
    if not check_python_installation():
        print("Python setup failed!")
        sys.exit(1)
    
    setup_environment()
    
    print("\nSetup completed successfully!")
    print("You can now run the build script or use CMake to build the engine.")
    print("\nFor CMake, use:")
    print("  cmake -S . -B build")
    print("  cmake --build build --config Release")
    
    input("Press Enter to continue...")
