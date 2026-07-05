import tkinter as tk
from tkinter import filedialog, simpledialog
import csv

def main():
    root = tk.Tk()
    root.withdraw() # Hide main window

    print("Select your 320x240 map.jpg file...")
    file_path = filedialog.askopenfilename(title="Select Map Image", filetypes=[("Image files", "*.jpg *.png *.bmp")])
    if not file_path:
        print("No file selected.")
        return

    csv_path = "countries.csv"
    print(f"Coordinates will be saved to {csv_path}")

    window = tk.Toplevel(root)
    window.title("Atlas Map Clicker - Click on a country!")

    try:
        img = tk.PhotoImage(file=file_path)
    except Exception as e:
        print("Error loading image (Make sure it's a format Tkinter supports like PNG/GIF or use PIL for JPG).")
        print(e)
        return

    canvas = tk.Canvas(window, width=img.width(), height=img.height())
    canvas.pack()
    canvas.create_image(0, 0, anchor=tk.NW, image=img)

    def on_click(event):
        x, y = event.x, event.y
        country = simpledialog.askstring("Input", "Country Name:", parent=window)
        if not country: return
        capital = simpledialog.askstring("Input", "Capital:", parent=window)
        population = simpledialog.askstring("Input", "Population:", parent=window)
        currency = simpledialog.askstring("Input", "Currency:", parent=window)
        timezone = simpledialog.askinteger("Input", "Timezone Offset (e.g., -5 for EST):", parent=window)
        
        with open(csv_path, 'a', newline='') as f:
            writer = csv.writer(f)
            writer.writerow([country, capital, population, currency, timezone, x, y])
            
        print(f"Saved: {country} at ({x}, {y})")

    canvas.bind("<Button-1>", on_click)
    print("Click on the map to add a country. Close the window when done.")
    window.mainloop()

if __name__ == "__main__":
    main()
