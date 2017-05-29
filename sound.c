#include <avr/io.h>
#include "sound.h"
#include "avr_utils.h"


#define c_note 1915    // 261 Hz
#define d_note 1700    // 294 Hz
#define e_note 1519    // 329 Hz
#define f_note 1432    // 349 Hz
#define g_note 1275    // 392 Hz
#define a_note 1136    // 440 Hz
#define b_note 1014    // 493 Hz
#define C_note 956     // 523 Hz

// Define a special note, 'R', to represent a rest
#define R_note 0

int melody_Tup[] = {  c_note,  g_note };
int beats_Tup[]  = { 16, 16 };
int MAX_COUNT_UP= sizeof(melody_Tup) / 2; // Melody length, for looping.

int melody_Tdown[] = {  g_note,  c_note };
int beats_Tdown[]  = { 16, 16 };
int MAX_COUNT_DOWN= sizeof(melody_Tdown) / 2; // Melody length, for looping.

int melody_Ttouch[] = {  c_note };
int beats_Ttouch[]  = { 16 };
int MAX_COUNT_TOUCH= sizeof(melody_Ttouch) / 2; // Melody length, for looping.

int melody_Tvalidate[] = {  g_note,  c_note };
int beats_Tvalidate[]  = { 16, 16 };
int MAX_COUNT_VALIDATE= sizeof(melody_Tvalidate) / 2; // Melody length, for looping.

int melody_Twrong[] = {  e_note,  e_note, c_note, c_note};
int beats_Twrong[]  = { 32, 32, 32, 32};
int MAX_COUNT_WRONG= sizeof(melody_Twrong) / 2; // Melody length, for looping.

int melody_Tgetin[] = {  e_note,  f_note };
int beats_Tgetin[]  = { 16, 16 };
int MAX_COUNT_GETIN= sizeof(melody_Tgetin) / 2; // Melody length, for looping.

int melody_Tgetout[] = {  f_note,  e_note };
int beats_Tgetout[]  = { 16, 16 };
int MAX_COUNT_GETOUT= sizeof(melody_Tgetout) / 2; // Melody length, for looping.

int melody_Talarm[] = {  c_note,  d_note, e_note, f_note, g_note, a_note, b_note, C_note };
int beats_Talarm[]  = { 8, 8, 8, 8, 8, 8, 8};
int MAX_COUNT_ALARM= sizeof(melody_Talarm) / 2; // Melody length, for looping.



// Set overall tempo
long tempo = 5000;

// Set length of pause between notes
int pause = 500;

// Loop variable to increase Rest length
int rest_count = 50; //<-BLETCHEROUS HACK; See NOTES

// Initialize core variables
int tone_ = 0;
int beat = 0;
long duration  = 0;

// PLAY TONE  ==============================================
// Pulse the speaker to play a tone for a particular duration
void playTone() {
	long elapsed_time = 0;
	int j;
	if (tone_ > 0) { // if this isn't a Rest beat, while the tone has

		//  played less long than 'duration', pulse speaker HIGH and LOW
		while (elapsed_time < duration) {
			PORTB |= 0x2;
			delay_us(tone_ / 2);

			// DOWN
			PORTB &= ~0x2;
			delay_us(tone_ / 2);
			// Keep track of how long we pulsed
			elapsed_time += (tone_);
		}
	} else { // Rest beat; loop times delay
		for (j = 0; j < rest_count; j++) // See NOTE on rest_count
			delay_us(duration);
	}
}
void bip()
{
	tone_ = melody_Ttouch[0];
	beat = beats_Ttouch[0];
	duration = beat * tempo; // Set up timing
	playTone();
}

void bip_alter()
{
	int i;

	for (i=0; i<MAX_COUNT_DOWN; i++) {
		tone_ = melody_Tdown[i];
		beat = beats_Tdown[i];
		duration = beat * tempo; // Set up timing
		playTone();
	}
}
void bip_wrong()
{
	int i;

	for (i=0; i<MAX_COUNT_WRONG; i++) {
		tone_ = melody_Twrong[i];
		beat = beats_Twrong[i];
		duration = beat * tempo; // Set up timing
		playTone();
	}
}

void play_getin()
{
	int i;

	for (i=0; i<MAX_COUNT_GETIN; i++) {
		tone_ = melody_Tgetin[i];
		beat = beats_Tgetin[i];
		duration = beat * tempo; // Set up timing
		playTone();
	}
}

void play_getout()
{
	int i;

	for (i=0; i<MAX_COUNT_GETOUT; i++) {
		tone_ = melody_Tgetout[i];
		beat = beats_Tgetout[i];
		duration = beat * tempo; // Set up timing
		playTone();
	}
}

void play_alarm()
{
	int i;

	for (i=0; i<MAX_COUNT_ALARM; i++) {
		tone_ = melody_Talarm[i];
		beat = beats_Talarm[i];
		duration = beat * tempo; // Set up timing
		playTone();
	}
}

void start_song()
{
	int i;

	for (i=0; i<MAX_COUNT_UP; i++) {
		tone_ = melody_Tup[i];
		beat = beats_Tup[i];

		duration = beat * tempo; // Set up timing

		playTone();
		// A pause between notes...
		delay_us(pause);
	}
}
void speaker_init()
{
	/* speaker */
	DDRB |= 0x2; // port B1
	PORTB = 0;
}
