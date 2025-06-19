#include <Wire.h> // Librería para I2C
#include <LiquidCrystal_I2C.h> // Librería para LCD con I2C
#include <Keypad.h>
#include <SD.h>
#include <TMRpcm.h>
#include <SPI.h>

// Configuración del LCD con I2C (dirección 0x27, 16 columnas y 2 filas)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Configuración del teclado
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[ROWS] = {2, 3, 4, 5};
byte colPins[COLS] = {6, 7, 8, 1};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// Configuración del audio
TMRpcm tmrpcm;
const int speakerPin = 9;

// Configuración del LED
const int ledPin = A3; // Pin del LED

// Variables globales
const char codigoCorrecto[7] = "762189"; // Código correcto
char codigoIngresado[7] = ""; // Buffer para el código ingresado
bool cuentaActiva = false; // Estado de la cuenta regresiva
unsigned long tiempoInicioCuenta = 0; // Tiempo de inicio de la cuenta regresiva
uint16_t tiempoRestante = 0; // Tiempo restante en segundos
uint16_t tiempoPersonalizado = 0; // Tiempo personalizado en segundos
bool modoDesactivacionActivado = false; // Modo desactivación activado
bool modoContrarrelojActivado = false; // Modo contrarreloj activado

// Variables para controlar el tiempo sin usar delay()
unsigned long previousMillis = 0;
const uint16_t interval = 1000; // Intervalo de 1 segundo

// Variables para el pitido sin delay()
unsigned long ultimoPitido = 0; // Almacena el último momento en que se hizo un pitido

// Variables estáticas para reducir el uso de stack
static char tiempoIngresado[6] = ""; // Buffer para almacenar el tiempo ingresado
static uint8_t indiceTiempo = 0; // Índice para el buffer de tiempo ingresado
static uint8_t indiceCodigo = 0; // Índice para el buffer de código ingresado

void beep() {
  tone(speakerPin, 1000, 100); // Pitido de 900 Hz por 100 ms
  digitalWrite(ledPin, HIGH); // Encender el LED
  delay(100); // Mantener el LED encendido durante el pitido
  digitalWrite(ledPin, LOW); // Apagar el LED
}

void colocado() {
  noTone(speakerPin); // Detener cualquier tono antes de reproducir el audio
  tmrpcm.play("test1.wav");
  delay(6000);
}

void susto() {
  noTone(speakerPin); // Detener cualquier tono antes de reproducir el audio
  tmrpcm.play("test4.wav");
  delay(1000);
}

void explosion() {
  noTone(speakerPin); // Detener cualquier tono antes de reproducir el audio
  tmrpcm.play("test3.wav");
  delay(8000);
}

void desactivado() {
  noTone(speakerPin); // Detener cualquier tono antes de reproducir el audio
  tmrpcm.play("test2.wav");
  delay(6000);
}

void setup() {
  Serial.begin(9600);

  // Inicializar el LCD con I2C
  lcd.init(); // Inicializar el LCD
  lcd.backlight(); // Encender la retroiluminación

  lcd.setCursor(0, 0);
  lcd.print("[Top Shot]  ");
  lcd.setCursor(0, 1);
  lcd.print("The Competition");

  tmrpcm.speakerPin = speakerPin;
  tmrpcm.setVolume(5);

  // Inicializar la tarjeta SD
  SD.begin(10);

  // Configurar el pin del LED como salida
  pinMode(ledPin, OUTPUT);

  // Pitido de inicio
  tone(speakerPin, 98, 500);
  digitalWrite(ledPin, HIGH); // Encender el LED
  delay(200);
  tone(speakerPin, 110, 500);
  delay(200);
  tone(speakerPin, 123, 500);
  delay(2000);
  digitalWrite(ledPin, LOW); // Apagar el LED
}

void loop() {
  if (cuentaActiva) {
    actualizarCuentaRegresiva();
    return;
  }

  unsigned long currentMillis = millis();
  char key = keypad.getKey();

  lcd.setCursor(0, 0);
  lcd.print("1.Desactivacion");
  lcd.setCursor(0, 1);
  lcd.print("2.Contrarreloj ");

  if (key) {
    beep();
    switch (key) {
      case '1':
        lcd.clear();
        modoDesactivacion();
        break;
      case '2':
        lcd.clear();
        modoContrarreloj();
        break;
    }
  }
}

void modoDesactivacion() {
  lcd.setCursor(0, 0);
  lcd.print("Ingrese codigo:");
  lcd.setCursor(0, 1);
  lcd.print("> ");
  indiceCodigo = 0;

  while (true) {
    char key = keypad.getKey();
    if (key) {
      beep();
      if (key == '*') {
        // Borrar último dígito
        if (indiceCodigo > 0) {
          indiceCodigo--;
          memset(codigoIngresado, 0, sizeof(codigoIngresado)); // Limpiar buffer
          codigoIngresado[indiceCodigo] = '\0';
          lcd.setCursor(2 + indiceCodigo, 1);
          lcd.print(" ");
        }
      } else if (key == '#') {
        // Confirmar código
        if (strcmp(codigoIngresado, codigoCorrecto) == 0) {
          lcd.clear();
          lcd.print("Codigo correcto!");
          delay(1000);
          cuentaActiva = true;
          tiempoRestante = 120; // 2 minutos por defecto
          tiempoInicioCuenta = millis();
          colocado(); // Reproducir audio de activación
          lcd.clear();
          indiceCodigo = 0;
          memset(codigoIngresado, 0, sizeof(codigoIngresado)); // Limpiar buffer
          return;
        } else {
          lcd.clear();
          lcd.print("Codigo incorrecto!");
          susto(); // Reproducir audio de susto
          delay(1000);
          lcd.clear();
          lcd.print("Ingrese codigo:");
          lcd.setCursor(0, 1);
          lcd.print("> ");
          indiceCodigo = 0;
          memset(codigoIngresado, 0, sizeof(codigoIngresado));
        }
      } else if (indiceCodigo < 6) {
        // Agregar dígito al código
        codigoIngresado[indiceCodigo] = key;
        lcd.setCursor(2 + indiceCodigo, 1);
        lcd.print(key);
        indiceCodigo++;
      }
    }
  }
}

void modoContrarreloj() {
  lcd.setCursor(0, 0);
  lcd.print("Ingrese tiempo:");
  lcd.setCursor(0, 1);
  lcd.print("> ");
  indiceTiempo = 0;
  memset(tiempoIngresado, 0, sizeof(tiempoIngresado)); // Limpiar buffer

  while (true) {
    char key = keypad.getKey();
    if (key) {
      beep();
      if (key == '*') {
        // Borrar último dígito
        if (indiceTiempo > 0) {
          indiceTiempo--;
          tiempoIngresado[indiceTiempo] = '\0';
          lcd.setCursor(2 + indiceTiempo, 1);
          lcd.print(" ");
        }
      } else if (key == '#') {
        // Confirmar tiempo
        tiempoPersonalizado = atoi(tiempoIngresado) * 60; // Convertir a segundos
        if (tiempoPersonalizado > 0) {
          lcd.clear();
          lcd.print("Tiempo listo!");
          delay(1000);
          memset(codigoIngresado, 0, sizeof(codigoIngresado));
          cuentaActiva = true;
          tiempoRestante = tiempoPersonalizado;
          tiempoInicioCuenta = millis();
          colocado(); // Reproducir audio de activación
          lcd.clear();
          return;
        } else {
          lcd.clear();
          lcd.print("Tiempo invalido!");
          delay(1000);
          lcd.clear();
          lcd.print("Ingrese tiempo:");
          lcd.setCursor(0, 1);
          lcd.print("> ");
          indiceTiempo = 0;
          memset(tiempoIngresado, 0, sizeof(tiempoIngresado));
        }
      } else if (indiceTiempo < 5) {
        // Agregar dígito al tiempo
        tiempoIngresado[indiceTiempo] = key;
        lcd.setCursor(2 + indiceTiempo, 1);
        lcd.print(key);
        indiceTiempo++;
      }
    }
  }
}

// Variables para controlar los pitidos con millis()
unsigned long ultimoPitidoMillis = 0; // Momento del último pitido
bool pitidoActivo = false; // Indica si el pitido está sonando

void actualizarCuentaRegresiva() {
  unsigned long currentMillis = millis();

  // Actualizar el tiempo restante cada segundo
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    tiempoRestante--;

    // Mostrar tiempo restante en la LCD
    lcd.setCursor(0, 0);
    lcd.print("Tiempo: ");
    lcd.print(tiempoRestante / 60); // Minutos
    lcd.print(":");
    if (tiempoRestante % 60 < 10) lcd.print("0"); // Segundos con dos dígitos
    lcd.print(tiempoRestante % 60);

    // Verificar si el tiempo ha llegado a 0
    if (tiempoRestante == 0) {
      explosion(); // Reproducir audio de explosión
      pitidosIntermitentes(); // Iniciar pitidos intermitentes
      cuentaActiva = false;
      return;
    }
  }

  // Controlar los pitidos según el tiempo restante
  if (tiempoRestante <= 5) {
    // Pitido cuádruple (4 pitidos rápidos)
    if (currentMillis - ultimoPitidoMillis >= 250) { // Cada 250 ms
      ultimoPitidoMillis = currentMillis;
      tone(speakerPin, 900, 100); // Pitido de 100 ms
      digitalWrite(ledPin, HIGH); // Encender el LED
      delay(100); // Mantener el LED encendido durante el pitido
      digitalWrite(ledPin, LOW); // Apagar el LED
    }
  } else if (tiempoRestante <= 10) {
    // Pitido triple (3 pitidos rápidos)
    if (currentMillis - ultimoPitidoMillis >= 333) { // Cada 333 ms
      ultimoPitidoMillis = currentMillis;
      tone(speakerPin, 900, 100); // Pitido de 100 ms
      digitalWrite(ledPin, HIGH); // Encender el LED
      delay(100); // Mantener el LED encendido durante el pitido
      digitalWrite(ledPin, LOW); // Apagar el LED
    }
  } else if (tiempoRestante <= 30) {
    // Pitido doble (2 pitidos rápidos)
    if (currentMillis - ultimoPitidoMillis >= 500) { // Cada 500 ms
      ultimoPitidoMillis = currentMillis;
      tone(speakerPin, 900, 100); // Pitido de 100 ms
      digitalWrite(ledPin, HIGH); // Encender el LED
      delay(100); // Mantener el LED encendido durante el pitido
      digitalWrite(ledPin, LOW); // Apagar el LED
    }
  } else {
    // Pitido simple (1 pitido por segundo)
    if (currentMillis - ultimoPitidoMillis >= 1000) { // Cada 1000 ms
      ultimoPitidoMillis = currentMillis;
      tone(speakerPin, 900, 100); // Pitido de 100 ms
      digitalWrite(ledPin, HIGH); // Encender el LED
      delay(100); // Mantener el LED encendido durante el pitido
      digitalWrite(ledPin, LOW); // Apagar el LED
    }
  }

  // Verificar si se ingresa un código para desactivar
  char key = keypad.getKey();
  if (key) {
    beep();
    if (key == '*') {
      // Borrar último dígito
      if (indiceCodigo > 0) {
        indiceCodigo--;
        codigoIngresado[indiceCodigo] = '\0';
        lcd.setCursor(0, 1);
        lcd.print("                "); // Limpiar la línea
        lcd.setCursor(0, 1);
        lcd.print("> ");
        lcd.print(codigoIngresado); // Mostrar el código actualizado
      }
    } else if (key == '#') {
      // Confirmar código
      if (strcmp(codigoIngresado, codigoCorrecto) == 0) {
        lcd.clear();
        lcd.print("Desactivado!");
        desactivado(); // Reproducir audio de desactivado
        cuentaActiva = false;
        return;
      } else {
        lcd.clear();
        lcd.print("Codigo incorrecto!");
        susto(); // Reproducir audio de susto
        lcd.clear();
        delay(1000);

        // Reiniciar el buffer de código ingresado
        memset(codigoIngresado, 0, sizeof(codigoIngresado)); // Limpiar buffer
        indiceCodigo = 0; // Reiniciar índice

        lcd.setCursor(0, 1);
        lcd.print("> "); // Mostrar prompt para ingresar código
      }
    } else if (indiceCodigo < 6) {
      // Agregar dígito al código
      codigoIngresado[indiceCodigo] = key;
      indiceCodigo++;
      lcd.setCursor(0, 1);
      lcd.print("> ");
      lcd.print(codigoIngresado); // Mostrar el código actualizado
    }
  }
}

void pitidosIntermitentes() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Presione una");
  lcd.setCursor(0, 1);
  lcd.print("tecla... ");
  bool pitidoAgudo = true;
  unsigned long ultimoPitidoMillis = 0;
  const long intervaloPitidos = 500;

  while (true) {
    unsigned long currentMillis = millis();

    if (currentMillis - ultimoPitidoMillis >= intervaloPitidos) {
      ultimoPitidoMillis = currentMillis;
      if (pitidoAgudo) {
        tone(speakerPin, 2000, 200);
        digitalWrite(ledPin, HIGH); // Encender el LED
      } else {
        tone(speakerPin, 500, 200);
        digitalWrite(ledPin, LOW); // Apagar el LED
      }
      pitidoAgudo = !pitidoAgudo;
    }

    char key = keypad.getKey();
    if (key) {
      noTone(speakerPin);
      digitalWrite(ledPin, LOW); // Asegurarse de apagar el LED
      return;
    }
  }
}