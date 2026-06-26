#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include <EEPROM.h>

// LCD
LiquidCrystal_I2C lcd(0x20, 16, 2); 

// Motor DC
const int pinMotor = 5;

// Servo y Botón
const int pinBoton = 4;
const int pinServo = 9;

// LEDS de la Casa
const int sala = 6;
const int comedor = 7;
const int cocina = 8;
const int bath = 10;
const int habitacion = 11;

// Config LEDS
const int L1_Azul = 12;
const int L2_Verde = 13;
const int L3_Rojo = A0;

Servo miServo;

int estadoBoton = 0;
int ultimoEstadoBoton = 0;
int posicion = 0;

unsigned long tiempoAnteriorFiesta = 0;
const long intervaloFiesta = 500; 
bool estadoLucesFiesta = false;

String modoActual = "inicio"; 

char custom1_nombre[11] = "";
int custom1_motor = 0;
byte custom1_leds[5] = {0, 0, 0, 0, 0};

char custom2_nombre[11] = "";
int custom2_motor = 0;
byte custom2_leds[5] = {0, 0, 0, 0, 0};

bool enConfiguracion = false;
bool errorSintaxis = false;
String modoEnProceso = "";
int conteoCustoms = 0;

void cargarConfiguracionesEEPROM() {
  for(int i=0; i<10; i++) custom1_nombre[i] = EEPROM.read(10 + i);
  custom1_nombre[10] = '\0';
  custom1_motor = EEPROM.read(20);
  for(int i=0; i<5; i++) custom1_leds[i] = EEPROM.read(21 + i);
  
  for(int i=0; i<10; i++) custom2_nombre[i] = EEPROM.read(30 + i);
  custom2_nombre[10] = '\0';
  custom2_motor = EEPROM.read(40);
  for(int i=0; i<5; i++) custom2_leds[i] = EEPROM.read(41 + i);

  byte count = EEPROM.read(5);
  if(count <= 2) conteoCustoms = count;
}

void guardarModoEnEEPROM(int slot, String nombre, int motor, byte leds[]) {
  int baseNombre = (slot == 1) ? 10 : 30;
  int baseDatos = (slot == 1) ? 20 : 40;
  
  for(int i=0; i<10; i++) {
    char c = (i < nombre.length()) ? nombre.charAt(i) : '\0';
    EEPROM.update(baseNombre + i, c);
  }
  EEPROM.update(baseDatos, motor);
  for(int i=0; i<5; i++) EEPROM.update(baseDatos + 1 + i, leds[i]);
}

void setup() {
  Serial.begin(9600);

  pinMode(L1_Azul, OUTPUT);
  pinMode(L2_Verde, OUTPUT);
  pinMode(L3_Rojo, OUTPUT);
  digitalWrite(L1_Azul, LOW);
  digitalWrite(L2_Verde, LOW);
  digitalWrite(L3_Rojo, LOW);

  lcd.init();
  lcd.backlight();
  mostrarMensajeLCD("Modo: INICIO", "Esperando mando");

  Serial.println("--- Sistema iniciado (Hardware Serial) ---");
  Serial.println("Escriba un comando o cargue un .org...");

  pinMode(pinMotor, OUTPUT);
  analogWrite(pinMotor, 0); 

  pinMode(pinBoton, INPUT);
  miServo.write(0);
  miServo.attach(pinServo);

  pinMode(sala, OUTPUT);
  pinMode(comedor, OUTPUT);
  pinMode(cocina, OUTPUT);
  pinMode(bath, OUTPUT);
  pinMode(habitacion, OUTPUT);
  
  apagarTodosLosLeds();

  cargarConfiguracionesEEPROM();

  byte modoGuardado = EEPROM.read(0);
  if (modoGuardado >= 1 && modoGuardado <= 7) {
    digitalWrite(L1_Azul, HIGH);
  } else {
    digitalWrite(L3_Rojo, HIGH);
  }

  if (modoGuardado == 1) { modoActual = "modo_fiesta"; mostrarMensajeLCD("RESTAURADO", "Modo: FIESTA"); }
  else if (modoGuardado == 2) { modoActual = "modo_relajado"; apagarTodosLosLeds(); mostrarMensajeLCD("RESTAURADO", "Modo: RELAJADO"); }
  else if (modoGuardado == 3) { modoActual = "modo_noche"; apagarTodosLosLeds(); mostrarMensajeLCD("RESTAURADO", "Modo: NOCHE"); }
  else if (modoGuardado == 4) { modoActual = "encender_todo"; encenderTodosLosLeds(); mostrarMensajeLCD("RESTAURADO", "Modo: ALL ON"); }
  else if (modoGuardado == 5) { modoActual = "apagar_todo"; apagarTodosLosLeds(); mostrarMensajeLCD("RESTAURADO", "Modo: ALL OFF"); }
  else if (modoGuardado == 6) { modoActual = "custom1"; mostrarMensajeLCD("RESTAURADO", String(custom1_nombre)); }
  else if (modoGuardado == 7) { modoActual = "custom2"; mostrarMensajeLCD("RESTAURADO", String(custom2_nombre)); }
}

void loop() {
  
  if (Serial.available() > 0) {
    String linea = Serial.readStringUntil('\n');
    linea.trim();

    if (linea.length() == 0 || linea.startsWith("//")) {
      return; 
    }

    if (linea == "conf_ini") {
      enConfiguracion = true;
      errorSintaxis = false;
      modoEnProceso = "";
      return;
    }

    if (enConfiguracion) {
      if (linea == "conf:fin") {
        enConfiguracion = false;
        if (!errorSintaxis) {
          mostrarMensajeLCD("Configuracion", "guardada");
          for(int i=0; i<3; i++) {
            digitalWrite(L2_Verde, HIGH); delay(200);
            digitalWrite(L2_Verde, LOW); delay(200);
          }
          Serial.println("-> OK: Archivo .org cargado exitosamente");
        } else {
          mostrarMensajeLCD("Error en", "archivo .org");
          digitalWrite(L3_Rojo, HIGH); delay(2000); digitalWrite(L3_Rojo, LOW);
          Serial.println("-> ERROR: Carga abortada por errores de sintaxis");
        }
        return;
      }

      linea.toLowerCase();

      if (linea.startsWith("modo_fiesta") || linea.startsWith("modo_relajado") || linea.startsWith("modo_noche")) {
        modoEnProceso = linea;
        return;
      }

      if (linea.startsWith("modo_custom:")) {
        int primerComilla = linea.indexOf('"');
        int ultimaComilla = linea.lastIndexOf('"');
        if (primerComilla != -1 && ultimaComilla != -1 && ultimaComilla > primerComilla) {
          String nombreModo = linea.substring(primerComilla + 1, ultimaComilla);
          nombreModo.trim();
          if(nombreModo.length() > 10) nombreModo = nombreModo.substring(0,10);
          
          if (String(custom1_nombre) == nombreModo) {
            modoEnProceso = "custom1_edit:" + nombreModo;
            digitalWrite(L2_Verde, HIGH); delay(500); digitalWrite(L2_Verde, LOW);
          } else if (String(custom2_nombre) == nombreModo) {
            modoEnProceso = "custom2_edit:" + nombreModo;
            digitalWrite(L2_Verde, HIGH); delay(500); digitalWrite(L2_Verde, LOW);
          } else {
            if (conteoCustoms == 0) { modoEnProceso = "custom1_new:" + nombreModo; conteoCustoms = 1; EEPROM.update(5, 1); }
            else if (conteoCustoms == 1) { modoEnProceso = "custom2_new:" + nombreModo; conteoCustoms = 2; EEPROM.update(5, 2); }
            else { modoEnProceso = "custom2_new:" + nombreModo; }
          }
        } else {
          errorSintaxis = true;
        }
        return;
      }

      if (linea.startsWith("ventilador:")) {
        int valMotor = linea.indexOf("on") != -1 ? 255 : 0;
        if (linea.indexOf("on") == -1 && linea.indexOf("off") == -1) errorSintaxis = true;
        
        if (modoEnProceso.startsWith("custom1")) {
          custom1_motor = valMotor;
        } else if (modoEnProceso.startsWith("custom2")) {
          custom2_motor = valMotor;
        }
        return;
      }

      if (linea.startsWith("led's:") || linea.startsWith("leds:")) {
        if (modoEnProceso.startsWith("custom1") || modoEnProceso.startsWith("custom2")) {
          bool c1 = modoEnProceso.startsWith("custom1");
          int idxSala = linea.indexOf("sala:");
          int idxComedor = linea.indexOf("comedor:");
          int idxCocina = linea.indexOf("cocina:");
          int idxBano = linea.indexOf("baño:");
          if(idxBano == -1) idxBano = linea.indexOf("bano:");
          int idxHab = linea.indexOf("habitacion:");

          if(idxSala == -1 || idxComedor == -1 || idxCocina == -1 || idxBano == -1 || idxHab == -1) {
            errorSintaxis = true;
            return;
          }

          byte s = linea.substring(idxSala).indexOf("on") != -1 ? 1 : 0;
          byte cm = linea.substring(idxComedor).indexOf("on") != -1 ? 1 : 0;
          byte cc = linea.substring(idxCocina).indexOf("on") != -1 ? 1 : 0;
          byte b = linea.substring(idxBano).indexOf("on") != -1 ? 1 : 0;
          byte h = linea.substring(idxHab).indexOf("on") != -1 ? 1 : 0;

          int separation = modoEnProceso.indexOf(':');
          String exactName = modoEnProceso.substring(separation + 1);

          if (c1) {
            custom1_motor = (custom1_motor == 255) ? 255 : 0;
            custom1_leds[0] = s; custom1_leds[1] = cm; custom1_leds[2] = cc; custom1_leds[3] = b; custom1_leds[4] = h;
            guardarModoEnEEPROM(1, exactName, custom1_motor, custom1_leds);
            strcpy(custom1_nombre, exactName.c_str());
          } else {
            custom2_motor = (custom2_motor == 255) ? 255 : 0;
            custom2_leds[0] = s; custom2_leds[1] = cm; custom2_leds[2] = cc; custom2_leds[3] = b; custom2_leds[4] = h;
            guardarModoEnEEPROM(2, exactName, custom2_motor, custom2_leds);
            strcpy(custom2_nombre, exactName.c_str());
          }
        }
        return;
      }
      return;
    }

    String comando = "";
    for (int i = 0; i < linea.length(); i++) {
      char c = linea.charAt(i);
      if (isalnum(c) || c == '_') comando += c;
    }
    comando.toLowerCase();

    String c1_str = String(custom1_nombre);
    c1_str.toLowerCase();
    String c2_str = String(custom2_nombre);
    c2_str.toLowerCase();

    if (comando == "modo_fiesta") { modoActual = "modo_fiesta"; mostrarMensajeLCD("Modo: FIESTA", "V:ON LEDS:Altern"); EEPROM.update(0, 1); } 
    else if (comando == "modo_relajado") { modoActual = "modo_relajado"; apagarTodosLosLeds(); mostrarMensajeLCD("Modo: RELAJADO", "V:OFF LEDS:OFF"); EEPROM.update(0, 2); } 
    else if (comando == "modo_noche") { modoActual = "modo_noche"; apagarTodosLosLeds(); mostrarMensajeLCD("Modo: NOCHE", "V:OFF LEDS:OFF"); EEPROM.update(0, 3); } 
    else if (comando == "encender_todo") { modoActual = "encender_todo"; encenderTodosLosLeds(); mostrarMensajeLCD("LEDS: ON", "Ventilador: ON"); EEPROM.update(0, 4); } 
    else if (comando == "apagar_todo") { modoActual = "apagar_todo"; apagarTodosLosLeds(); mostrarMensajeLCD("LEDS: OFF", "Ventilador: OFF"); EEPROM.update(0, 5); }
    else if (comando == c1_str && c1_str.length() > 0) { modoActual = "custom1"; mostrarMensajeLCD("Modo Custom:", custom1_nombre); EEPROM.update(0, 6); }
    else if (comando == c2_str && c2_str.length() > 0) { modoActual = "custom2"; mostrarMensajeLCD("Modo Custom:", custom2_nombre); EEPROM.update(0, 7); }
  }

  if (modoActual == "modo_fiesta") {
    analogWrite(pinMotor, 255);
    unsigned long tiempoActual = millis();
    if (tiempoActual - tiempoAnteriorFiesta >= intervaloFiesta) {
      tiempoAnteriorFiesta = tiempoActual;
      maxLucesFiesta();
    }
  }
  else if (modoActual == "encender_todo") {
    analogWrite(pinMotor, 255);
  }
  else if (modoActual == "modo_relajado" || modoActual == "modo_noche" || modoActual == "apagar_todo" || modoActual == "inicio") {
    analogWrite(pinMotor, 0);
    digitalWrite(pinMotor, LOW);
  }
  else if (modoActual == "custom1") {
    analogWrite(pinMotor, custom1_motor); 
    digitalWrite(sala, custom1_leds[0]);
    digitalWrite(comedor, custom1_leds[1]);
    digitalWrite(cocina, custom1_leds[2]);
    digitalWrite(bath, custom1_leds[3]);
    digitalWrite(habitacion, custom1_leds[4]);
  }
  else if (modoActual == "custom2") {
    analogWrite(pinMotor, custom2_motor); 
    digitalWrite(sala, custom2_leds[0]);
    digitalWrite(comedor, custom2_leds[1]);
    digitalWrite(cocina, custom2_leds[2]);
    digitalWrite(bath, custom2_leds[3]);
    digitalWrite(habitacion, custom2_leds[4]);
  }

  estadoBoton = digitalRead(pinBoton);
  if (estadoBoton == HIGH && ultimoEstadoBoton == LOW) {
    if (posicion == 0) { miServo.write(90); posicion = 1; } 
    else { miServo.write(0); posicion = 0; }
    delay(300); 
  }
  ultimoEstadoBoton = estadoBoton;
}

void encenderTodosLosLeds() {
  digitalWrite(sala, HIGH); digitalWrite(comedor, HIGH); digitalWrite(cocina, HIGH); digitalWrite(bath, HIGH); digitalWrite(habitacion, HIGH);
}

void apagarTodosLosLeds() {
  digitalWrite(sala, LOW); digitalWrite(comedor, LOW); digitalWrite(cocina, LOW); digitalWrite(bath, LOW); digitalWrite(habitacion, LOW);
}

void maxLucesFiesta() {
  estadoLucesFiesta = !estadoLucesFiesta;
  digitalWrite(sala, estadoLucesFiesta);
  digitalWrite(cocina, estadoLucesFiesta);
  digitalWrite(comedor, !estadoLucesFiesta);
  digitalWrite(bath, !estadoLucesFiesta);
  digitalWrite(habitacion, estadoLucesFiesta);
}

void mostrarMensajeLCD(String linea1, String linea2) {
  lcd.clear(); lcd.setCursor(0, 0); printString(linea1); lcd.setCursor(0, 1); printString(linea2);
}

void printString(String str) {
  for(int i=0; i<str.length(); i++) lcd.write(str[i]);
}