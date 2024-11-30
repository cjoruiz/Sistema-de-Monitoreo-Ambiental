// Model.h
#ifndef MODEL_H
#define MODEL_H

#include <LiquidMenu.h>
#include "melody.h"
#include <LiquidCrystal.h>
#include <Keypad.h>
#include <DHT.h>
#include "StateMachineLib.h"
#include "AsyncTaskLib.h"
#include <LiquidMenu_config.h>

#define SENSOR_PIN 6 // Pin del sensor infrarrojo
#define DHT_TYPE DHT11
#define DHT_PIN 39
#define PHOTOCELL_PIN A0
#define BUZZER_PIN 7
#define HALL_PIN 53
#define LCD_RS 12
#define LCD_EN 11
#define LCD_D4 5
#define LCD_D5 4
#define LCD_D6 3
#define LCD_D7 2
#define LED_R 8
#define LED_G 9
#define LED_B 10

// ********************** VARIABLES ******************************

#define PASSWORD_ATTEMPTS 3

#define DEF_TMP_HIGH 40
#define DEF_TMP_LOW 10
#define DEF_LUZ_HIGH 650
#define DEF_LUZ_LOW 20
// #define DEF_HALL 600
#define DEF_HALL 0
#define DEF_IR 1

// Variables para la contraseña
struct buffer {
  static const int size = 16;
  char str[size + 1];
  byte len = 0;
  void push(char chr) {
    if (len == size) return;
    str[len++] = chr;
  }
  void clear() {
    for (size_t i = 0; i < this->len; i++) {
      str[i] = 0;
    }
    len = 0;
  }
  bool isFull() {
    return len == size;
  }
  char lastCharacter() {
    return len == 0 ? 0 : str[len - 1];
  }
} keypadBuffer;
const byte password_len = 4;
const char password[password_len + 1] = "4444";
byte password_attempts;

// Variables de la configuaracion de los eventos
float tmp_high = DEF_TMP_HIGH;
float tmp_low = DEF_TMP_LOW;
int16_t luz_high = DEF_LUZ_HIGH;
int16_t luz_low = DEF_LUZ_LOW;
int16_t hall_high = DEF_HALL;
int16_t ir_low = DEF_IR;

// Variables de Monitoreo
float T = 0; /* !< Temperatura */
int16_t H = 0; /* !< Humedad */
int16_t L = 0; /* !< Luz */
int16_t HALL = 0; /* !< Hall */
int16_t IR_SENSOR = 0; /* !< Sensor Infrarrojo */


char messageAlarma[17];
char messageAlerta[17];

/* CONFIGURACION LCD */
LiquidCrystal lcd(LCD_RS, LCD_EN, LCD_D4, LCD_D5, LCD_D6, LCD_D7);

/* CONFIGURACION DHT */
DHT dht(DHT_PIN, DHT_TYPE);

/* CONFIGURACION KEYPAD */
const byte KEYPAD_ROWS = 4;
const byte KEYPAD_COLS = 4;
char padKeys[KEYPAD_ROWS][KEYPAD_COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte keypadRowPins[KEYPAD_ROWS] = {22, 24, 26, 28}; //connect to the row pinouts of the keypad
byte keypadColPins[KEYPAD_COLS] = {30, 32, 34, 36}; //connect to the column pinouts of the keypad
Keypad customKeypad = Keypad( makeKeymap(padKeys), keypadRowPins, keypadColPins, KEYPAD_ROWS, KEYPAD_COLS);



// Enums de la maquina de estados
enum State {
  Inicio = 0,
  Alerta = 1,
  MonitoreoAmbiental = 2,
  Bloqueado = 3,
  MonitoreoEventos = 4,
  Alarma = 5,
};
enum Input {
  Unknown = 0,
  ClaveCorrecta = 2,
  Cambio = 1,
  BloqueoSistema = 3,
  Timeout = 4,
  Umbral = 5
};
Input input = Unknown;

#endif // MODEL_H