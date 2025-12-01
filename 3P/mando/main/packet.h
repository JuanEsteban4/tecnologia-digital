// bit 0 → Botón A  
// bit 1 → Botón B  
// bit 2 → Botón X  
// bit 3 → Botón Y  
// bit 4 → (libre)
// bit 5 → (libre)
// bit 6 → (libre)
// bit 7 → (libre)

//Joys values divided by 2^4


typedef struct __attribute__((packed)) {
    uint8_t buttons;  // 4 botones comprimidos en bits
    uint8_t joyX;     // 0-255
    uint8_t joyY;     // 0-255
    int16_t gyro;     // 1 eje del giroscopio 

} controlPacket;
