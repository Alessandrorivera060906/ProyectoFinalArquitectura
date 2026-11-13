# Sistema computacional de Control de Acceso y Seguridad

Proyecto de aplicación 2S2026

Arquitectura del Computador, Facultad de Ingeniería, Ingeniería en Informática y Sistemas

## Integrantes

| Nombre | Carné |
|---|---|
| María Inés Leiva Casiano | 1089524 |
| Javier Alessandro Rivera Lemus | 1241224 |
| Jennifer Fernanda Turcios Estrada | 1088724 |

Universidad Rafael Landívar, Sección 02

Catedrático: Ing. Andrés Sebastián Gálvez Arriaza

---

# Manual de usuario

## 1. Descripción del sistema

El sistema es una terminal de control de acceso en miniatura basada en un Arduino UNO. El usuario se identifica con una tarjeta RFID y luego ingresa un PIN de 4 dígitos en un teclado matricial. Si ambas credenciales son correctas, un servomotor abre la puerta. Si el PIN se ingresa mal tres veces, se activa una alarma sonora.

Durante todo el funcionamiento:

- La pantalla LCD muestra los mensajes al usuario.
- Un registro de desplazamiento 74HC595 enciende uno de 8 LEDs para indicar el estado interno del programa.
- Los usuarios autorizados se guardan en la memoria EEPROM, por lo que se conservan aunque el Arduino se quede sin alimentación.

## 2. Contenido del repositorio

| Carpeta o archivo | Contenido |
|---|---|
| `README.md` | Este manual de usuario |
| `CodigoFase2/` | Código fuente del sistema (`CodigoFase2.ino`) |
| `Modelos 3D/` | Modelos `.stl` de las piezas impresas en 3D |
| `Proyecto Final Arquitectura del Computador.pdf` | Documento del proyecto final, con el diagrama de bloques de arquitectura |

## 3. Componentes

| Componente | Cantidad | Función |
|---|---|---|
| Arduino UNO | 1 | Unidad central de procesamiento |
| Módulo RFID RC522 | 1 | Lectura del UID de las tarjetas |
| Tarjetas o llaveros RFID | 3 | 2 registradas y 1 no registrada para pruebas |
| Teclado matricial 4x4 | 1 | Ingreso del PIN y comandos |
| Pantalla LCD 16x2 con módulo I2C | 1 | Mensajes al usuario |
| Registro de desplazamiento 74HC595 | 1 | Control de los 8 LEDs de estado |
| LEDs con resistencia de 220 Ω | 8 | Indicadores visuales de estado |
| Servomotor SG90 | 1 | Apertura y cierre de la puerta |
| Buzzer activo | 1 | Alarma sonora |
| Resistencias de 1 kΩ | 9 | Divisores de voltaje para el RC522 |
| Power bank USB o batería de 9 V con jack | 1 | Alimentación independiente del Arduino |
| Protoboard y cables | | Conexiones |
| Estructura impresa en 3D | 1 | Terminal con puerta, ver la carpeta `Modelos 3D/` |

## 4. Asignación de pines

| Pin Arduino | Conectado a | Descripción |
|---|---|---|
| D2, D3, D4, D5 | Teclado, filas 1 a 4 | Barrido del teclado |
| A0, A1, A2, A3 | Teclado, columnas 1 a 4 | Pines analógicos usados como digitales |
| D6 | 74HC595 pin 12 (ST_CP) | Latch del registro |
| D7 | Buzzer activo (+) | Alarma |
| D8 | Servo SG90 (señal) | Control de la puerta |
| D10 | RC522 SDA (SS) | Selección del lector, con divisor de voltaje |
| D11 (MOSI) | RC522 MOSI y 74HC595 pin 14 (DS) | Bus SPI compartido, divisor de voltaje hacia el RC522 |
| D12 (MISO) | RC522 MISO | Conexión directa |
| D13 (SCK) | RC522 SCK y 74HC595 pin 11 (SH_CP) | Bus SPI compartido, divisor de voltaje hacia el RC522 |
| A4 (SDA) | LCD I2C SDA | Bus I2C, dirección 0x27 |
| A5 (SCL) | LCD I2C SCL | Bus I2C |
| 3.3 V | RC522 VCC y RC522 RST | El RST queda fijo en alto |
| 5 V | LCD, 74HC595, servo | Alimentación de la lógica |
| GND | Todos los módulos | Referencia común |

### Divisores de voltaje del RC522

El RC522 trabaja a 3.3 V y el Arduino entrega 5 V en sus salidas. Por eso las líneas SDA, SCK y MOSI pasan por un divisor de voltaje antes de llegar al lector:

- R1 = 1 kΩ entre el pin del Arduino y el pin del RC522.
- R2 = 2 kΩ (dos de 1 kΩ en serie) entre el pin del RC522 y GND.
- Vout = 5 V × 2 kΩ / 3 kΩ = 3.33 V

La línea MISO va directa porque es una salida del RC522 a 3.3 V, y el Arduino la reconoce como nivel alto.

## 5. Alimentación

- El Arduino se alimenta de forma independiente con un power bank conectado por el cable USB o con una batería de 9 V conectada al jack.
- El servomotor puede alimentarse desde el pin de 5 V del Arduino o desde una fuente externa de 4.8 V a 6 V, por ejemplo 4 pilas AA. Si se usa fuente externa, su GND debe unirse al GND del Arduino para tener la misma referencia eléctrica.
- No conecte la batería de 9 V directamente al servo, porque su voltaje máximo es de 6 V.

## 6. Instalación del software

1. Instale el Arduino IDE.
2. En el gestor de librerías (Herramientas > Administrar bibliotecas) instale:
   - MFRC522, de GithubCommunity
   - LiquidCrystal I2C, de Frank de Brabander
   - Keypad, de Mark Stanley y Alexander Brevig
3. Las librerías SPI, Wire, EEPROM y Servo ya vienen con el IDE.
4. Abra `CodigoFase2/CodigoFase2.ino`.
5. Seleccione la placa Arduino UNO y el puerto correspondiente.
6. Presione Subir.

Una vez subido, el código queda guardado en el Arduino. Puede desconectarlo de la computadora y alimentarlo con el power bank. Cada vez que reciba corriente, el programa iniciará desde el principio.

## 7. Usuarios registrados

| Usuario | UID de la tarjeta | PIN |
|---|---|---|
| 1 | 35 5A 08 01 | 1234 |
| 2 | 61 33 B6 06 | 5678 |

La tarjeta con UID 51 88 DA 06 no está registrada y se usa para probar el caso de tarjeta inválida.

Los usuarios se escriben en la EEPROM solo la primera vez que se ejecuta el programa. En las siguientes ejecuciones el sistema detecta la firma 0x5B en la dirección 0 y lee los usuarios ya guardados.

Organización de la EEPROM:

| Dirección | Contenido |
|---|---|
| 0 | Firma 0x5B |
| 1 | Cantidad de usuarios |
| 2 a 7 | Usuario 1: UID (4 bytes) y PIN (2 bytes) |
| 8 a 13 | Usuario 2: UID (4 bytes) y PIN (2 bytes) |

## 8. Teclas de control

| Tecla | Función |
|---|---|
| 0 a 9 | Ingresar los dígitos del PIN |
| * | Borrar el último dígito |
| # | Confirmar el PIN (solo cuando hay 4 dígitos) |
| C | Cerrar la puerta |
| D | Desactivar la alarma |

## 9. Uso del sistema

### Encendido

Al conectar la alimentación la pantalla muestra `SISTEMA ACCESO / INICIANDO...` durante 2 segundos y luego `CONTROL ACCESO / PRESENTE TARJETA`. La puerta inicia cerrada.

### Acceso autorizado

1. Acerque una tarjeta registrada al lector RFID.
2. La pantalla muestra `INGRESE SU PIN` y debajo `____`.
3. Ingrese los 4 dígitos de su PIN. Por cada dígito aparece un `*`. Si se equivoca, presione `*` para borrar.
4. Presione `#` para confirmar.
5. Si el PIN es correcto, la pantalla muestra `ACCESO / AUTORIZADO` y el servo abre la puerta.
6. La puerta permanece abierta hasta que presione `C`. Al presionarla, la puerta se cierra y el sistema vuelve a `PRESENTE TARJETA`.

### Tarjeta no registrada

Si acerca una tarjeta que no está registrada, la pantalla muestra `TARJETA INVALIDA / ACCESO DENEGADO` durante 3 segundos y el sistema regresa a `PRESENTE TARJETA`. La puerta no se mueve.

### PIN incorrecto

- Primer y segundo intento fallido: la pantalla muestra `PIN INCORRECTO / INTENTE DE NUEVO` durante 2 segundos y vuelve a pedir el PIN.
- Tercer intento fallido: la pantalla muestra `ACCESO DENEGADO` durante 3 segundos y se activa la alarma.

### Alarma

Con la alarma activada, el buzzer suena de forma continua y la pantalla muestra `ACCESO BLOQUEADO / ALARMA ACTIVADA`. La alarma se mantiene hasta que se presiona `D`. Después el sistema vuelve a `PRESENTE TARJETA` con los intentos en cero.

## 10. Indicadores de estado (74HC595)

Cada estado del programa se representa con un byte que se envía al 74HC595 por el bus SPI. Cada bit enciende un LED, por lo que siempre hay un solo LED encendido.

| LED | Byte | Estado |
|---|---|---|
| 1 | 0b00000001 | Inicio del sistema |
| 2 | 0b00000010 | Esperando tarjeta RFID |
| 3 | 0b00000100 | Validando tarjeta |
| 4 | 0b00001000 | Esperando PIN |
| 5 | 0b00010000 | Validando PIN |
| 6 | 0b00100000 | Acceso autorizado |
| 7 | 0b01000000 | Acceso denegado |
| 8 | 0b10000000 | Alarma activada |

## 11. Funcionamiento interno

- El programa es una máquina de estados dentro de `loop()`. Los tiempos se controlan con `millis()`, por lo que el Arduino no se bloquea y sigue leyendo el teclado mientras espera.
- El RC522 y el 74HC595 comparten el bus SPI (MOSI en D11 y SCK en D13). El lector se selecciona con D10 y el registro con su latch en D6.
- La comparación del UID leído contra los usuarios de la EEPROM y la del PIN ingresado contra el PIN del usuario las realiza la ALU del microcontrolador.
- El PIN se guarda como un número de 16 bits (`uint16_t`), y cada usuario ocupa 6 bytes en la EEPROM.

## 12. Solución de problemas

| Problema | Posible causa y solución |
|---|---|
| El lector no detecta tarjetas | Revise los divisores de voltaje, que VCC y RST estén en 3.3 V y que MISO esté en D12. En el Monitor Serie la versión del lector debe ser distinta de 0x00 y 0xFF. |
| La pantalla enciende pero no muestra texto | Ajuste el potenciómetro de contraste del módulo I2C y verifique SDA en A4 y SCL en A5. |
| Una fila o columna del teclado no responde | Revise que los cables del conector del teclado estén firmes. |
| El buzzer no suena | Verifique que el pin + del buzzer esté en D7 y el otro en GND. |
| El Arduino se reinicia al mover el servo | Alimente el servo con una fuente externa de 4.8 V a 6 V con GND común. |
| El power bank se apaga solo | Algunos power banks se apagan si el consumo es bajo. Use otro power bank o la batería de 9 V por el jack. |

## 13. Modelos 3D

Los modelos se basaron en los modelos base del curso (https://github.com/memepelle/arduino-security-project) y se modificaron para este prototipo.

| Archivo | Pieza |
|---|---|
| `Front nuevo.stl` | Frente de la terminal |
| `back.stl` | Parte trasera |
| `Puerta.stl` | Puerta |
| `Pestillo.stl` | Pestillo de la puerta |
| `servo_bracket.stl` | Soporte del servo |
| `U_rectangular_servo.stl` | Pieza en U para el servo |
| `Bandeja RFID.stl` | Bandeja del lector RFID |
| `Tapadera RFID.stl` | Tapadera del lector RFID |
| `Tapadera cables teclado.stl` | Tapadera para los cables del teclado |
| `4_soportes_esquina_M4.stl` | Soportes de esquina para tornillos M4 |
