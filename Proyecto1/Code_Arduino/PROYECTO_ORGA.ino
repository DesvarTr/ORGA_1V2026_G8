// C++ code
//
#include <EEPROM.h>
#include <Servo.h>
#include <LiquidCrystal.h>

#define PIN_SERVO 9
#define PIN_BOTON 7
#define PIN_VENTILADOR 8

#define LED_AZUL 10
#define LED_VERDE 6
#define LED_ROJO A0


Servo puerta;

LiquidCrystal lcd(
  12,
  11,
  5,
  4,
  3,
  2
);



bool puertaAbierta = false;

bool ultimoEstadoBoton = HIGH;

#define LED_SALA       A1
#define LED_COMEDOR    A2
#define LED_COCINA     A3
#define LED_BANIO      A4
#define LED_HABITACION A5

bool modoFiestaActivo = false;

bool fiestaFase = false;

unsigned long ultimoCambioFiesta = 0;

const int ADDR_MODO = 0;

#define MODO_NINGUNO    0
#define MODO_FIESTA     1
#define MODO_RELAJADO   2
#define MODO_NOCHE      3
#define MODO_TODO_ON    4
#define MODO_TODO_OFF   5
#define MODO_CUSTOM1    6
#define MODO_CUSTOM2    7

void setup()
{
  Serial.begin(9600);

    // Servo
    puerta.attach(PIN_SERVO);

    // Botón
    pinMode(PIN_BOTON, INPUT_PULLUP);

    // Ventilador
    pinMode(PIN_VENTILADOR, OUTPUT);

    // LEDs Estado
    pinMode(LED_AZUL, OUTPUT);
    pinMode(LED_VERDE, OUTPUT);
    pinMode(LED_ROJO, OUTPUT);
  
  //LEDs sala, comedor, cocina, baño y habitación
  pinMode(LED_SALA, OUTPUT);
  pinMode(LED_COMEDOR, OUTPUT);
  pinMode(LED_COCINA, OUTPUT);
  pinMode(LED_BANIO, OUTPUT);
  pinMode(LED_HABITACION, OUTPUT);

    // LCD
    lcd.begin(16,2);

    puerta.write(0);

    iniciarSistema();
    restaurarModo();
    

   

    Serial.println("Sistema iniciado");
     
}

void loop()
{
    manejarPuerta();
     actualizarFiesta();
     leerBluetooth();
  
    

}

void manejarPuerta()
{
    bool estadoBoton =
        digitalRead(PIN_BOTON);

    if(
        ultimoEstadoBoton == HIGH &&
        estadoBoton == LOW
    )
    {
        if(puertaAbierta)
        {
            cerrarPuerta();
        }
        else
        {
            abrirPuerta();
        }

        delay(300);
    }

    ultimoEstadoBoton =
        estadoBoton;
}

void abrirPuerta()
{
    puerta.write(90);

    puertaAbierta = true;

    Serial.println("Puerta abierta");
}

void cerrarPuerta()
{
    puerta.write(0);

    puertaAbierta = false;

    Serial.println("Puerta cerrada");
}

void encenderVentilador()
{
    digitalWrite(
        PIN_VENTILADOR,
        HIGH
    );

    Serial.println("Ventilador ON");
}

void apagarVentilador()
{
    digitalWrite(
        PIN_VENTILADOR,
        LOW
    );

    Serial.println("Ventilador OFF");
}

void iniciarSistema() {
  digitalWrite(LED_AZUL, HIGH);   // Sistema listo
  digitalWrite(LED_VERDE, LOW);
  digitalWrite(LED_ROJO, LOW);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("SmartHome GT");
  lcd.setCursor(0, 1);
  lcd.print("Sistema activo");

  Serial.println("Sistema activo. Esperando comandos...");
}


// -------------------- MENSAJES LCD --------------------

void mostrarModoFiesta() {
  limpiarError();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Modo: FIESTA");
  lcd.setCursor(0, 1);
  encenderVentilador();
  lcd.print("VENT ON");
  

  Serial.println("Modo FIESTA activado");
}

void mostrarModoRelajado() {
  limpiarError();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Modo: RELAJADO");
  lcd.setCursor(0, 1);
  apagarVentilador();
  lcd.print("VENT OFF");

  Serial.println("Modo RELAJADO activado");
}

void mostrarModoNoche() {
  limpiarError();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Modo: NOCHE");
  lcd.setCursor(0, 1);
  apagarVentilador();
  lcd.print("VENT OFF");

  Serial.println("Modo NOCHE activado");
}

void mostrarEncenderTodo() {
  limpiarError();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("LEDs: ON");
  lcd.setCursor(0, 1);
  encenderVentilador();
  lcd.print("VENT ON");

  Serial.println("Todo encendido");
}

void mostrarApagarTodo() {
  limpiarError();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("LEDs: OFF");
  lcd.setCursor(0, 1);
  apagarVentilador();
  lcd.print("VENT OFF");

  Serial.println("Todo apagado");
}

void activarModoCustom1()
{
    modoFiestaActivo = false;

    encenderVentilador();

    digitalWrite(LED_SALA, HIGH);
    digitalWrite(LED_COMEDOR, HIGH);

    digitalWrite(LED_COCINA, LOW);
    digitalWrite(LED_BANIO, LOW);
    digitalWrite(LED_HABITACION, LOW);

    mostrarModoCustom(
        "Cena",
        true
    );

    indicarExito();
  
  guardarModo(
    MODO_CUSTOM1
);
  
}

void activarModoCustom2()
{
    modoFiestaActivo = false;

    apagarVentilador();

    digitalWrite(LED_SALA, LOW);
    digitalWrite(LED_COMEDOR, LOW);
    digitalWrite(LED_COCINA, HIGH);

    digitalWrite(LED_BANIO, LOW);
    digitalWrite(LED_HABITACION, HIGH);

    mostrarModoCustom(
        "Estudio",
        false
    );

    indicarExito();
  
  guardarModo(
    MODO_CUSTOM2
);
  
}

void mostrarModoCustom(String nombreModo, bool ventiladorON) {
  limpiarError();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Modo:");
  lcd.print(nombreModo);

  lcd.setCursor(0, 1);
  lcd.print("Vent:");

  if (ventiladorON) {
    lcd.print("ON");
  } else {
    lcd.print("OFF");
  }

  Serial.print("Modo custom activado: ");
  Serial.println(nombreModo);
}

void mostrarEstado() {
  limpiarError();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Sistema activo");
  lcd.setCursor(0, 1);
  lcd.print("Listo comandos");

  Serial.println("Estado: sistema activo y listo");
}

void mostrarError(String mensaje) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("ERROR:");
  lcd.setCursor(0, 1);
  lcd.print(mensaje);

  Serial.print("ERROR: ");
  Serial.println(mensaje);
}

void mostrarErrorArchivo() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Error archivo");
  lcd.setCursor(0, 1);
  lcd.print(".org invalido");

  indicarError();

  Serial.println("Error en archivo .org");
}

void mostrarConfiguracionGuardada() {
  limpiarError();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Configuracion");
  lcd.setCursor(0, 1);
  lcd.print("guardada");

  parpadearVerde(3);

  Serial.println("Configuracion guardada correctamente");
}

// -------------------- LEDS DE ESTADO --------------------

void indicarExito() {
  digitalWrite(LED_ROJO, LOW);
  digitalWrite(LED_AZUL, HIGH);

  digitalWrite(LED_VERDE, HIGH);

    delay(2000);

    digitalWrite(LED_VERDE, LOW);
}

void indicarError() {
  digitalWrite(LED_VERDE, LOW);
  digitalWrite(LED_ROJO, HIGH);
  digitalWrite(LED_AZUL, HIGH);
}

void limpiarError() {
  digitalWrite(LED_ROJO, LOW);
  digitalWrite(LED_AZUL, HIGH);
}

void parpadearVerde(int veces) {
  for (int i = 0; i < veces; i++) {
    digitalWrite(LED_VERDE, HIGH);
    delay(200);
    digitalWrite(LED_VERDE, LOW);
    delay(200);
  }
}
//Funciones Matriz de LEDs y Modos


void apagarTodosLosLEDs()
{
    digitalWrite(LED_SALA, LOW);
    digitalWrite(LED_COMEDOR, LOW);
    digitalWrite(LED_COCINA, LOW);
    digitalWrite(LED_BANIO, LOW);
    digitalWrite(LED_HABITACION, LOW);
}

void encenderTodosLosLEDs()
{
    digitalWrite(LED_SALA, HIGH);
    digitalWrite(LED_COMEDOR, HIGH);
    digitalWrite(LED_COCINA, HIGH);
    digitalWrite(LED_BANIO, HIGH);
    digitalWrite(LED_HABITACION, HIGH);
}

void activarFiestaLEDs()
{
    modoFiestaActivo = true;
}

void actualizarFiesta()
{
    if(!modoFiestaActivo)
    {
        return;
    }

    if(millis() - ultimoCambioFiesta >= 500)
    {
        ultimoCambioFiesta = millis();

        fiestaFase = !fiestaFase;

        if(fiestaFase)
        {
            digitalWrite(LED_SALA, HIGH);
            digitalWrite(LED_COCINA, HIGH);
            digitalWrite(LED_HABITACION, HIGH);

            digitalWrite(LED_COMEDOR, LOW);
            digitalWrite(LED_BANIO, LOW);
        }
        else
        {
            digitalWrite(LED_SALA, LOW);
            digitalWrite(LED_COCINA, LOW);
            digitalWrite(LED_HABITACION, LOW);

            digitalWrite(LED_COMEDOR, HIGH);
            digitalWrite(LED_BANIO, HIGH);
        }
    }
}

// ========================================
// MODOS DEL SISTEMA
// ========================================


void activarModoFiesta()
{
    modoFiestaActivo = true;

    encenderVentilador();

    mostrarModoFiesta();

    indicarExito();
  
   guardarModo(
        MODO_FIESTA
    );
}

void activarModoRelajado()
{
   modoFiestaActivo = false;

    apagarVentilador();

    apagarTodosLosLEDs();

    mostrarModoRelajado();

    indicarExito();

    guardarModo(MODO_RELAJADO);
}

void activarModoNoche()
{
   modoFiestaActivo = false;

    apagarVentilador();

    apagarTodosLosLEDs();

    mostrarModoNoche();

    indicarExito();

    guardarModo(MODO_NOCHE);
  
}

void encenderTodo()
{
    modoFiestaActivo = false;

    encenderVentilador();

    encenderTodosLosLEDs();

    mostrarEncenderTodo();

    indicarExito();
  
  guardarModo(
    MODO_TODO_ON
);
  
}

void apagarTodo()
{
    modoFiestaActivo = false;

    apagarVentilador();

    apagarTodosLosLEDs();

    mostrarApagarTodo();

    indicarExito();
  
  guardarModo(
    MODO_TODO_OFF
);
  
}

void guardarModo(byte modo)
{
    EEPROM.update(
        ADDR_MODO,
        modo
    );
}

void restaurarModo()
{
    byte modo =
        EEPROM.read(ADDR_MODO);

    switch(modo)
    {
        case MODO_FIESTA:
            activarModoFiesta();
            break;

        case MODO_RELAJADO:
            activarModoRelajado();
            break;

        case MODO_NOCHE:
            activarModoNoche();
            break;

        case MODO_TODO_ON:
            encenderTodo();
            break;

        case MODO_TODO_OFF:
            apagarTodo();
            break;

        case MODO_CUSTOM1:
            activarModoCustom1();
            break;

        case MODO_CUSTOM2:
            activarModoCustom2();
            break;
    }
}

void leerBluetooth()
{
    if (Serial.available())
    {
        String comando =
            Serial.readStringUntil('\n');

        comando.trim();

        procesarComando(comando);
    }
}

void procesarComando(String comando)
{
    comando.toLowerCase();

    if(comando == "modo_fiesta")
    {
        activarModoFiesta();
    }
    else if(comando == "modo_relajado")
    {
        activarModoRelajado();
    }
    else if(comando == "modo_noche")
    {
        activarModoNoche();
    }
    else if(comando == "encender_todo")
    {
        encenderTodo();
    }
    else if(comando == "apagar_todo")
    {
        apagarTodo();
    }
    else if(comando == "modo_custom_1")
    {
        activarModoCustom1();
    }
    else if(comando == "modo_custom_2")
    {
        activarModoCustom2();
    }
    else if(comando == "estado")
    {
        mostrarEstado();
    }
    else
    {
    mostrarError("Modo inval");
    indicarError();
    }
}













