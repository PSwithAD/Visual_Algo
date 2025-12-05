# run_animation.py
import sys
import numpy as np
import matplotlib.pyplot as plt
import subprocess
import os

CPP_EXCUTABLE = "./ino_code/string_matching_kmp"

# --- This part is mostly from 11_13.py ---
W, H = 16, 16
current_frame = np.zeros((H, W, 3), dtype=np.uint8)

def parse_hex_color(hex_str: str):
    """RRGGBB 형식의 16진 문자열을 (r, g, b)로 변환"""
    hex_str = hex_str.strip()
    if len(hex_str) != 6:
        return (0, 0, 0)
    try:
        r = int(hex_str[0:2], 16)
        g = int(hex_str[2:4], 16)
        b = int(hex_str[4:6], 16)
        return (r, g, b)
    except ValueError:
        return (0, 0, 0)

def read_line(fin):
    """A stream (like subprocess.stdout)에서 한 줄 읽기"""
    line = fin.readline()
    if not line:
        return None
    # Popen's stdout is in bytes, so we need to decode
    return line.decode("utf-8", errors="ignore").strip()

def read_frame(fin):
    """FRAME 한 덩어리 읽기"""
    while True:
        line = read_line(fin)
        if line is None:
            return False  # EOF
        if line.startswith("FRAME:"):
            frame_num = line.split(":", 1)[1].strip()
            print(f"\rFRAME {frame_num}", end="", file=sys.stderr, flush=True)
            break

    for y in range(H):
        line = read_line(fin)
        if line is None:
            return False
        hex_colors = line.split()
        for x in range(min(len(hex_colors), W)):
            r, g, b = parse_hex_color(hex_colors[x])
            current_frame[y, x] = [r, g, b]

    _ = read_line(fin) # Read "---"
    return True

def main():
    print("============================================================")
    print(" LED Matrix Visualizer (Subprocess runner)")
    print("============================================================")

    cpp_executable = CPP_EXCUTABLE
    if not os.path.exists(cpp_executable):
        print(f"Error: Executable not found at {cpp_executable}")
        print("Please compile the C++ code first.")
        return

    # Start the C++ process
    process = subprocess.Popen([cpp_executable], stdout=subprocess.PIPE, stderr=subprocess.PIPE) 
    
    # --- Visualization part ---
    plt.ion()
    fig, ax = plt.subplots(figsize=(6, 6))
    im = ax.imshow(current_frame, interpolation="nearest", vmin=0, vmax=255)
    ax.set_title("LED Matrix")
    ax.axis("off")
    plt.show(block=False)

    try:
        while True:
            # Pass the process's stdout stream to read_frame
            ok = read_frame(process.stdout)
            if not ok:
                break
            im.set_data(current_frame)
            fig.canvas.draw()
            plt.pause(0.001)
    except KeyboardInterrupt:
        print("\nInterrupted by user.")
    finally:
        process.kill() # Ensure the C++ process is terminated
        print("\nC++ process terminated.", file=sys.stderr) 
        
    print("\nVisualization finished.", file=sys.stderr)

if __name__ == "__main__":
    main()
