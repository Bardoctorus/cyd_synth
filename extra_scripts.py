#
# PlatformIO extra script to copy User_Setup.h to TFT_eSPI library
# This ensures TFT_eSPI uses our custom configuration
#

Import("env")

import os
import shutil
from pathlib import Path

def copy_user_setup(*args, **kwargs):
    """
    Copy User_Setup.h from project include/ to TFT_eSPI library directory
    This runs after libraries are installed but before compilation
    """
    project_dir = Path(env["PROJECT_DIR"])
    libdeps_dir = project_dir / ".pio" / "libdeps" / env["PIOENV"]
    
    # Check if libdeps directory exists (libraries must be installed first)
    if not libdeps_dir.exists():
        print("Warning: Library dependencies not installed yet. Run 'pio lib install' first.")
        return
    
    # Find TFT_eSPI library directory
    tft_espi_dirs = list(libdeps_dir.glob("TFT_eSPI*"))
    
    if not tft_espi_dirs:
        print("Warning: TFT_eSPI library not found in libdeps.")
        print("         Libraries will be installed on first build.")
        return
    
    tft_espi_dir = tft_espi_dirs[0]
    user_setup_src = project_dir / "include" / "User_Setup.h"
    user_setup_dst = tft_espi_dir / "User_Setup.h"
    
    if not user_setup_src.exists():
        print(f"Warning: {user_setup_src} not found!")
        return
    
    # Copy the file
    try:
        print(f"Copying User_Setup.h to TFT_eSPI library...")
        shutil.copy2(user_setup_src, user_setup_dst)
        print(f"  Source: {user_setup_src}")
        print(f"  Dest:   {user_setup_dst}")
        print("  User_Setup.h copied successfully!")
    except Exception as e:
        print(f"Error copying User_Setup.h: {e}")

# Register the copy function to run before building
# Using "$BUILD_DIR" target ensures libraries are installed first
env.AddPreAction("$BUILD_DIR", copy_user_setup)

