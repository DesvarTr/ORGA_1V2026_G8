#include <EEPROM.h>
#include <Servo.h>
#include <LiquidCrystal.h>

const int PIN_SERVO = 9;
const int PIN_BOTON = 7;
const int PIN_VENTILADOR = 8;

const int LED_AZUL = 10;
const int LED_VERDE = 6;
const int LED_ROJO = A0;

const int LED_SALA = A1;
const int LED_COMEDOR = A2;
const int LED_COCINA = A3;
const int LED_BANIO = A4;
const int LED_HABITACION = A5;

#define MODO_FIESTA     1
#define MODO_RELAJADO   2
#define MODO_NOCHE      3
#define MODO_TODO_ON    4
#define MODO_TODO_OFF   5
#define MODO_CUSTOM1    6
#define MODO_CUSTOM2    7

Servo puerta;

LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

struct ConfigModo
{
    byte ventilador;

    byte sala;
    byte comedor;
    byte cocina;
    byte banio;
    byte habitacion;
};

struct ConfigCustom
{
    char nombre[11];

    ConfigModo config;
};



const int ADDR_FIESTA = 0;

const int ADDR_RELAJADO =
ADDR_FIESTA +
sizeof(ConfigModo);

const int ADDR_NOCHE =
ADDR_RELAJADO +
sizeof(ConfigModo);

const int ADDR_ENCENDER =
ADDR_NOCHE +
sizeof(ConfigModo);

const int ADDR_APAGAR =
ADDR_ENCENDER +
sizeof(ConfigModo);

const int ADDR_CUSTOM1 =
ADDR_APAGAR +
sizeof(ConfigModo);

const int ADDR_CUSTOM2 =
ADDR_CUSTOM1 +
sizeof(ConfigCustom);

const int ADDR_ULTIMO_MODO =
ADDR_CUSTOM2 +
sizeof(ConfigCustom);

ConfigModo modoFiesta;
ConfigModo modoRelajado;
ConfigModo modoNoche;
ConfigModo modoEncenderTodo;
ConfigModo modoApagarTodo;

ConfigCustom custom1;
ConfigCustom custom2;

bool puertaAbierta = false;

bool modoFiestaActivo = false;

unsigned long tiempoFiesta = 0;

bool faseFiesta = false;

bool cargandoOrg = false;

bool ultimoEstadoBoton = HIGH;

String modoActualOrg = "";

ConfigModo configTemporal;

bool leyendoCustom = false;
bool esperandoVentilador = false;
bool esperandoLeds = false;
bool configuracionValida = true;

enum EstadoParser
{
    ESPERANDO_INICIO,

    LEYENDO_ARCHIVO,

    LEYENDO_CUSTOM
};

EstadoParser estadoParser =
ESPERANDO_INICIO;

String lineaActual = "";

String modoActual = "";

void setup()
{
    Serial.begin(9600);
    delay(1000);
    Serial.println("SERIAL OK");

    Serial.println("ANTES LCD");
    lcd.begin(16, 2);
    Serial.println("DESPUES LCD");

    //lcd.backlight();
    Serial.println("DESPUES BACKLIGHT");

    puerta.attach(PIN_SERVO);
    Serial.println("DESPUES SERVO");

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

  digitalWrite(
    LED_AZUL,
    HIGH
);

puerta.write(0);

inicializarEEPROM();
cargarConfiguraciones();
restaurarModo();

}

void loop()
{
    manejarPuerta();
    actualizarModoFiesta();

    if(Serial.available())
    {
        Serial.println("HAY DATOS EN SERIAL");
    }

    if(cargandoOrg)
    {
        procesarArchivoOrg();
    }
    else
    {
        leerBluetooth();
    }
}

void inicializarEEPROM()
{
    if(EEPROM.read(ADDR_FIESTA) == 255)
    {
        ConfigModo fiesta =
        {
            1,
            1,1,1,1,1
        };

        ConfigModo relajado =
        {
            0,
            0,0,0,0,0
        };

        ConfigModo noche =
        {
            0,
            0,0,0,0,0
        };

        ConfigModo encender =
        {
            1,
            1,1,1,1,1
        };

        ConfigModo apagar =
        {
            0,
            0,0,0,0,0
        };

        EEPROM.put(
            ADDR_FIESTA,
            fiesta
        );

        EEPROM.put(
            ADDR_RELAJADO,
            relajado
        );

        EEPROM.put(
            ADDR_NOCHE,
            noche
        );

        EEPROM.put(
            ADDR_ENCENDER,
            encender
        );

        EEPROM.put(
            ADDR_APAGAR,
            apagar
        );
      
       // ---------- CUSTOM 1 ----------
        strcpy(
            custom1.nombre,
            "Custom1"
        );

        custom1.config =
        {
            0,
            0,0,0,0,0
        };

        EEPROM.put(
            ADDR_CUSTOM1,
            custom1
        );

        // ---------- CUSTOM 2 ----------
        strcpy(
            custom2.nombre,
            "Custom2"
        );

        custom2.config =
        {
            0,
            0,0,0,0,0
        };

        EEPROM.put(
            ADDR_CUSTOM2,
            custom2
        );

    }



}

void cargarConfiguraciones()
{
    EEPROM.get(
        ADDR_FIESTA,
        modoFiesta
    );

    EEPROM.get(
        ADDR_RELAJADO,
        modoRelajado
    );

    EEPROM.get(
        ADDR_NOCHE,
        modoNoche
    );

    EEPROM.get(
        ADDR_ENCENDER,
        modoEncenderTodo
    );

    EEPROM.get(
        ADDR_APAGAR,
        modoApagarTodo
    );

    EEPROM.get(
        ADDR_CUSTOM1,
        custom1
    );

    EEPROM.get(
        ADDR_CUSTOM2,
        custom2
    );
}

void guardarModo(
    byte modo
)
{
    EEPROM.update(
        ADDR_ULTIMO_MODO,
        modo
    );
}

void restaurarModo()
{
    byte modo =
        EEPROM.read(
            ADDR_ULTIMO_MODO
        );

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

void aplicarConfiguracion(
    ConfigModo cfg
)
{
    digitalWrite(
        LED_SALA,
        cfg.sala
    );

    digitalWrite(
        LED_COMEDOR,
        cfg.comedor
    );

    digitalWrite(
        LED_COCINA,
        cfg.cocina
    );

    digitalWrite(
        LED_BANIO,
        cfg.banio
    );

    digitalWrite(
        LED_HABITACION,
        cfg.habitacion
    );

    if(cfg.ventilador)
    {
        encenderVentilador();
    }
    else
    {
        apagarVentilador();
    }
}

void guardarConfiguracion()
{
    if(modoActual == "modo_fiesta")
    {
        EEPROM.put(
            ADDR_FIESTA,
            configTemporal
        );

        modoFiesta = configTemporal;
    }

    else if(modoActual == "modo_relajado")
    {
        EEPROM.put(
            ADDR_RELAJADO,
            configTemporal
        );

        modoRelajado = configTemporal;
    }

    else if(modoActual == "modo_noche")
    {
        EEPROM.put(
            ADDR_NOCHE,
            configTemporal
        );

        modoNoche = configTemporal;
    }

    else if(modoActual == "modo_encender_todo")
    {
        EEPROM.put(
            ADDR_ENCENDER,
            configTemporal
        );

        modoEncenderTodo = configTemporal;
    }

    else if(modoActual == "modo_apagar_todo")
    {
        EEPROM.put(
            ADDR_APAGAR,
            configTemporal
        );

        modoApagarTodo = configTemporal;
    }

    Serial.println(
        "Configuracion guardada."
    );

      indicarExito();

}

// ========================================
// VENTILADOR
// ========================================

void encenderVentilador()
{
    digitalWrite(
        PIN_VENTILADOR,
        HIGH
    );
}

void apagarVentilador()
{
    digitalWrite(
        PIN_VENTILADOR,
        LOW
    );
}

// ========================================
// PUERTA
// ========================================

void abrirPuerta()
{
    puerta.write(90);

    puertaAbierta = true;
}

void cerrarPuerta()
{
    puerta.write(0);

    puertaAbierta = false;
}

void manejarPuerta()
{
    bool estadoBoton =
        digitalRead(
            PIN_BOTON
        );

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

        delay(50);
    }

    ultimoEstadoBoton =
        estadoBoton;
}

// ========================================
// BLUETOOTH
// ========================================

void leerBluetooth()
{
    if(!Serial.available())
    {
        return;
    }

    String comando =
        Serial.readStringUntil('\n');

    comando.trim();
    Serial.print("Comando recibido: ");
    Serial.println(comando);

    procesarComando(
        comando
    );
}

//======================================================
// PARSER ORG
//======================================================

void procesarArchivoOrg()
{
    if(!Serial.available())
    {
        return;
    }

    lineaActual =
        Serial.readStringUntil('\n');

    lineaActual.trim();

    if(
        lineaActual.length() == 0
    )
    {
        return;
    }

    if(
        lineaActual.startsWith("//")
    )
    {
        return;
    }

    procesarLinea();
}

void procesarLinea()
{
    switch(estadoParser)
    {
        case ESPERANDO_INICIO:

            procesarInicio();

            break;

        case LEYENDO_ARCHIVO:

            procesarModo();

            break;

        case LEYENDO_CUSTOM:

            procesarCustom();

            break;
    }
}

void procesarCustom()
{
    if(esperandoVentilador)
    {
        leerVentilador();

        return;
    }

    if(esperandoLeds)
    {
        leerLeds();

        return;
    }

    indicarError();

    Serial.println("ERROR: custom incompleto");

    estadoParser = LEYENDO_ARCHIVO;
}

void guardarCustom()
{
    if(modoActual == "modo_custom_1")
    {
        custom1.config = configTemporal;

        EEPROM.put(
            ADDR_CUSTOM1,
            custom1
        );
    }

    else if(modoActual == "modo_custom_2")
    {
        custom2.config = configTemporal;

        EEPROM.put(
            ADDR_CUSTOM2,
            custom2
        );
    }

    Serial.println(
        "Modo personalizado guardado."
    );

    indicarExito();

}

void extraerNombreCustom(byte numero)
{
    int inicio =
        lineaActual.indexOf('"');

    int fin =
        lineaActual.lastIndexOf('"');

    if(inicio == -1 || fin == -1)
    {
        return;
    }

    String nombre =
        lineaActual.substring(
            inicio + 1,
            fin
        );

    if(numero == 1)
    {
        nombre.toCharArray(
            custom1.nombre,
            11
        );
    }
    else
    {
        nombre.toCharArray(
            custom2.nombre,
            11
        );
    }
}

void procesarInicio()
{
    if(lineaActual == "conf_ini")
    {
        estadoParser =
            LEYENDO_ARCHIVO;

        Serial.println(
            "Inicio correcto"
        );
    }
   else
 {
     indicarError();

     Serial.println(
        "ERROR: Se esperaba conf_ini"
     );
 }
    

}

void procesarModo()
{
    if(lineaActual == "conf:fin")
    {
        estadoParser = ESPERANDO_INICIO;

        cargandoOrg = false;

        Serial.println("Configuracion cargada.");

        return;
    }

    if(lineaActual.startsWith("modo_custom_1"))
   {
     modoActual = "modo_custom_1";

     extraerNombreCustom(1);

     estadoParser = LEYENDO_CUSTOM;

     esperandoVentilador = true;
     esperandoLeds = false;

     return;
   }

   if(lineaActual.startsWith("modo_custom_2"))
   {
     modoActual = "modo_custom_2";

     extraerNombreCustom(2);

     estadoParser = LEYENDO_CUSTOM;

     esperandoVentilador = true;
     esperandoLeds = false;

     return;
   }

    if(lineaActual.startsWith("modo_"))
   {
     modoActual = lineaActual;

     esperandoVentilador = true;
     esperandoLeds = false;

     return;
   }

    if(esperandoVentilador)
    {
        leerVentilador();

        return;
    }

    if(esperandoLeds)
    {
        leerLeds();

        return;
    }

   indicarError();

    Serial.println("ERROR: comando invalido");


}

void leerVentilador()
{
    if(!lineaActual.startsWith("Ventilador:"))
    {
        return;
    }

    String valor =
        lineaActual.substring(11);

    valor.trim();

    Serial.print("Valor ventilador: ");
    Serial.println(valor);

    if(valor == "on")
    {
        configTemporal.ventilador = 1;
    }
    else if(valor == "off")
    {
        configTemporal.ventilador = 0;
    }
    else
    {
        Serial.println("ERROR: Ventilador invalido");
        indicarError();

        return;
    }

    esperandoVentilador = false;
    esperandoLeds = true;

    Serial.println("Ventilador OK");
}

void leerLeds()
{
    if(!lineaActual.startsWith("LED'S:"))
    {
        return;
    }

    String datos =
        lineaActual.substring(6);

    datos.trim();

    parsearLeds(datos);

    esperandoLeds = false;

    if(
        modoActual == "modo_custom_1" ||
        modoActual == "modo_custom_2"
    )
    {
        guardarCustom();

        estadoParser = LEYENDO_ARCHIVO;
    }
    else
    {
        guardarConfiguracion();
    }

    Serial.println("LEDS OK");
}

void parsearLeds(
    String datos
)
{
    configTemporal.sala =
        datos.indexOf("sala:on") != -1;

    configTemporal.comedor =
        datos.indexOf("comedor:on") != -1;

    configTemporal.cocina =
        datos.indexOf("cocina:on") != -1;

    configTemporal.banio =
        datos.indexOf("bano:on") != -1;

    configTemporal.habitacion =
        datos.indexOf("habitacion:on") != -1;
}



// ========================================
// LCD
// ========================================


void mostrarModoLCD(
    String linea1,
    String linea2
)
{
    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print(linea1);

    lcd.setCursor(0, 1);
    lcd.print(linea2);

    Serial.println(linea1);
    Serial.println(linea2);
}

void mostrarModoFiesta()
{
    lcd.clear();

    lcd.setCursor(0,0);
    lcd.print("Modo: FIESTA");

    lcd.setCursor(0,1);
    lcd.print("Ventilador: ON");
}

void mostrarModoRelajado()
{
    lcd.clear();

    lcd.setCursor(0,0);
    lcd.print("Modo: RELAJADO");

    lcd.setCursor(0,1);
    lcd.print("Ventilador: OFF");
}

void mostrarModoNoche()
{
    lcd.clear();

    lcd.setCursor(0,0);
    lcd.print("Modo: NOCHE");

    lcd.setCursor(0,1);
    lcd.print("Ventilador: OFF");
}

void mostrarEncenderTodo()
{
    lcd.clear();

    lcd.setCursor(0,0);
    lcd.print("ENCENDER TODO");

    lcd.setCursor(0,1);
    lcd.print("Ventilador: ON");
}

void mostrarApagarTodo()
{
    lcd.clear();

    lcd.setCursor(0,0);
    lcd.print("APAGAR TODO");

    lcd.setCursor(0,1);
    lcd.print("Ventilador: OFF");
}

void mostrarModoCustom(
    String nombre,
    bool ventilador
)
{
    lcd.clear();

    lcd.setCursor(0,0);
    lcd.print(nombre);

    lcd.setCursor(0,1);

    if(ventilador)
    {
        lcd.print("Ventilador ON");
    }
    else
    {
        lcd.print("Ventilador OFF");
    }
}

void mostrarError(
    String mensaje
)
{
    lcd.clear();

    lcd.setCursor(0,0);
    lcd.print("ERROR");

    lcd.setCursor(0,1);
    lcd.print(mensaje);
}

// ========================================
// INDICADORES
// ========================================

void limpiarIndicadores()
{
    digitalWrite(
        LED_VERDE,
        LOW
    );

    digitalWrite(
        LED_ROJO,
        LOW
    );
}



// ========================================
// MODOS
// ========================================

void activarModoFiesta()
{
    modoFiestaActivo = true;

    tiempoFiesta = millis();

    faseFiesta = false;

    encenderVentilador();

    guardarModo(
        MODO_FIESTA
    );

    mostrarModoLCD(
        "Modo FIESTA",
        "Vent: ON"
    );
}

void activarModoRelajado()
{
    modoFiestaActivo = false;

    aplicarConfiguracion(
        modoRelajado
    );

    guardarModo(
        MODO_RELAJADO
    );

    mostrarModoLCD(
        "Modo RELAJADO",
        "Vent: OFF"
    );
}

void activarModoNoche()
{
    modoFiestaActivo = false;

    aplicarConfiguracion(
        modoNoche
    );

    guardarModo(
        MODO_NOCHE
    );

    mostrarModoLCD(
        "Modo NOCHE",
        "Vent: OFF"
    );
}

void encenderTodo()
{
    modoFiestaActivo = false;

    aplicarConfiguracion(
        modoEncenderTodo
    );

    guardarModo(
        MODO_TODO_ON
    );

    mostrarModoLCD(
        "LED'S ON",
        "Vent: ON"
    );
}

void apagarTodo()
{
    modoFiestaActivo = false;

    aplicarConfiguracion(
        modoApagarTodo
    );

    guardarModo(
        MODO_TODO_OFF
    );

    mostrarModoLCD(
        "LED'S OFF",
        "Vent: OFF"
    );
}

void activarModoCustom1()
{
    modoFiestaActivo = false;

    aplicarConfiguracion(
        custom1.config
    );

    guardarModo(
        MODO_CUSTOM1
    );

    mostrarModoLCD(
        custom1.nombre,
        custom1.config.ventilador ?
        "Vent: ON" :
        "Vent: OFF"
    );
}

void activarModoCustom2()
{
    modoFiestaActivo = false;

    aplicarConfiguracion(
        custom2.config
    );

    guardarModo(
        MODO_CUSTOM2
    );

    mostrarModoLCD(
        custom2.nombre,
        custom2.config.ventilador ?
        "Vent: ON" :
        "Vent: OFF"
    );
}


void procesarComando(
    String comando
)
{
    if(
        comando ==
        "modo_fiesta"
    )
    {
        activarModoFiesta();
    }

    else if(
        comando ==
        "modo_relajado"
    )
    {
        activarModoRelajado();
    }

    else if(
        comando ==
        "modo_noche"
    )
    {
        activarModoNoche();
    }

    else if(
        comando ==
        "encender_todo"
    )
    {
        encenderTodo();
    }

    else if(
        comando ==
        "apagar_todo"
    )
    {
        apagarTodo();
    }

    else if(
        comando ==
        "modo_custom_1"
    )
    {
        activarModoCustom1();
    }

    else if(
        comando ==
        "modo_custom_2"
    )
    {
        activarModoCustom2();
    }

    else if(
        comando ==
        "estado"
    )
    {
        Serial.println(
            "Sistema activo"
        );
    }

    else if(
        comando ==
        "cargar_org"
    )
    {
        cargandoOrg = true;

        Serial.println(
            "Modo carga activado"
        );
    }

    else
    {
        Serial.println(
            "ERROR: comando invalido"
        );
    }
}


// ========================================
// ANIMACION FIESTA
// ========================================

void actualizarModoFiesta()
{
    if(!modoFiestaActivo)
    {
        return;
    }

    if(
        millis() - tiempoFiesta < 500
    )
    {
        return;
    }

    tiempoFiesta = millis();

    faseFiesta = !faseFiesta;

    digitalWrite(
        LED_SALA,
        faseFiesta
    );

    digitalWrite(
        LED_COMEDOR,
        !faseFiesta
    );

    digitalWrite(
        LED_COCINA,
        faseFiesta
    );

    digitalWrite(
        LED_BANIO,
        !faseFiesta
    );

    digitalWrite(
        LED_HABITACION,
        faseFiesta
    );
}

// ========================================
// INDICADORES DE ESTADO
// ========================================

void indicarExito()
{
    digitalWrite(
        LED_VERDE,
        HIGH
    );

    delay(150);

    digitalWrite(
        LED_VERDE,
        LOW
    );
}

void indicarError()
{
    for(int i = 0; i < 3; i++)
    {
        digitalWrite(
            LED_ROJO,
            HIGH
        );

        delay(120);

        digitalWrite(
            LED_ROJO,
            LOW
        );

        delay(120);
    }
}
