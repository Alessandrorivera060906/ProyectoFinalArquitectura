#include <SPI.h>
#include <EEPROM.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <Servo.h>

const uint8_t PIN_RFID_SS   = 10;
const uint8_t PIN_595_LATCH = 6;
const uint8_t PIN_BUZZER    = 7;
const uint8_t PIN_SERVO     = 8;

const char TECLA_BORRAR     = '*';
const char TECLA_CONFIRMAR  = '#';
const char TECLA_CERRAR     = 'C';
const char TECLA_DESACTIVAR = 'D';

const uint8_t ANGULO_CERRADO = 180;
const uint8_t ANGULO_ABIERTO = 80;

enum Estado : uint8_t {
  ESTADO_INICIO         = 0b00000001,
  ESTADO_ESPERANDO_RFID = 0b00000010,
  ESTADO_VALIDANDO_RFID = 0b00000100,
  ESTADO_ESPERANDO_PIN  = 0b00001000,
  ESTADO_VALIDANDO_PIN  = 0b00010000,
  ESTADO_AUTORIZADO     = 0b00100000,
  ESTADO_DENEGADO       = 0b01000000,
  ESTADO_ALARMA         = 0b10000000
};

const unsigned long TIEMPO_INICIO     = 2000;
const unsigned long TIEMPO_VALIDACION = 1000;
const unsigned long TIEMPO_RESULTADO  = 3000;
const unsigned long TIEMPO_MENSAJE    = 2000;

const uint8_t MAX_INTENTOS = 3;
const uint8_t LARGO_PIN    = 4;

const uint8_t FIRMA_EEPROM = 0x5B;
const int     DIR_FIRMA    = 0;
const int     DIR_CANTIDAD = 1;
const int     DIR_USUARIOS = 2;
const uint8_t LARGO_UID    = 4;

struct Usuario {
  uint8_t  uid[LARGO_UID];
  uint16_t pin;
};

const Usuario USUARIOS_INICIALES[] = {
  { {0x35, 0x5A, 0x08, 0x01}, 1234 },
  { {0x61, 0x33, 0xB6, 0x06}, 5678 }
};
const uint8_t CANTIDAD_INICIAL = sizeof(USUARIOS_INICIALES) / sizeof(Usuario);

const byte FILAS = 4;
const byte COLUMNAS = 4;

char teclas[FILAS][COLUMNAS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

byte pinesFilas[FILAS] = {2, 3, 4, 5};
byte pinesColumnas[COLUMNAS] = {A0, A1, A2, A3};

MFRC522 rfid(PIN_RFID_SS, UINT8_MAX);
LiquidCrystal_I2C lcd(0x27, 16, 2);
Keypad teclado = Keypad(makeKeymap(teclas), pinesFilas, pinesColumnas, FILAS, COLUMNAS);
Servo servoPuerta;

Estado        estadoActual = ESTADO_INICIO;
unsigned long inicioEstado = 0;
uint8_t       uidLeido[LARGO_UID];
int           usuarioActual = -1;

uint8_t  digitosIngresados = 0;
uint16_t pinIngresado = 0;
uint8_t  intentosFallidos = 0;
bool     mostrandoPinIncorrecto = false;

void setup()
{
  Serial.begin(9600);
  SPI.begin();

  lcd.init();
  lcd.backlight();

  pinMode(PIN_595_LATCH, OUTPUT);
  digitalWrite(PIN_595_LATCH, LOW);

  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);

  servoPuerta.attach(PIN_SERVO);
  servoPuerta.write(ANGULO_CERRADO);

  cambiarEstado(ESTADO_INICIO);
  mostrarLCD(F("SISTEMA ACCESO"), F("INICIANDO..."));
  Serial.println(F("SISTEMA ACCESO"));
  Serial.println(F("INICIANDO..."));

  rfid.PCD_Init();
  delay(100);

  byte version = rfid.PCD_ReadRegister(MFRC522::VersionReg);
  Serial.print(F("Version del lector: 0x"));
  Serial.println(version, HEX);
  if (version == 0x00 || version == 0xFF)
  {
    Serial.println(F("ERROR: no se detecta el lector RFID"));
  }

  inicializarEEPROM();
  mostrarUsuarios();
}

void loop()
{
  unsigned long transcurrido = millis() - inicioEstado;
  char tecla = teclado.getKey();

  switch (estadoActual)
  {
    case ESTADO_INICIO:
      if (transcurrido >= TIEMPO_INICIO)
      {
        irAEsperaRFID();
      }
      break;

    case ESTADO_ESPERANDO_RFID:
      if (leerTarjeta())
      {
        cambiarEstado(ESTADO_VALIDANDO_RFID);
        Serial.print(F("Tarjeta detectada. UID: "));
        imprimirUID(uidLeido);
        Serial.println();
      }
      break;

    case ESTADO_VALIDANDO_RFID:
      if (transcurrido >= TIEMPO_VALIDACION)
      {
        validarTarjeta();
      }
      break;

    case ESTADO_ESPERANDO_PIN:
      if (mostrandoPinIncorrecto)
      {
        if (transcurrido >= TIEMPO_MENSAJE)
        {
          mostrandoPinIncorrecto = false;
          pedirPIN();
        }
      }
      else if (tecla)
      {
        procesarTeclaPIN(tecla);
      }
      break;

    case ESTADO_VALIDANDO_PIN:
      if (transcurrido >= TIEMPO_VALIDACION)
      {
        validarPIN();
      }
      break;

    case ESTADO_AUTORIZADO:
      if (tecla == TECLA_CERRAR)
      {
        Serial.println(F("Puerta cerrada"));
        irAEsperaRFID();
      }
      break;

    case ESTADO_DENEGADO:
      if (transcurrido >= TIEMPO_RESULTADO)
      {
        if (intentosFallidos >= MAX_INTENTOS)
        {
          activarAlarma();
        }
        else
        {
          irAEsperaRFID();
        }
      }
      break;

    case ESTADO_ALARMA:
      if (tecla == TECLA_DESACTIVAR)
      {
        digitalWrite(PIN_BUZZER, LOW);
        Serial.println(F("Alarma desactivada"));
        irAEsperaRFID();
      }
      break;

    default:
      break;
  }
}

void mostrarLCD(const __FlashStringHelper *linea1, const __FlashStringHelper *linea2)
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(linea1);
  lcd.setCursor(0, 1);
  lcd.print(linea2);
}

void cambiarEstado(Estado nuevo)
{
  estadoActual = nuevo;
  inicioEstado = millis();
  escribir595(nuevo);
}

void cerrarPuerta()
{
  servoPuerta.write(ANGULO_CERRADO);
}

void abrirPuerta()
{
  servoPuerta.write(ANGULO_ABIERTO);
}

void irAEsperaRFID()
{
  cerrarPuerta();
  usuarioActual = -1;
  intentosFallidos = 0;
  cambiarEstado(ESTADO_ESPERANDO_RFID);
  mostrarLCD(F("CONTROL ACCESO"), F("PRESENTE TARJETA"));
  Serial.println();
  Serial.println(F("CONTROL ACCESO"));
  Serial.println(F("PRESENTE TARJETA"));
}

void validarTarjeta()
{
  usuarioActual = buscarUsuario(uidLeido);

  if (usuarioActual >= 0)
  {
    Serial.print(F("Tarjeta valida. Usuario "));
    Serial.println(usuarioActual + 1);
    intentosFallidos = 0;
    pedirPIN();
  }
  else
  {
    cerrarPuerta();
    cambiarEstado(ESTADO_DENEGADO);
    mostrarLCD(F("TARJETA INVALIDA"), F("ACCESO DENEGADO"));
    Serial.println(F("TARJETA INVALIDA"));
    Serial.println(F("ACCESO DENEGADO"));
  }
}

void pedirPIN()
{
  digitosIngresados = 0;
  pinIngresado = 0;
  cambiarEstado(ESTADO_ESPERANDO_PIN);
  mostrarLCD(F("INGRESE SU PIN"), F("____"));
  Serial.println(F("INGRESE SU PIN"));
}

void mostrarAsteriscos()
{
  lcd.setCursor(0, 1);
  for (uint8_t i = 0; i < LARGO_PIN; i++)
  {
    lcd.print(i < digitosIngresados ? '*' : '_');
  }
}

void procesarTeclaPIN(char tecla)
{
  if (tecla >= '0' && tecla <= '9')
  {
    if (digitosIngresados < LARGO_PIN)
    {
      pinIngresado = pinIngresado * 10 + (tecla - '0');
      digitosIngresados++;
      mostrarAsteriscos();
    }
  }
  else if (tecla == TECLA_BORRAR)
  {
    if (digitosIngresados > 0)
    {
      pinIngresado = pinIngresado / 10;
      digitosIngresados--;
      mostrarAsteriscos();
    }
  }
  else if (tecla == TECLA_CONFIRMAR)
  {
    if (digitosIngresados == LARGO_PIN)
    {
      cambiarEstado(ESTADO_VALIDANDO_PIN);
      Serial.println(F("Validando PIN..."));
    }
  }
}

void validarPIN()
{
  Usuario usuario = leerUsuario(usuarioActual);

  if (pinIngresado == usuario.pin)
  {
    cambiarEstado(ESTADO_AUTORIZADO);
    mostrarLCD(F("ACCESO"), F("AUTORIZADO"));
    abrirPuerta();
    Serial.println(F("ACCESO AUTORIZADO"));
    Serial.println(F("Puerta abierta, presione C para cerrar"));
    return;
  }

  cerrarPuerta();
  intentosFallidos++;
  Serial.print(F("PIN INCORRECTO. Intento "));
  Serial.println(intentosFallidos);

  if (intentosFallidos < MAX_INTENTOS)
  {
    cambiarEstado(ESTADO_ESPERANDO_PIN);
    mostrandoPinIncorrecto = true;
    mostrarLCD(F("PIN INCORRECTO"), F("INTENTE DE NUEVO"));
  }
  else
  {
    cambiarEstado(ESTADO_DENEGADO);
    mostrarLCD(F("ACCESO DENEGADO"), F(""));
    Serial.println(F("ACCESO DENEGADO"));
  }
}

void activarAlarma()
{
  cerrarPuerta();
  cambiarEstado(ESTADO_ALARMA);
  digitalWrite(PIN_BUZZER, HIGH);
  mostrarLCD(F("ACCESO BLOQUEADO"), F("ALARMA ACTIVADA"));
  Serial.println(F("ACCESO BLOQUEADO"));
  Serial.println(F("ALARMA ACTIVADA, presione D para desactivar"));
}

void escribir595(uint8_t valor)
{
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
  SPI.transfer(valor);
  SPI.endTransaction();

  digitalWrite(PIN_595_LATCH, HIGH);
  digitalWrite(PIN_595_LATCH, LOW);
}

bool leerTarjeta()
{
  if (!rfid.PICC_IsNewCardPresent())
  {
    return false;
  }
  if (!rfid.PICC_ReadCardSerial())
  {
    return false;
  }

  memset(uidLeido, 0, LARGO_UID);
  memcpy(uidLeido, rfid.uid.uidByte, min(rfid.uid.size, LARGO_UID));

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
  return true;
}

void imprimirUID(const uint8_t *uid)
{
  for (uint8_t i = 0; i < LARGO_UID; i++)
  {
    if (uid[i] < 0x10)
    {
      Serial.print('0');
    }
    Serial.print(uid[i], HEX);
    Serial.print(' ');
  }
}

void inicializarEEPROM()
{
  if (EEPROM.read(DIR_FIRMA) == FIRMA_EEPROM)
  {
    Serial.println(F("Usuarios cargados desde la EEPROM."));
    return;
  }

  for (uint8_t i = 0; i < CANTIDAD_INICIAL; i++)
  {
    EEPROM.put(direccionUsuario(i), USUARIOS_INICIALES[i]);
  }
  EEPROM.update(DIR_CANTIDAD, CANTIDAD_INICIAL);
  EEPROM.update(DIR_FIRMA, FIRMA_EEPROM);
  Serial.println(F("Usuarios guardados en la EEPROM por primera vez."));
}

uint8_t cantidadUsuarios()
{
  return EEPROM.read(DIR_CANTIDAD);
}

int direccionUsuario(uint8_t indice)
{
  return DIR_USUARIOS + indice * sizeof(Usuario);
}

Usuario leerUsuario(uint8_t indice)
{
  Usuario usuario;
  EEPROM.get(direccionUsuario(indice), usuario);
  return usuario;
}

int buscarUsuario(const uint8_t *uid)
{
  for (uint8_t i = 0; i < cantidadUsuarios(); i++)
  {
    Usuario usuario = leerUsuario(i);
    if (memcmp(usuario.uid, uid, LARGO_UID) == 0)
    {
      return i;
    }
  }
  return -1;
}

void mostrarUsuarios()
{
  Serial.print(F("Usuarios registrados: "));
  Serial.println(cantidadUsuarios());

  for (uint8_t i = 0; i < cantidadUsuarios(); i++)
  {
    Usuario usuario = leerUsuario(i);
    Serial.print(F("  Usuario "));
    Serial.print(i + 1);
    Serial.print(F(" - UID: "));
    imprimirUID(usuario.uid);
    Serial.print(F("- PIN: "));
    Serial.println(usuario.pin);
  }
}
