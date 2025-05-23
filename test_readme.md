# Testing Width/Height Configuration Feature

## Overview
This document describes how to test the new width and height configuration feature added to the Vonsh game options menu.

## Test Cases

### 1. Basic Navigation
1. Start the game
2. Select "Options" from the main menu
3. Navigate to the "Width:" and "Height:" menu items using arrow keys
4. Verify that the current values are displayed (default: Width: 56, Height: 41)

### 2. Width Configuration (Windowed Mode)
1. In the Options menu, select the "Width:" item and press Enter/Space
2. The display should show "Width: _" indicating input mode
3. Type a new width value (e.g., "80")
4. Press Enter to confirm
5. The window should immediately resize to the new width
6. The value should be retained when returning to the Options menu

### 3. Height Configuration (Windowed Mode)
1. In the Options menu, select the "Height:" item and press Enter/Space
2. The display should show "Height: _" indicating input mode
3. Type a new height value (e.g., "50")
4. Press Enter to confirm
5. The window should immediately resize to the new height
6. The value should be retained when returning to the Options menu

### 4. Input Validation
1. Try entering values below the minimum (Width < 30, Height < 20)
2. Press Enter - the previous value should be retained
3. Try entering non-numeric characters - they should be ignored
4. Try entering more than 3 digits - only first 3 should be accepted

### 5. Fullscreen Mode Behavior
1. Switch to fullscreen mode using the "Display: window" option
2. Navigate to Width/Height options
3. They should appear in grey text
4. The displayed values should show the actual fullscreen dimensions
5. Pressing Enter/Space on these items should have no effect
6. Switch back to windowed mode - previous windowed values should be restored

### 6. Input Controls
- **0-9**: Add digit to input
- **Keypad 0-9**: Add digit to input
- **Backspace**: Remove last digit
- **Enter**: Confirm value
- **Escape**: Cancel input and return to Options menu

### 7. Mouse Support
1. Click on Width/Height items with the mouse
2. Should enter configuration mode same as keyboard
3. In fullscreen mode, clicking should have no effect

## Expected Behavior Summary
- Width minimum: 30 tiles
- Height minimum: 20 tiles  
- Maximum: 999 tiles (3-digit limit)
- Immediate window resize in windowed mode
- Values preserved when switching between fullscreen/windowed
- Fullscreen mode shows actual dimensions but prevents editing 