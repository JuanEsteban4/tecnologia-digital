#pragma once

#define BUZZER_PIN 8 


void setupBuzzer() {
  ledcAttach(BUZZER_PIN, 5000, 8);
}


struct SongState {
    const uint16_t* freqs = nullptr;
    const uint16_t* durs = nullptr;
    const unsigned long* starts = nullptr;
    int notes = 0;
    float speed = 1.0;
    uint8_t pin = BUZZER_PIN;

    bool playing = false;
    int index = 0;

    unsigned long startTime = 0;   // millis when song started
    unsigned long noteEndTime = 0; // end time (relative to song time)
} song;


void playSongAsyncStart(
  const uint16_t* freqs,
  const uint16_t* durs,
  const unsigned long* starts,
  int notes,
  float speed = 1.0,
  uint8_t pin = BUZZER_PIN
){
    song.freqs  = freqs;
    song.durs   = durs;
    song.starts = starts;
    song.notes  = notes;
    song.speed  = speed;
    song.pin    = pin;

    song.playing = true;
    song.index = 0;
    song.startTime = millis();
    song.noteEndTime = 0;

    ledcWriteTone(song.pin, 0);
}

void silenceBuzzer(){
    song.playing = false;
    ledcWriteTone(song.pin, 0);
}
//----------------------------------------------------
//         NON-BLOCKING UPDATE (CALL IN LOOP)
//----------------------------------------------------
void playSongAsyncUpdate() {
    if (!song.playing) return;

    unsigned long now = millis();
    unsigned long songTime = now - song.startTime;

    if (song.index >= song.notes) {
        //song.playing = false;
        //LOOP song
        song.index = 0;
        ledcWriteTone(song.pin, 0);
        return;
    }

    unsigned long start = song.starts[song.index] / song.speed;


    if(songTime > song.noteEndTime && songTime < start){
        ledcWriteTone(song.pin, 0);
        Serial.println(millis());
        return;
    }
    
    // Not time for this note yet
    if (songTime < start)
        return;

    // Currently waiting for current note to end (ringing)
    if (songTime < song.noteEndTime)
        return;

    // ---------------------------
    // PLAY NEXT NOTE
    // ---------------------------
    // wait until songtime is greater than next dur
    


    uint16_t freq = song.freqs[song.index];
    uint16_t dur  = song.durs[song.index] / song.speed;
    if (dur < 5) dur = 5;

    if (freq > 0)
        ledcWriteTone(song.pin, freq);
    else
        ledcWriteTone(song.pin, 0);

    song.noteEndTime = start + dur;
    song.index++;
}


//----------------------------------------------------
//                EXAMPLE USAGE
//----------------------------------------------------
// Debes definir extern tus arrays:
// extern const uint16_t melody[];
// extern const uint16_t durations[];
// extern const unsigned long starts[];
// extern const int NUM_NOTES;

// En tu sketch normal:
//
// void setup() {
//   setupBuzzer();
//   playSongAsyncStart(melody, durations, starts, NUM_NOTES, 1.0);
// }
//
// void loop() {
//   playSongAsyncUpdate();
//   // tu código normal sigue funcionando sin bloqueos
// }
