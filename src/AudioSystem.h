#pragma once
#include <M5Cardputer.h>

enum class SoundEvent {
    BOOT,
    NAVIGATE,
    CLICK,
    SELECT,
    CONFIRM,
    ERROR,
    SHUTDOWN,
    ALERT,
    POMO_FOCUS,
    POMO_BREAK
};

struct ToneNote {
    uint32_t freq;
    uint32_t duration;
    uint32_t pause;
};

class AudioSystem {
private:
    // Fixed-size ring buffer — no heap allocation
    static const int QUEUE_CAP = 8;
    ToneNote queue[QUEUE_CAP];
    uint8_t queueHead = 0;
    uint8_t queueSize = 0;
    unsigned long nextNoteTime = 0;
    bool playing = false;
    uint8_t masterVolume = 64; // 0-255

    void enqueue(uint32_t freq, uint32_t duration, uint32_t pause) {
        if (queueSize >= QUEUE_CAP) return;
        uint8_t idx = (queueHead + queueSize) % QUEUE_CAP;
        queue[idx] = {freq, duration, pause};
        queueSize++;
    }

    ToneNote dequeue() {
        ToneNote n = queue[queueHead];
        queueHead = (queueHead + 1) % QUEUE_CAP;
        queueSize--;
        return n;
    }

public:
    void init() {
        M5Cardputer.Speaker.setVolume(masterVolume);
    }

    void setVolume(uint8_t vol) {
        masterVolume = vol;
        M5Cardputer.Speaker.setVolume(masterVolume);
    }

    uint8_t getVolume() const {
        return masterVolume;
    }

    void update() {
        if (!playing || queueSize == 0) return;
        
        if (millis() >= nextNoteTime) {
            if (M5Cardputer.Speaker.isPlaying()) return; // Wait for current tone to finish

            ToneNote note = dequeue();

            if (note.freq > 0) {
                M5Cardputer.Speaker.tone(note.freq, note.duration);
            }
            
            nextNoteTime = millis() + note.duration + note.pause;
            
            if (queueSize == 0) {
                playing = false;
            }
        }
    }

    void play(InteractionStyle style, SoundEvent evt) {
        // Reset queue
        queueHead = 0;
        queueSize = 0;

        switch (style) {
            case InteractionStyle::ORGANIZER:
            case InteractionStyle::PASTEL:
            case InteractionStyle::PRIDE:
                if (evt == SoundEvent::NAVIGATE) { enqueue(4000, 20, 0); }
                else if (evt == SoundEvent::CLICK) { enqueue(5000, 30, 0); }
                else if (evt == SoundEvent::SELECT) { enqueue(4500, 20, 0); }
                else if (evt == SoundEvent::CONFIRM) { enqueue(4000, 40, 20); enqueue(6000, 60, 0); }
                else if (evt == SoundEvent::ERROR) { enqueue(1000, 80, 40); enqueue(1000, 80, 0); }
                else if (evt == SoundEvent::BOOT) { enqueue(3000, 100, 50); enqueue(4000, 100, 50); enqueue(5000, 150, 0); }
                else if (evt == SoundEvent::SHUTDOWN) { enqueue(4000, 80, 40); enqueue(3000, 80, 40); enqueue(2000, 120, 0); }
                else if (evt == SoundEvent::POMO_FOCUS) { enqueue(3000, 50, 20); enqueue(4000, 50, 20); enqueue(5000, 100, 0); }
                else if (evt == SoundEvent::POMO_BREAK) { enqueue(4000, 100, 20); enqueue(3000, 200, 0); }
                break;

            case InteractionStyle::STEEL:
                if (evt == SoundEvent::NAVIGATE) { enqueue(800, 15, 0); }
                else if (evt == SoundEvent::CLICK) { enqueue(1200, 20, 0); }
                else if (evt == SoundEvent::SELECT) { enqueue(1000, 15, 0); }
                else if (evt == SoundEvent::CONFIRM) { enqueue(1500, 30, 30); enqueue(2000, 50, 0); }
                else if (evt == SoundEvent::ERROR) { enqueue(300, 100, 0); }
                else if (evt == SoundEvent::BOOT) { enqueue(1000, 100, 0); enqueue(1500, 200, 0); }
                else if (evt == SoundEvent::SHUTDOWN) { enqueue(800, 100, 0); enqueue(400, 200, 0); }
                else if (evt == SoundEvent::POMO_FOCUS) { enqueue(1000, 50, 0); enqueue(1500, 150, 0); }
                else if (evt == SoundEvent::POMO_BREAK) { enqueue(1000, 150, 0); enqueue(800, 200, 0); }
                break;

            case InteractionStyle::VAPORWARE:
                if (evt == SoundEvent::NAVIGATE) { enqueue(659, 30, 0); }
                else if (evt == SoundEvent::CLICK) { enqueue(880, 40, 0); }
                else if (evt == SoundEvent::SELECT) { enqueue(740, 30, 0); }
                else if (evt == SoundEvent::CONFIRM) { enqueue(523, 60, 20); enqueue(659, 60, 20); enqueue(783, 100, 0); }
                else if (evt == SoundEvent::ERROR) { enqueue(200, 150, 0); }
                else if (evt == SoundEvent::BOOT) { enqueue(440, 150, 0); enqueue(554, 150, 0); enqueue(659, 150, 0); enqueue(880, 300, 0); }
                else if (evt == SoundEvent::SHUTDOWN) { enqueue(880, 100, 0); enqueue(659, 100, 0); enqueue(440, 200, 0); }
                else if (evt == SoundEvent::POMO_FOCUS) { enqueue(440, 50, 0); enqueue(554, 50, 0); enqueue(880, 150, 0); }
                else if (evt == SoundEvent::POMO_BREAK) { enqueue(880, 100, 0); enqueue(659, 100, 0); enqueue(440, 200, 0); }
                break;

            case InteractionStyle::HACKERPUNK:
                if (evt == SoundEvent::NAVIGATE) { enqueue(6000, 10, 20); enqueue(4000, 10, 0); }
                else if (evt == SoundEvent::CLICK) { enqueue(8000, 15, 0); }
                else if (evt == SoundEvent::SELECT) { enqueue(5000, 10, 0); }
                else if (evt == SoundEvent::CONFIRM) { enqueue(3000, 20, 10); enqueue(5000, 20, 10); enqueue(7000, 40, 0); }
                else if (evt == SoundEvent::ERROR) { enqueue(500, 30, 10); enqueue(300, 30, 10); enqueue(500, 30, 0); }
                else if (evt == SoundEvent::BOOT) { enqueue(1000, 20, 20); enqueue(2000, 20, 20); enqueue(3000, 20, 20); enqueue(8000, 50, 0); }
                else if (evt == SoundEvent::SHUTDOWN) { enqueue(500, 50, 0); enqueue(200, 100, 0); }
                else if (evt == SoundEvent::POMO_FOCUS) { enqueue(6000, 20, 10); enqueue(8000, 50, 0); }
                else if (evt == SoundEvent::POMO_BREAK) { enqueue(4000, 50, 10); enqueue(2000, 100, 0); }
                break;

            case InteractionStyle::MATRIX:
                if (evt == SoundEvent::NAVIGATE) { enqueue(12000, 5, 0); }
                else if (evt == SoundEvent::CLICK) { enqueue(10000, 10, 0); }
                else if (evt == SoundEvent::SELECT) { enqueue(11000, 5, 0); }
                else if (evt == SoundEvent::CONFIRM) { enqueue(8000, 20, 20); enqueue(8000, 20, 0); }
                else if (evt == SoundEvent::ERROR) { enqueue(100, 100, 0); }
                else if (evt == SoundEvent::BOOT) { enqueue(2000, 30, 20); enqueue(3000, 30, 20); enqueue(4000, 30, 20); enqueue(5000, 30, 20); }
                else if (evt == SoundEvent::SHUTDOWN) { enqueue(100, 200, 0); }
                else if (evt == SoundEvent::POMO_FOCUS) { enqueue(8000, 20, 20); enqueue(10000, 50, 0); }
                else if (evt == SoundEvent::POMO_BREAK) { enqueue(6000, 50, 20); enqueue(4000, 100, 0); }
                break;

            case InteractionStyle::CASSETTE:
                if (evt == SoundEvent::NAVIGATE) { enqueue(200, 15, 20); enqueue(150, 10, 0); }
                else if (evt == SoundEvent::CLICK) { enqueue(300, 20, 20); enqueue(200, 15, 0); }
                else if (evt == SoundEvent::SELECT) { enqueue(250, 10, 0); }
                else if (evt == SoundEvent::CONFIRM) { enqueue(1000, 50, 20); enqueue(1200, 80, 0); }
                else if (evt == SoundEvent::ERROR) { enqueue(2500, 100, 50); enqueue(2500, 100, 0); }
                else if (evt == SoundEvent::BOOT) { enqueue(400, 50, 30); enqueue(600, 50, 30); enqueue(800, 100, 0); }
                else if (evt == SoundEvent::SHUTDOWN) { enqueue(800, 40, 20); enqueue(600, 40, 20); enqueue(400, 60, 0); }
                else if (evt == SoundEvent::POMO_FOCUS) { enqueue(400, 40, 20); enqueue(800, 80, 0); }
                else if (evt == SoundEvent::POMO_BREAK) { enqueue(800, 60, 20); enqueue(400, 100, 0); }
                break;
        }
        
        // Fallback for ALERT if not specified
        if (evt == SoundEvent::ALERT && queueSize == 0) {
            enqueue(2000, 100, 100);
            enqueue(2000, 100, 100);
        }

        if (queueSize > 0) {
            playing = true;
            nextNoteTime = millis();
        }
    }
};

extern AudioSystem SysAudio;
