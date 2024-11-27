/**
  Proyecto Arquitectura Computacional - Microcontroladores
  Hecho por:
  cristian ortega
  santiago bastidas
  edwin ordoñes
**/
#include <LiquidMenu.h>
#include "melody.h"
#include <LiquidCrystal.h>
#include <Keypad.h>
#include <DHT.h>
#include "StateMachineLib.h"
#include "AsyncTaskLib.h"
#include <LiquidMenu_config.h>

// ********************* PINES CONF ******************************

#define SENSOR_PIN 6 // Pin del sensor infrarrojo
#define DHT_TYPE DHT22
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
int alert_attempts = 3;
#define DEF_TMP_HIGH 40
#define DEF_TMP_LOW 10
#define DEF_LUZ_HIGH 650
#define DEF_LUZ_LOW 20
// #define DEF_HALL 600
#define DEF_HALL 0
#define DEF_IR 0

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

//************************** MAQUINA DE ESTADOS  ************************************


void onEnteringInicio();
void onLeavingInicio();
void onEnteringAlerta();
void onLeavingAlerta();
void onEnteringMAmbiental();
void onLeavingMAmbiental();
void onEnteringMEventos();
void onLeavingMEventos();
void onEnteringBloqueado();
void onLeavingBloqueado();
void onEnteringAlarma();
void onLeavingAlarma();

boolean compareAndResetInput(Input inputIn);

StateMachine stateMachine(6, 11);

void setupStateMachine() {
  // Inicio
  stateMachine.AddTransition(Inicio, MonitoreoAmbiental, []() {
    return compareAndResetInput(Input::ClaveCorrecta);
  });
  stateMachine.AddTransition(Inicio, Bloqueado, []() {
    return compareAndResetInput(Input::BloqueoSistema);
  });

  // Alerta
  stateMachine.AddTransition(Alerta, Alarma, []() {
    return compareAndResetInput(Umbral);
  });

  //Monitoreo Ambiental
  // stateMachine.AddTransition(MonitoreoAmbiental, Alerta, []() {
  //   return compareAndResetInput(Cambio);
  // });
  stateMachine.AddTransition(MonitoreoAmbiental, Alarma, []() {
    return compareAndResetInput(Umbral);
  });
  stateMachine.AddTransition(MonitoreoAmbiental, MonitoreoEventos, []() {
    return compareAndResetInput(Timeout);
  });
  stateMachine.AddTransition(MonitoreoAmbiental, Alerta, []() {
    return compareAndResetInput(Cambio);
  });

  // Bloqueado
  stateMachine.AddTransition(Bloqueado, Inicio, []() {
    return compareAndResetInput(Timeout);
  });

  //Monitoreo Eventos
  stateMachine.AddTransition(MonitoreoEventos, Alerta, []() {
    return compareAndResetInput(Cambio);
  });
  stateMachine.AddTransition(Alerta, MonitoreoEventos, []() {
    return compareAndResetInput(Timeout);
  });
  stateMachine.AddTransition(MonitoreoEventos, MonitoreoAmbiental, []() {
    return compareAndResetInput(Timeout);
  });
  stateMachine.AddTransition(Alarma, Inicio, []() {
    return compareAndResetInput(Cambio);
  });

  stateMachine.SetOnEntering(Inicio, onEnteringInicio);
  stateMachine.SetOnEntering(Alerta, onEnteringAlerta);
  stateMachine.SetOnEntering(MonitoreoAmbiental, onEnteringMAmbiental);
  stateMachine.SetOnEntering(Bloqueado, onEnteringBloqueado);
  stateMachine.SetOnEntering(MonitoreoEventos, onEnteringMEventos);
  stateMachine.SetOnEntering(Alarma, onEnteringAlarma);
  stateMachine.SetOnLeaving(Inicio, onLeavingInicio);
  stateMachine.SetOnLeaving(Alerta, onLeavingAlerta);
  stateMachine.SetOnLeaving(MonitoreoAmbiental, onLeavingMAmbiental);
  stateMachine.SetOnLeaving(Bloqueado, onLeavingBloqueado);
  stateMachine.SetOnLeaving(MonitoreoEventos, onLeavingMEventos);
  stateMachine.SetOnLeaving(Alarma, onLeavingAlarma);
}

// INPUT FUNCTION
boolean compareAndResetInput(Input inputIn) {
  if (input == inputIn) {
    input = Unknown;
    return true;
  }
  return false;
}




// ***********************  PANTALLA ********************************



void blankFunction() {
  return;
}


LiquidScreen scrn_Alerta;

LiquidLine status_tmp_m(0, 0, "Luz:", L);
LiquidLine status_hum_m(8, 0, "Hum:", H, "%");
LiquidLine status_luz_m(0, 1, "Temp:", T, "C");
LiquidScreen scrn_monitoreo_ambiental(status_tmp_m, status_hum_m, status_luz_m);

LiquidLine status_hall_m(0, 0, "Hall:", HALL);
LiquidLine status_if_m(0, 1, "If: ", IR_SENSOR);
LiquidScreen scrn_monitoreo_eventos(status_hall_m, status_if_m);

LiquidLine line_alarma(0, 0, "Precaucion");
LiquidLine status_alarma(0, 1, messageAlarma);
LiquidScreen scrn_alarma(line_alarma, status_alarma);

LiquidLine line_alerta(0, 0, "Precaucion");
LiquidLine status_alerta(0, 1, messageAlerta);
LiquidScreen scrn_alerta(line_alerta, status_alerta);

LiquidMenu mainMenu(lcd);
/**
 * @brief Configura el menú de la pantalla LCD.
 * 
 * Esta función agrega las diferentes pantallas al menú principal.
 */
void setupLiquidMenu() {
  scrn_Alerta.set_displayLineCount(2);
  mainMenu.add_screen(scrn_Alerta);
  mainMenu.add_screen(scrn_monitoreo_eventos);
  mainMenu.add_screen(scrn_monitoreo_ambiental);
  mainMenu.add_screen(scrn_alarma);
  mainMenu.add_screen(scrn_alerta);
}



// ******************** TASK TAREAS ***********************************



void controlTemperatura();
void controlHumedad();
void controlLuz();
void controlHall();
void onSeguridadAFK();
void inputTimeout();
void onUpdateMenu();
void alertaAlarma();
void comeBack();

AsyncTask taskUpdateMenu(1000, true, onUpdateMenu);
AsyncTask taskTemperatura(1000, true, controlTemperatura);
AsyncTask taskHumedad(1000, true, controlHumedad);
AsyncTask taskLuz(500, true, controlLuz);
AsyncTask taskHall(500, true, controlHall);
AsyncTask taskIRSensor(500, true, controlIRSensor);
AsyncTask taskTimeoutEventos(3000, false, inputTimeout);
AsyncTask taskTimeoutAmbiental(7000, false, inputTimeout);
AsyncTask taskTimeoutAlarma(4000, false, inputTimeout);
AsyncTask taskTimeoutAlerta(3000, false, inputTimeout);
AsyncTask taskTimeoutInicioAFK(2000, false, onSeguridadAFK);

/**
 * @brief Controla la temperatura y verifica si excede el límite superior.
 * 
 * Si la temperatura es mayor que el límite superior, se activa la alarma.
 */
void controlTemperatura() {
  T = dht.readTemperature();
  if (T > tmp_high) {
    strcpy(messageAlarma, "'Ta Caliente");
    input = Input::Umbral;
  }
}
/**
 * @brief Controla la humedad y la actualiza.
 */
void controlHumedad() {
  H = dht.readHumidity();
}
/**
 * @brief Controla la luz y verifica si excede el límite superior.
 * 
 * Si la luz es mayor que el límite superior, se activa la alarma.
 */
void controlLuz() {
  L = analogRead(PHOTOCELL_PIN);
  if (L > luz_high) {
    strcpy(messageAlarma, "'Ta Oscuro");
    input = Input::Umbral;
  }
}
/**
 * @brief Controla el sensor Hall y verifica si se detecta un imán.
 * 
 * Si se detecta un imán, se activa la alarma y se enciende un LED azul.
 */
void controlHall() {
  HALL = digitalRead(HALL_PIN);
  if (HALL > hall_high) {
    alertAlarm();
    strcpy(messageAlerta, "iman Detectado");
    setLED(0, 0, 1); // Encender LED Azul
    // input = Input::Cambio;
  }
}

/**
 * @brief Controla el sensor infrarrojo y verifica si se detecta movimiento.
 * 
 * Si se detecta movimiento, se activa la alarma y se enciende un LED azul.
 */

bool motionDetected = false; // Variable para controlar la detección de movimiento
void controlIRSensor() {
  IR_SENSOR = digitalRead(SENSOR_PIN);
  if (IR_SENSOR > ir_low && !motionDetected) { // Solo si no hay detección previa
    if (alert_attempts >= 0) {
      --alert_attempts;
      Serial.print("Intentos restantes: ");
      Serial.println(alert_attempts);
      strcpy(messageAlerta, "Algo Detectado");
      input = Input::Cambio;
      setLED(0, 0, 1); // Encender LED Azul
      motionDetected = true; // Marcar que se ha detectado movimiento
      // Reiniciar la detección después de un tiempo

      delay(3000); // Espera 3 segundos antes de permitir otra detección
      motionDetected = false; // Reiniciar la detección
    } else {
      input = Umbral; // Activar estado de Alarma
      strcpy(messageAlerta, "Algo Detectado");
    }
  }
}
/**
 * @brief Función que se llama al activar el estado de seguridad AFK (Away From Keyboard).
 * 
 * Esta función se encarga de realizar acciones específicas cuando el sistema detecta que no hay actividad
 * durante un periodo de tiempo determinado. Puede incluir la activación de alarmas o notificaciones.
 * En este caso, se activa un tono en el buzzer y se enciende un LED de color rojo para indicar el estado de alerta.
 */
void onSeguridadAFK() {
  --password_attempts; // Reducir los intentos
  setLED(1, 1, 0); // LED amarillo para error
  lcd.clear();
  lcd.print("Tiempo agotado");
  delay(1000);
  setLED(0, 0, 0);
  lcd.clear();
  lcd.print("Clave: ");
  keypadBuffer.clear(); // Reiniciar el buffer
  input = Unknown; // Reiniciar el estado de entrada
}
/**
 * @brief Controla el sensor infrarrojo y verifica si se detecta movimiento.
 * 
 * Si se detecta movimiento, se activa la alarma y se enciende un LED azul.
 */
void alertAlarm() {
  if (alert_attempts > 0) {
    --alert_attempts;
    Serial.print("Intentos restantes: ");
    Serial.println(alert_attempts);
    input = Input::Cambio;
  } else {
    input = Umbral; // Activar estado de Alarma
  }
}
/**
 * @brief Función que se llama cuando se agota el tiempo de entrada.
 * 
 * Esta función establece la entrada como Timeout para la máquina de estados.
 */
void inputTimeout() {
  input = Timeout;
}
/**
 * @brief Actualiza el menú en la pantalla LCD.
 * 
 * Esta función se llama periódicamente para actualizar la visualización del menú.
 */
void onUpdateMenu() {
  mainMenu.update();
}


/* FUNCIONES PREDECLARADAS */

void seguridad();
void menu();
void bloqueo();
void alarma();
void monitoreoAmbiental();
void monitoreoEventos();


/* SETUP ----------------------------------------------------------  */
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

/**
 * @brief Maneja el estado de seguridad.
 * 
 * Esta función permite la entrada de la contraseña y gestiona el acceso al sistema.
 */
void seguridad() {
  updateTimeoutInicioAFK(); // Actualiza la lógica del temporizador

  if (password_attempts == 0) {
    input = BloqueoSistema;
    return;
  }

  const char key = customKeypad.getKey();
  if (!key) return;

  if (key != 'A') {
    keypadBuffer.push(key);
    lcd.print('*');
    startTimeoutInicioAFK(); // Reiniciar el temporizador
    if (!keypadBuffer.isFull()) return;
  }

  if (keypadBuffer.len == password_len && strncmp(password, keypadBuffer.str, password_len) == 0) {
    setLED(0, 1, 0);
    lcd.clear();
    lcd.print("Correcto");
    input = ClaveCorrecta;
  } else {
    --password_attempts;
    setLED(0, 0, 1);
    lcd.clear();
    lcd.print("Incorrecto");
  }
  stopTimeoutInicioAFK();
  delay(1000);
  setLED(0, 0, 0);
  lcd.clear();
  lcd.print("Clave: ");
  keypadBuffer.clear();
}

/**
 * @brief Maneja el estado de bloqueo del sistema.
 * 
 * Esta función se ejecuta cuando el sistema está bloqueado al intoducir la clave 3 veces de forma incorrecta y muestra un mensaje en el LCD.
 */
void bloqueo() {
  setLED(1, 0, 0);
  lcd.setCursor(0, 0);
  lcd.print("Sistema ");
  lcd.setCursor(0, 1);
  lcd.print("Bloqueado");
  execute_melody(melodyBloqueo);
  lcd.clear();
  input = Input::Timeout;
}
/**
 * @brief Maneja el estado de alarma.
 * 
 * Esta función se ejecuta cuando el sistema está en estado de alarma y gestiona las tareas de monitoreo.
 */
void alarma() {
  const char key = customKeypad.getKey();
  taskTimeoutAlarma.Update();
  taskTemperatura.Update();
  taskHumedad.Update();
  taskLuz.Update();
  taskHall.Update();
  taskIRSensor.Update();
  if (key == '*') input = Input::Cambio;
}
/**
 * @brief Maneja el menú de alertas.
 * 
 * Esta función se ejecuta cuando el sistema está en estado de alerta y gestiona las tareas de monitoreo.
 */
void menu() {
  taskTimeoutAlerta.Update();
  taskTimeoutAlarma.Update();
  taskHumedad.Update();
  taskLuz.Update();
  taskHall.Update();
  taskIRSensor.Update();
}
/**
 * @brief Maneja el monitoreo ambiental.
 * 
 * Esta función se ejecuta en el estado de monitoreo ambiental y gestiona las tareas de monitoreo.
 */
void monitoreoAmbiental() {
  taskTemperatura.Update();
  taskHumedad.Update();
  taskLuz.Update();
  taskHall.Update();
  taskIRSensor.Update(); // Llamar a la función del sensor infrarrojo
  taskTimeoutAmbiental.Update();
}
/**
 * @brief Maneja el monitoreo de eventos.
 * 
 * Esta función se ejecuta en el estado de monitoreo de eventos y gestiona las tareas de monitoreo.
 */
void monitoreoEventos() {
  taskTemperatura.Update();
  taskHumedad.Update();
  taskLuz.Update();
  taskHall.Update();
  taskIRSensor.Update();
  taskTimeoutEventos.Update();
  taskUpdateMenu.Update();

}


// *************** MAQUINA DE ESTADOS *****************************
// **************ENTERING/LEAVING FUNCTIONS*********

  // ENTERING/LEAVING FUNCTIONS
/**
 * @brief Función que se llama al entrar en el estado de Inicio.
 * 
 * Esta función inicializa las variables y muestra el mensaje de entrada en el LCD.
 */
void onEnteringInicio() {
  alert_attempts = 3;
  Serial.println("Entering Inicio");
  keypadBuffer.clear();
  password_attempts = PASSWORD_ATTEMPTS;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Clave: ");
}
/**
 * @brief Función que se llama al salir del estado de Inicio.
 * 
 * Esta función detiene el temporizador de seguridad AFK.
 */
void onLeavingInicio() {
  Serial.println("Leaving Inicio");
  taskTimeoutInicioAFK.Stop();
}
/**
 * @brief Función que se llama al entrar en el estado de Alerta.
 * 
 * Esta función activa las tareas de monitoreo y muestra el mensaje de alerta en el LCD.
 */
void onEnteringAlerta() {
  Serial.println("Entering Alerta");
  setLED(0, 0, 1);
  tone(buzzer, 1000, 5000);
  taskTemperatura.Start();
  taskHumedad.Start();
  taskLuz.Start();
  taskHall.Start();
  taskIRSensor.Start(); // Iniciar la tarea del sensor infrarrojo
  taskTimeoutAlerta.Start();
  mainMenu.change_screen(&scrn_alerta);
  mainMenu.update();
}
/**
 * @brief Función que se llama al salir del estado de Alerta.
 * 
 * Esta función detiene las tareas de monitoreo y apaga el buzzer.
 */
void onLeavingAlerta() {
  Serial.println("Leaving Alerta");
  taskTemperatura.Stop();
  taskHumedad.Stop();
  taskLuz.Stop();
  taskHall.Stop();
  taskIRSensor.Stop();
  noTone(buzzer);
  taskTimeoutAlerta.Stop();
  setLED(0, 0, 0);
}
/**
 * @brief Función que se llama al entrar en el estado de Monitoreo Ambiental.
 * 
 * Esta función activa las tareas de monitoreo y muestra la pantalla de monitoreo ambiental.
 */
void onEnteringMAmbiental() {
  Serial.println("Entering Monitoreo Ambiental");
  taskTemperatura.Start();
  taskHumedad.Start();
  taskLuz.Start();
  taskHall.Start();
  taskIRSensor.Start();
  taskTimeoutAmbiental.Start();
  taskUpdateMenu.Start();
  mainMenu.change_screen(&scrn_monitoreo_ambiental);
  mainMenu.update();
}
/**
 * @brief Función que se llama al salir del estado de Monitoreo Ambiental.
 * 
 * Esta función detiene las tareas de monitoreo.
 */

void onLeavingMAmbiental() {
  Serial.println("Leaving Monitoreo Ambiental");
  taskTemperatura.Stop();
  taskHumedad.Stop();
  taskLuz.Stop();
  taskHall.Stop();
  taskIRSensor.Stop();
  taskTimeoutAmbiental.Stop();
  taskUpdateMenu.Stop();
}
/**
 * @brief Función que se llama al entrar en el estado de Monitoreo Eventos.
 * 
 * Esta función activa las tareas de monitoreo y muestra la pantalla de monitoreo de eventos.
 */
void onEnteringMEventos() {
  Serial.println("Entering Monitoreo Eventos");
  taskTemperatura.Start();
  taskHumedad.Start();
  taskLuz.Start();
  taskHall.Start();
  taskIRSensor.Start();
  taskTimeoutEventos.Start();
  taskUpdateMenu.Start();
  mainMenu.change_screen(&scrn_monitoreo_eventos);
  mainMenu.update();
}
/**
 * @brief Función que se llama al entrar en el estado de Monitoreo Eventos.
 * 
 * Esta función activa las tareas de monitoreo y muestra la pantalla de monitoreo de eventos.
 */
void onLeavingMEventos() {
  Serial.println("Leaving Monitoreo Eventos");
  taskTemperatura.Stop();
  taskHumedad.Stop();
  taskLuz.Stop();
  taskHall.Stop();
  taskIRSensor.Stop();
  taskTimeoutEventos.Stop();
  taskUpdateMenu.Stop();
}
/**
 * @brief Función que se llama al entrar en el estado de Bloqueado.
 * 
 * Esta función se ejecuta cuando el sistema está bloqueado.
 */
void onEnteringBloqueado() {
  Serial.println("Entering Bloqueado");
}
/**
 * @brief Función que se llama al salir del estado de Bloqueado.
 * 
 * Esta función apaga los LEDs al salir del estado de bloqueo.
 */
void onLeavingBloqueado() {
  Serial.println("Leaving Bloqueado");
  setLED(0, 0, 0);
}
/**
 * @brief Detiene todas las tareas relacionadas con la alarma.
 * 
 * Esta función se llama al salir del estado de alarma. Detiene las tareas de monitoreo de temperatura,
 * humedad, luz, hall y sensor infrarrojo, apaga el buzzer y apaga todos los LEDs.
 */
void onEnteringAlarma() {
  Serial.println("Entering Alarma");
  setLED(0, 0, 1);
  tone(buzzer, 97, 4000);
  taskTemperatura.Start();
  taskHumedad.Start();
  taskLuz.Start();
  taskHall.Start();
  taskIRSensor.Start(); // Iniciar la tarea del sensor infrarrojo
  mainMenu.change_screen(&scrn_alarma);
  mainMenu.update();
}
/**
 * @brief Establece el estado de los LEDs RGB.
 * 
 * Esta función recibe valores para los componentes rojo, verde y azul del LED y los configura
 * en los pines correspondientes.
 * 
 * @param red Valor para el componente rojo (0-255).
 * @param green Valor para el componente verde (0-255).
 * @param blue Valor para el componente azul (0-255).
 */
void onLeavingAlarma() {
  Serial.println("Leaving Alarma");
  taskTemperatura.Stop();
  taskHumedad.Stop();
  taskLuz.Stop();
  taskHall.Stop();
  taskIRSensor.Stop();
  noTone(buzzer);
  setLED(0, 0, 0);
}
/**
 * @brief Establece el estado de los LEDs RGB.
 * 
 * Esta función recibe valores para los componentes rojo, verde y azul del LED y los configura
 * en los pines correspondientes.
 * 
 * @param red Valor para el componente rojo (0-255).
 * @param green Valor para el componente verde (0-255).
 * @param blue Valor para el componente azul (0-255).
 */
void setLED(uint16_t red, uint16_t green, uint16_t blue) {
  digitalWrite(LED_R, red);
  digitalWrite(LED_G, green);
  digitalWrite(LED_B, blue);
}
/**
 * @brief Verifica si ha transcurrido un intervalo de tiempo desde un momento dado.
 * 
 * Esta función compara el tiempo actual con un tiempo de inicio y un intervalo especificado
 * para determinar si el intervalo ha sido superado.
 * 
 * @param startTime Tiempo de inicio en milisegundos.
 * @param interval Intervalo de tiempo en milisegundos.
 * @return true Si ha transcurrido el intervalo.
 * @return false Si no ha transcurrido el intervalo.
 */
bool hasElapsed(unsigned long startTime, unsigned long interval) {
  return (millis() - startTime) >= interval;
}

unsigned long TimeoutInicioAFK = 0;
bool taskTimeoutInicioAFK_Active = false;
/**
 * @brief Inicia el temporizador para el estado de seguridad AFK.
 * 
 * Esta función almacena el tiempo actual y activa la bandera que indica que el temporizador está activo.
 */
void startTimeoutInicioAFK() {
  TimeoutInicioAFK = millis();
  taskTimeoutInicioAFK_Active = true;
}
/**
 * @brief Detiene el temporizador para el estado de seguridad AFK.
 * 
 * Esta función desactiva la bandera que indica que el temporizador está activo.
 */
void stopTimeoutInicioAFK() {
  taskTimeoutInicioAFK_Active = false;
}
/**
 * @brief Actualiza el estado del temporizador de seguridad AFK.
 * 
 * Esta función verifica si el temporizador está activo y si ha transcurrido el tiempo especificado
 * (2000 ms en este caso). Si se cumple la condición, se llama a la función onSeguridadAFK() 
 * y se detiene el temporizador.
 */
void updateTimeoutInicioAFK() {
  if (taskTimeoutInicioAFK_Active && hasElapsed(TimeoutInicioAFK, 2000)) {
    onSeguridadAFK();
    stopTimeoutInicioAFK();
  }
}