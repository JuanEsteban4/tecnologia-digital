import os
from PIL import Image

# Carpeta de entrada y archivo de salida sprites
INPUT_FOLDER = "res/sprites"
OUTPUT_FILE = "main/sprites.h"

def rgb888_to_rgb565(r, g, b):
    """Convierte un color RGB888 a RGB565 (16 bits)."""
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)

def image_to_array(image_path):
    """Convierte una imagen en una lista de valores RGB565."""
    img = Image.open(image_path).convert('RGB')
    pixels = []
    for y in range(img.height):
        for x in range(img.width):
            r, g, b = img.getpixel((x, y))
            pixels.append(rgb888_to_rgb565(r, g, b))
    return pixels, img.width, img.height

def sanitize_name(filename):
    """Convierte el nombre del archivo en un nombre de variable válido en C++."""
    name = os.path.splitext(os.path.basename(filename))[0]
    name = name.replace(" ", "_").replace("-", "_")
    return name

def main():
    files = [f for f in os.listdir(INPUT_FOLDER)
             if f.lower().endswith((".png", ".jpg", ".bmp"))]
    
    if not files:
        print("No se encontraron imágenes en la carpeta:", INPUT_FOLDER)
        return

    sprite_names = []

    with open(OUTPUT_FILE, "w", encoding="utf-8") as out:
        out.write("// Archivo generado automáticamente\n")
        out.write("// Contiene sprites en formato RGB565 (16 bits)\n\n")
        out.write("#pragma once\n\n")
        out.write("#include <stdint.h>\n\n")

        for file in files:
            path = os.path.join(INPUT_FOLDER, file)
            name = sanitize_name(file)
            sprite_names.append(name)
            pixels, w, h = image_to_array(path)
            
            out.write(f"// Sprite: {file} ({w}x{h})\n")
            out.write(f"const uint16_t {name}[] = {{\n")
            
            # Escribe los valores en formato hexadecimal
            for i, val in enumerate(pixels):
                out.write(f"0x{val:04X}, ")
                if (i + 1) % w == 0:
                    out.write("\n")
            out.write("};\n\n")

            # Dimensiones
            out.write(f"const uint16_t {name}_width = {w};\n")
            out.write(f"const uint16_t {name}_height = {h};\n\n")

        # --- Arreglo maestro con referencias a todos los sprites ---
        out.write("// --- Referencias a todos los sprites ---\n")
        out.write("const uint16_t* const all_sprites[] = {\n")
        for name in sprite_names:
            out.write(f"    {name},\n")
        out.write("};\n\n")

        out.write(f"const uint16_t NUM_SPRITES = {len(sprite_names)};\n")

    print(f" Archivo '{OUTPUT_FILE}' generado con {len(files)} sprites")

if __name__ == "__main__":
    main()
