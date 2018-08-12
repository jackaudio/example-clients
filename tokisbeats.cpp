/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   main.cpp
 * Author: thorsten
 *
 * Created on 12. August 2018, 00:23
 */

#include <cstdlib>
#include <ctime>
#include <cmath>
#include <iostream>

extern "C" {
#include <jack/jack.h>
#include <unistd.h>
}

using namespace std;

float accel = 8.0;

struct Instrument {
    float freqA;
    float freqB;
    float shiftA;
    float shiftB;
    float power;
    bool signum;
    jack_default_audio_sample_t buffer[44100];

    float compute(float x) {
        float y = sin(x * freqA + shiftA);
        if (signum) y = -signbit(y) * 2.0 + 1.0;
        y *= (cos(x * freqB + shiftB) + 1.0) / 2.0;
        y /= 1 + pow(x, power);
        return y;
    }

    void randomize() {
        freqA = (float) rand() / RAND_MAX * 0.19 + 0.01;
        freqB = (float) rand() / RAND_MAX * 0.01 + 0.005;
        shiftA = (float) rand() / RAND_MAX * 0.2;
        shiftB = (float) rand() / RAND_MAX * 0.2;
        power = (float) rand() / RAND_MAX * 0.2 + 0.2;
        signum = rand() & 1;

        for (int i = 0; i < sizeof (buffer) / sizeof (buffer[0]); ++i) buffer[i] = 0;

        for (int i = 0; i < (float) sizeof (buffer) / (float) sizeof (buffer[0]) / accel; ++i) {
            buffer[i] = compute((float) i / accel);
        }

        float sum = 0;
        for (int i = 0; i < sizeof (buffer) / sizeof (buffer[0]); ++i) sum += buffer[i];
        for (int i = 0; i < sizeof (buffer) / sizeof (buffer[0]); ++i) buffer[i] /= sum;
        for (int i = 0; i < sizeof (buffer) / sizeof (buffer[0]); ++i) buffer[i] *= 200;
    }
};

struct Pattern {
    Instrument aud;
    bool pat[16];

    int bounds(int i) {
        while (i < 0) i += 16;
        while (i >= 16) i -= 16;

        return i;
    }

    void randomize() {
        for (int i = 0; i < 16; ++i) pat[i] = false;

        for (int i = 0; i < 4; ++i) {
            if (rand() & 1) {
                pat[4 * i] = true;

                if (rand() & 1) {
                    pat[bounds(4 * i + 2)] = true;
                }
                if (rand() & 1) {
                    pat[bounds(4 * i - 1)] = true;
                }
            }
        }
    }
};

int index = 0;
int counter = 0;
int x;
jack_client_t *jc;
jack_port_t *jp;
Pattern patterns[10];

void randomize() {
    for (int i = 0; i < sizeof (patterns) / sizeof (patterns[0]); ++i) {
        patterns[i].aud.randomize();
        patterns[i].randomize();
    }
}

int myCallback(jack_nframes_t nframes, void *arg) {
    if (jack_port_connected(jp)) {
        jack_default_audio_sample_t *b = (jack_default_audio_sample_t *) jack_port_get_buffer(jp, nframes);

        for (int j = 0; j < nframes; ++j) {
            b[j] = 0;
            for (int i = 0; i < sizeof (patterns) / sizeof (patterns[0]); ++i) {
                if (patterns[i].pat[index]) {
                    b[j] += patterns[i].aud.buffer[x + j];
                }
            }
        }

        x += nframes;

        if (x > (44100 / accel)) {
            x = 0;
            index += 1;

            if (index >= 16) {
                index = 0;
                counter += 1;

                if (counter >= 4) {
                    counter = 0;
                    randomize();
                }
            }
        }
    }

    return 0;
}

/*
 * 
 */
int main(int argc, char** argv) {
    time_t t;
    time(&t);
    srand(t);

    randomize();

    jack_status_t stat;
    jc = jack_client_open("TokisBeat", JackNullOption, &stat);
    jack_set_process_callback(jc, myCallback, 0);

    cout << stat << endl;

    jp = jack_port_register(jc, "Mono Port", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
    jack_activate(jc);

    while (true) {
        usleep(1000);
    }

    return 0;
}

