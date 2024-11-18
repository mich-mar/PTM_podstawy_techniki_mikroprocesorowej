/**************************************************/
/*   PTM - Podstawy Techniki Mikroprocesorowej    */
/*           Michał Markuzel, 275417              */
/**************************************************/

#define F_CPU 1000000UL  // 1 MHz
#include <avr/io.h>
#include <stdio.h>
#include <util/delay.h>
#include <string.h>

// Definicje pinów
#define RS 0        // Register Select pin na porcie PA0
#define RW 1        // Read/Write pin na porcie PA1
#define E  2        // Enable pin na porcie PA2
#define SW1_PIN 8   // Switch 1 pin, odpowiada PD8
#define SW5_PIN 9   // Switch 5 pin, odpowiada PD9
#define SW9_PIN 10  // Switch 9 pin, odpowiada PD10
#define SW13_PIN 11 // Switch 13 pin, odpowiada PD11
#define AREF_VOLTAGE 5.0      // Maksymalne napięcie odniesienia AREF
#define ADC_MAX_VALUE 1023    // Maksymalna wartość odczytu ADC (10-bitowa rozdzielczość)
#define SCALED_MAX 400        // Zakres skalowania do 0-400

// Zmienne globalne do wyświetlania parametrów
int _sp=50, _h=4;
float _pv3=300, _e=-200;
bool cv;
int sw1, sw5, sw9, sw13;

// Funkcje opóźnienia czasowego
void delay_ms(int ms) {
    volatile long unsigned int i;
    for(i = 0; i < ms; i++) _delay_ms(1);
}

void delay_us(int us) {
    volatile long unsigned int i;
    for(i = 0; i < us; i++) _delay_us(1);
}

/**
 * @brief Inicjalizacja wyświetlacza LCD 2x16
 * Ustawia tryb wyświetlacza: dwie linie, kursor i miganie
 */
void LCD2x16_init(void) {
    PORTC &= ~(1 << RS);
    PORTC &= ~(1 << RW);

    PORTC |= (1 << E);
    PORTD = 0x38;   // Ustawienia dla dwóch linii, 5x7 punktów
    PORTC &= ~(1 << E);
    _delay_us(120);

    PORTC |= (1 << E);
    PORTD = 0x0E;   // Włącza wyświetlacz, kursor, miganie
    PORTC &= ~(1 << E);
    _delay_us(120);

    PORTC |= (1 << E);
    PORTD = 0x06;
    PORTC &= ~(1 << E);
    _delay_us(120);
}

/**
 * @brief Czyści wyświetlacz LCD
 */
void LCD2x16_clear(void) {
    PORTC &= ~(1 << RS);
    PORTC &= ~(1 << RW);

    PORTC |= (1 << E);
    PORTD = 0x01;
    PORTC &= ~(1 << E);
    delay_ms(120);
}

/**
 * @brief Wyświetla pojedynczy znak na wyświetlaczu LCD
 * @param data Znak do wyświetlenia
 */
void LCD2x16_putchar(int data) {
    PORTC |= (1 << RS);
    PORTC &= ~(1 << RW);

    PORTC |= (1 << E);
    PORTD = data;
    PORTC &= ~(1 << E);
    _delay_us(120);
}

/**
 * @brief Ustawia kursor na wybranym wierszu i kolumnie
 * @param wiersz Numer wiersza (1 lub 2)
 * @param kolumna Numer kolumny (1 do 16)
 */
void LCD2x16_pos(int wiersz, int kolumna) {
    PORTC &= ~(1 << RS);
    PORTC &= ~(1 << RW);

    PORTC |= (1 << E);
    delay_ms(1);
    PORTD = 0x80 + (wiersz - 1) * 0x40 + (kolumna - 1);
    delay_ms(1);
    PORTC &= ~(1 << E);
    _delay_us(120);
}

/**
 * @brief Wyświetla parametry "E", "H", "PV" oraz "SP" na LCD
 */
void print_LCD() {
    char tmp1[16];
    char tmp2[16];

    char str_e[10];
    char str_pv[10];

    dtostrf(_e, 1, 2, str_e);  // Konwersja wartości zmiennoprzecinkowej _e
    dtostrf(_pv3, 1, 2, str_pv); // Konwersja wartości zmiennoprzecinkowej _pv3

    sprintf(tmp1, "E:%s H:%i%%       ", str_e, _h);
    sprintf(tmp2, "PV:%s SP:%i%%  ", str_pv, _sp);

    LCD2x16_pos(1, 1);
    for(int i = 0; i < 16; i++)
        LCD2x16_putchar(tmp1[i]);

    LCD2x16_pos(2, 1);
    for(int i = 0; i < 16; i++)
        LCD2x16_putchar(tmp2[i]);
}

/**
 * @brief Inicjalizuje piny jako wejścia dla przycisków
 * Ustawia piny PB8-PB11 jako wejścia, aby monitorować stany przycisków
 */
void SW_init(void) {
    DDRB &= ~(1 << SW1_PIN); // Ustawia PB8 jako wejście
    DDRB &= ~(1 << SW5_PIN); // Ustawia PB9 jako wejście
    DDRB &= ~(1 << SW9_PIN); // Ustawia PB10 jako wejście
    DDRB &= ~(1 << SW13_PIN); // Ustawia PB11 jako wejście
}

/**
 * @brief Odczytuje stan przycisków podłączonych do pinów PB8-PB11
 * Wartości stanów przycisków są przechowywane w zmiennych sw1, sw5, sw9, sw13
 */
void read_SW() {
    sw1 = (PINB & (1 << (SW1_PIN - 8))) ? 1 : 0;
    sw5 = (PINB & (1 << (SW5_PIN - 8))) ? 1 : 0;
    sw9 = (PINB & (1 << (SW9_PIN - 8))) ? 1 : 0;
    sw13 = (PINB & (1 << (SW13_PIN - 8))) ? 1 : 0;
}

/**
 * @brief Zmienia wartości stanów przycisków w sw1, sw5, sw9, sw13
 */
void change_values() {
  read_SW();

  if(sw1 == 0)
    _sp = 50;
  if(sw5 == 0)
    _sp = 40;
  if(sw9 == 0)
    _h = 4;
  if(sw13 == 0)
    _h = 10;
}

/**
 * @brief Wyświetla stany przycisków na wyświetlaczu LCD
 */
void print_SW() {
    read_SW();

    char tmp1[16];
    char tmp2[16];

    sprintf(tmp1, "SW1:%i SW5:%i     ", sw1, sw5);
    sprintf(tmp2, "SW9:%i SW13:%i     ", sw9, sw13);

    LCD2x16_pos(1, 1);
    for(int i = 0; i < 16; i++)
        LCD2x16_putchar(tmp1[i]);

    LCD2x16_pos(2, 1);
    for(int i = 0; i < 16; i++)
        LCD2x16_putchar(tmp2[i]);
}

/**
 * @brief Funkcja inicjalizująca ADC i konfiguruje kanał A5 do odczytu analogowego
 */
void ADC_init() {
    ADMUX = (1 << REFS0) | (1 << MUX0) | (1 << MUX2); // Ustaw referencję AVcc i wybierz kanał A5
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // Włącz ADC, preskaler 128 (dla większej dokładności)
}

/**
 * @brief Odczytuje napięcie na pinie A5 i przelicza na zakres 0-5V
 * @return Napięcie w woltach (float) w zakresie 0.00 do 5.00V
 */
float read_inputs() {
    ADCSRA |= (1 << ADSC); // Rozpocznij konwersję ADC na kanale A5
    while (ADCSRA & (1 << ADSC)); // Czekaj na zakończenie konwersji

    int adc_value = ADC;               // Pobierz wartość ADC
    _pv3 = (adc_value / 1023.0) * 400; // Przeskaluj do napięcia w zakresie 0–5V
  	
  	float setpoint = (_sp / 100.0) * 400;
    _e = setpoint - _pv3;
  
}

/**
 * @brief Implementacja regulatora dwustawowego.
 * Steruje stanem urządzenia na podstawie wartości PV (zmienna procesu),
 * wartości SP (wartość zadana) oraz H (histereza).
 */
void controller() {
    // Przeliczanie wartości zadanej (SP) na skalę 0-400
    float setpoint = (_sp / 100.0) * 400;
    
    // Wyznaczanie granic histerezy
    float h_half = (_h / 100.0) * 400 / 2.0;

    // Sprawdzanie warunków działania regulatora
    if (_e > h_half) {
        cv = true;
        PORTB |= (1 << PB5);  // Ustaw D13 na wysoki stan (włącz diodę)
    } else if (_e < -h_half) {
        cv = false;
        PORTB &= ~(1 << PB5); // Ustaw D13 na niski stan (wyłącz diodę)
    }
}




/**
 * @brief Główna funkcja programu
 * Inicjalizuje wyświetlacz LCD oraz piny przycisków, a następnie wyświetla stany przycisków w pętli
 */
int main(void) {
    DDRD = 0xff;   // Ustawia port D jako wyjścia
    PORTD = 0x00;  // Ustawia wszystkie wyjścia portu D na niski poziom
    DDRC = 0xff;   // Ustawia port C jako wyjścia
    PORTC = 0x00;  // Ustawia wszystkie wyjścia portu C na niski poziom

    _delay_ms(200); // Krótkie opóźnienie po inicjalizacji

    LCD2x16_init(); // Inicjalizacja wyświetlacza
    LCD2x16_clear(); // Czyszczenie wyświetlacza
    SW_init();       // Inicjalizacja pinów przycisków
    ADC_init();

    while(1) {
        change_values();  // Sprawdzamy zmiany przycisków
        read_inputs(); // Odczytujemy aktualną wartości
        controller();     // Uruchom regulator dwustawowy
        print_LCD();      // Wyświetlamy dane na LCD
        //print_SW();     // Wyświetlanie stanów przycisków
    }

    return 0;
}
