void drawTransparentBitmap(
  GFXcanvas16 &canvas, 
  int16_t x, int16_t y,
  const uint16_t *bitmap, 
  int16_t w, int16_t h,
  uint16_t transparentColor = 0xFFFF // blanco por defecto
) {
  for (int16_t j = 0; j < h; j++) {
    for (int16_t i = 0; i < w; i++) {
      uint16_t color = bitmap[j * w + i];
      if (color != transparentColor) {
        canvas.drawPixel(x + i, y + j, color);
      }
    }
  }
}