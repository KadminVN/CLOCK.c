# 🕰️ Analog Clock for Windows NT

This project is a classic **Analog Clock** application written in C using the Win32 API. It visually displays the current system time with moving hour, minute, and second hands, updating every second.

---

## ✨ Features

- **Real-Time Analog Display:** Shows the current time with smooth-moving hands.
- **Resizable Window:** The clock scales automatically to fit the window size.
- **Minimalist Design:** Only the clock face and hands are drawn for clarity.
- **Efficient Redrawing:** Uses Windows timers and painting routines for smooth updates.

---

## 🖥️ How It Works

### 1. **Window Creation**

- The program registers a window class and creates a main application window.
- The window uses the `WndProc` callback to handle messages (painting, resizing, timer, etc.).

### 2. **Timer-Driven Updates**

- A Windows timer (`SetTimer`) triggers every second.
- On each tick, the clock fetches the current system time and redraws the hands.

### 3. **Drawing the Clock**

- **Isotropic Mapping:** The drawing area is set up so the clock remains circular, regardless of window size.
- **Tick Marks:** The clock face is marked with hour and minute ticks using ellipses.
- **Hands:** The hour, minute, and second hands are drawn as polylines, rotated according to the current time.

### 4. **Redrawing Logic**

- On each timer event, the client area is cleared and the clock is redrawn.
- The `WM_PAINT` message ensures the clock is correctly rendered when the window is exposed or resized.

---

## 🛠️ How to Build & Run

1. **Requirements:**
   - Windows NT or later
   - C compiler supporting Win32 API (e.g., MSVC)

2. **Build:**
   - Open the project in your IDE or use the command line:
     ```
     cl CLOCK.c user32.lib gdi32.lib
     ```

3. **Run:**
   - Execute the generated `CLOCK.exe`.
   - The analog clock window will appear and update in real time.

---

## 📦 File Structure

```
CLOCK.c         # Main source code
README.md       # This documentation
```

---

## 📝 Code Highlights

- **SetIsotropic:** Ensures the clock face is always a perfect circle.
- **RotatePoint:** Rotates points to draw hands and ticks at correct angles.
- **DrawClock:** Draws the tick marks for hours and minutes.
- **DrawHands:** Draws the hour, minute, and second hands based on system time.

---

## 📸 Screenshot

<img width="1423" height="783" alt="image" src="https://github.com/user-attachments/assets/fc176a06-0b0e-49ce-af0c-2bdec26c138c" />


---

## 🧑‍💻 Author

- Original code adapted for educational purposes.

---

## ⚠️ Notes

- This program is designed for classic Windows environments (Windows NT and later).
- For modern Windows, it should still compile and run with minor or no changes.

---
