#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal.h>
#include <ESP32SPISlave.h>

#define DIRECCION_I2C 0x28
#define pin_SDA 21
#define pin_SCL 22
#define pin_Pot 4

#define rs 32
#define en 33
#define d4 27
#define d5 14
#define d6 25
#define d7 13

#define LED1 
#define LED2 
#define LED3 

#define SPI_HEADER  0xA5   // Encabezado para validar la trama
#define BUFFER_SIZE 4      // Trama: [0xA5][LED][TIEMPO_H][TIEMPO_L]
#define QUEUE_SIZE  1      // Tamaño de la cola de mensajes

LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

int contador = 0;

uint8_t respuesta[4] = {0, 0, 0, 0};

uint8_t tx_buf[BUFFER_SIZE] __attribute__((aligned(4))) = {0};  // Datos que se envian al maestro
uint8_t rx_buf[BUFFER_SIZE] __attribute__((aligned(4))) = {0};  // Datos que se reciben del maestro

const uint8_t ledPins[3] = {LED1, LED2, LED3};

int ledActivo = -1;               // -1 = ningun LED encendido
unsigned long tInicio = 0;        // Momento en que se encendio el LED
unsigned long tDuracion = 0;      // Tiempo que debe permanecer encendido

ESP32SPISlave slave;  // Crear una instancia de la clase ESP32SPISlave

uint16_t valorPot = 0;
uint32_t milivoltios = 0;
unsigned long tiempoAnterior = 0;


void enviarDato();
void procesarTrama(void);
void apagarLeds(void);

void setup() {


lcd.begin(16, 2);
lcd.clear();
lcd.setCursor(0, 0);
lcd.print("Prueba LCD");
lcd.setCursor(0, 1);
lcd.print("ESP32 iniciado");

Serial.begin(115200);
analogReadResolution(12);
analogSetPinAttenuation(pin_Pot, ADC_11db);

// Configurar los 3 LEDs como salida e iniciarlos apagados
for (int i = 0; i < 3; i++) {
    pinMode(ledPins[i], OUTPUT);
    digitalWrite(ledPins[i], LOW);
}

//configurar em modo de comunicacion SPI
slave.setDataMode(SPI_MODE0);  // Configurar el modo SPI (puede ser SPI_MODE0, SPI_MODE1, SPI_MODE2 o SPI_MODE3)
slave.setQueueSize(QUEUE_SIZE);  // Configurar el tamaño de la cola de mensajes
//HSPI: CS: 15, SCK: 14, MOSI: 13, MISO: 12
//VSPI CS: 5, SCK: 18, MOSI: 23, MISO: 19
slave.begin(VSPI);
//imprimirlos en los pines empleados 
Serial.print("MOSI pin: ");
Serial.println(MOSI);
Serial.print("MISO pin: ");
Serial.println(MISO);
Serial.print("SCK pin: ");
Serial.println(SCK);
Serial.print("SS Pin: ");
Serial.println(SS);

Serial.println("Esclavo SPI listo para recibir comandos...");

Wire.onRequest(enviarDato);

bool iniciado = Wire.begin((uint8_t)DIRECCION_I2C, pin_SDA, pin_SCL, 100000);

if (iniciado)
{
    Wire.slaveWrite(respuesta, 4);
    Serial.println("ESP32 iniciado como esclavo I2C");
}
else
{
    Serial.println("Error al iniciar I2C");
}
}

void loop() {

  //**********************************************/
  //1. preparar la comunicacion 
  //*******************************************/
  //el esclavo esta libre para recibir el dato?
  if (slave.hasTransactionsCompletedAndAllResultsHandled()) {
    //limpia el buffer de recepcion
    memset(rx_buf, 0, BUFFER_SIZE);
    //prepara una transaccion SPI de 4 bytes.
    slave.queue(tx_buf, rx_buf, BUFFER_SIZE);
    //habilita la transaccion. 
    slave.trigger();
  }

  //**********************************************/
  //2. Procesar la comunicacion terminada. 
  //*******************************************/
  //la transaccion spi ya termino y el dato ya fue recibido?
  if (slave.hasTransactionsCompletedAndAllResultsReady(QUEUE_SIZE)) {
    //obtiene el resultado de la transaccion.
    slave.numBytesReceivedAll();
    //interpreta el comando recibido
    procesarTrama();
  }

  //**********************************************/
  //3. Apagar el LED cuando se cumpla el tiempo
  //*******************************************/
  //se usa millis() para no bloquear la comunicacion SPI
  if (ledActivo >= 0 && (millis() - tInicio >= tDuracion)) {
    digitalWrite(ledPins[ledActivo], LOW);
    Serial.print("LED ");
    Serial.print(ledActivo + 1);
    Serial.println(" apagado");
    ledActivo = -1;
  }

    unsigned long tiempoActual = millis();

    if (tiempoActual - tiempoAnterior >= 100)
    {
        tiempoAnterior = tiempoActual;

        valorPot = analogRead(pin_Pot);
        milivoltios = analogReadMilliVolts(pin_Pot);

        respuesta[0] = (uint8_t)(valorPot >> 8);
        respuesta[1] = (uint8_t)(valorPot & 0xFF);
        respuesta[2] = (uint8_t)(milivoltios >> 8);
        respuesta[3] = (uint8_t)(milivoltios & 0xFF);

        Wire.slaveWrite(respuesta, sizeof(respuesta));

        Serial.printf("ADC: %u\n", (unsigned int)valorPot);
        Serial.printf("mV: %u\n", (unsigned int)milivoltios);

        char linea1[17];
        char linea2[17];

        snprintf(linea1, sizeof(linea1), "P1:%lu.%03luV", (unsigned long)(milivoltios / 1000), (unsigned long)(milivoltios % 1000));
        snprintf(linea2, sizeof(linea2), "ADC:%4u LED:-", (unsigned int)valorPot);

        lcd.setCursor(0, 0);
        lcd.print(linea1);
        lcd.print("      ");

        lcd.setCursor(0, 1);
        lcd.print(linea2);
        lcd.print("  ");
    }
}

//******************************************/
// Otras funciones
//******************************************/

void enviarDato()
{
    Wire.write(respuesta, 4);
}

// Revisa la trama recibida y enciende el LED indicado
void procesarTrama(void) {
  //verificar el encabezado
  if (rx_buf[0] != SPI_HEADER) {
    Serial.println("Trama invalida (encabezado)");
    return;
  }

    uint8_t led = rx_buf[1];
  uint16_t tiempo = ((uint16_t)rx_buf[2] << 8) | rx_buf[3];

  //Muestra el dato recibido.
  Serial.print("Dato recibido -> LED: ");
  Serial.print(led);
  Serial.print("  Tiempo: ");
  Serial.print(tiempo);
  Serial.println(" ms");

  //verificar que los datos sean validos
  if (led < 1 || led > 3 || tiempo == 0) {
    Serial.println("Trama invalida (datos)");
    return;
  }

  //encender el LED indicado
  apagarLeds();
  ledActivo = led - 1;
  tDuracion = tiempo;
  tInicio = millis();
  digitalWrite(ledPins[ledActivo], HIGH);

  Serial.print("Encendiendo LED ");
  Serial.println(led);
}

// Apaga los 3 LEDs
void apagarLeds(void) {
  for (int i = 0; i < 3; i++) {
    digitalWrite(ledPins[i], LOW);
  }
  ledActivo = -1;
}