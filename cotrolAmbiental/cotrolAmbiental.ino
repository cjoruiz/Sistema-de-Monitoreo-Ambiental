/**
  Proyecto Arquitectura Computacional - Microcontroladores
  Hecho por:
  cristian ortega
  santiago bastidas
  edwin ordoñes
**/

#include "controller.h"
int alert_attempts = 1;
void setup() {
  // Inicializa los componentes necesarios
  lcd.begin(16, 2);
  dht.begin();
  Serial.begin(9600);

  // Inicializa el LED
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);
  setLED(0, 0, 0);
  // Inicializa el menu del lcd
  setupLiquidMenu();

  // Inicializa la maquina de estados
  Serial.println("Starting State Machine...");
  setupStateMachine();
  Serial.println("State Machine Started");

  stateMachine.SetState(Inicio, false, true);
}
/* LOOP ----------------------------------------------------------  */
void loop() {
  switch (stateMachine.GetState()) {
    case State::Inicio: seguridad(); break;
    case State::Alerta: menu(); break;
    case State::MonitoreoAmbiental: monitoreoAmbiental(); break;
    case State::MonitoreoEventos: monitoreoEventos(); break;
    case State::Alarma: alarma(); break;
    case State::Bloqueado: bloqueo(); break;
  }
  // Actualizar Maquina de Estados
  stateMachine.Update();
}


