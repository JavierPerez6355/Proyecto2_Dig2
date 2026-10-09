#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal.h>
#include <ESP32SPISlave.h>

//******************************************/
// Definiciones
//******************************************/
#define LED1 26
#define LED2 2
#define LED3 15

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


//******************************************/
//configuración SPI
//******************************************/
#define SPI_HEADER  0xA5   // Encabezado para validar la trama
#define BUFFER_SIZE 4      // Trama: [0xA5][LED][TIEMPO_H][TIEMPO_L]
#define QUEUE_SIZE  1      // Tamaño de la cola de mensajes

//******************************************/
//Objetos
//******************************************/
ESP32SPISlave slave;  // Crear una instancia de la clase ESP32SPISlave
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

//******************************************/
// Variables SPI
//******************************************/
uint8_t tx_buf[BUFFER_SIZE] __attribute__((aligned(4))) = {0};  // Datos que se envian al maestro
uint8_t rx_buf[BUFFER_SIZE] __attribute__((aligned(4))) = {0};  // Datos que se reciben del maestro

const uint8_t ledPins[3] = {LED1, LED2, LED3};
const char inicialLed[3] = {'R', 'V', 'A'};

int ledActivo = -1;               // -1 = ningun LED encendido
char ultimoLed = '-';

unsigned long tInicio = 0;        // Momento en que se encendio el LED
unsigned long tDuracion = 0;      // Tiempo que debe permanecer encendido

//******************************************/
// Variables ADC e I2C
//******************************************/

uint8_t respuesta[4] = {0, 0, 0, 0};

uint16_t valorPot = 0;
uint8_t valorADC8 = 0;
uint32_t milivoltios = 0;

unsigned long tiempoAnterior = 0;

//******************************************/
// Prototipos
//******************************************/

void enviarDato();
void procesarTrama();
void apagarLeds();
void actualizarRespuestaI2C();

//******************************************/
// Configuración
//******************************************/

void setup() {

Serial.begin(115200);

//configuracion del LCD
lcd.begin(16, 2);
lcd.clear();
lcd.setCursor(0, 0);
lcd.print("Iniciando...");
lcd.setCursor(0, 1);
lcd.print("ESP32");

//configuracion del ADC
analogReadResolution(12);
analogSetPinAttenuation(pin_Pot, ADC_11db);

//Primera lectura del potenciómetro

actualizarRespuestaI2C();

// Configurar los 3 LEDs como salida e iniciarlos apagadaos

for (int i = 0; i < 3; i++) {
    pinMode(ledPins[i], OUTPUT);
    digitalWrite(ledPins[i], LOW);
}

//configurar SPI esclavo
slave.setDataMode(SPI_MODE0);  // Configurar el modo SPI (puede ser SPI_MODE0, SPI_MODE1, SPI_MODE2 o SPI_MODE3)
slave.setQueueSize(QUEUE_SIZE);  // Configurar el tamaño de la cola de mensajes

if (slave.begin(VSPI)) {
    Serial.println("ESP32 iniciado como esclavo SPI");
    Serial.print("MOSI pin: ");
    Serial.println(MOSI);
    Serial.print("MISO pin: ");
    Serial.println(MISO);
    Serial.print("SCK pin: ");
    Serial.println(SCK);
    Serial.print("SS Pin: ");
    Serial.println(SS);
} else {
    Serial.println("Error al iniciar SPI");
}

// configuracion del esclavo I2C
Wire.onRequest(enviarDato);

bool iniciado = Wire.begin((uint8_t)DIRECCION_I2C, pin_SDA, pin_SCL, 100000);

if (iniciado)
{
    Wire.slaveWrite(respuesta, sizeof(respuesta));
    Serial.println("ESP32 iniciado como esclavo I2C");
}
else
{
    Serial.println("Error al iniciar I2C");
}
}

//******************************************/
//Ciclo principal
//******************************************/

void loop()
 {

  
  // Preparar la nueva recepcion SPI si el esclavo esta libre
  if (slave.hasTransactionsCompletedAndAllResultsHandled()) 
  {
    //limpia el buffer de recepcion
    memset(rx_buf, 0, sizeof(rx_buf));
    //prepara una transaccion SPI de 4 bytes.
    slave.queue(tx_buf, rx_buf, BUFFER_SIZE);
    //habilita la transaccion. 
    slave.trigger();
  }

  //Procesar la comunicacion SPI si la transaccion ya termino y el dato ya fue recibido

  if (slave.hasTransactionsCompletedAndAllResultsReady(QUEUE_SIZE)) {

    //interpreta el comando recibido

    auto bytesRecibidos = slave.numBytesReceivedAll();

    if (!bytesRecibidos.empty() && bytesRecibidos[0] == BUFFER_SIZE) {
        procesarTrama();
    } 
    else 
    {
        Serial.println("Trama SPI incompleta");
    }
  
  }

 
  //Apagar el LED cuando se cumpla el tiempo 

  //se usa millis() para no bloquear la comunicacion SPI
  if (ledActivo >= 0 && (millis() - tInicio >= tDuracion))
   {
    digitalWrite(ledPins[ledActivo], LOW);

    Serial.print("LED ");
    Serial.print(ledActivo + 1);
    Serial.println(" apagado");

    ledActivo = -1;
  }
//Actualizar ADC, I2C y LCD cada 100 ms

    unsigned long tiempoActual = millis();

    if (tiempoActual - tiempoAnterior >= 100)
    {
        tiempoAnterior = tiempoActual;

        actualizarRespuestaI2C();
        Wire.slaveWrite(respuesta, sizeof(respuesta));

        Serial.printf("ADC: %u\n", (unsigned int)valorADC8);
        Serial.printf("mV: %u\n", (unsigned int)milivoltios);

        char linea1[17];
        char linea2[17];

        snprintf(linea1, sizeof(linea1), "P1:%lu.%03luV", (unsigned long)(milivoltios / 1000), (unsigned long)(milivoltios % 1000));
        snprintf(linea2, sizeof(linea2), "ADC:%4u LED:%c", (unsigned int)valorADC8, ultimoLed); 

        lcd.setCursor(0, 0);
        lcd.print(linea1);
        lcd.print("      ");

        lcd.setCursor(0, 1);
        lcd.print(linea2);
        lcd.print("  ");
    }
}

//******************************************/
// Actualizar medición y respuesta I2C
//******************************************/

void actualizarRespuestaI2C()
{
        valorPot = analogRead(pin_Pot);
        valorADC8 = map(valorPot, 0, 4095, 0, 255);
        milivoltios = analogReadMilliVolts(pin_Pot);

        respuesta[0] = 0;
        respuesta[1] = valorADC8;
        respuesta[2] = (uint8_t)(milivoltios >> 8);
        respuesta[3] = (uint8_t)(milivoltios & 0xFF);
}



//******************************************/
// Otras funciones
//******************************************/

// Responde a la solicitud de datos del maestro I2C

void enviarDato()
{
    Wire.write(respuesta, sizeof(respuesta));
}

// Revisa la trama recibida y enciende el LED indicado
void procesarTrama() {
  //verificar el encabezado
  if (rx_buf[0] != SPI_HEADER) {
    Serial.println("Trama invalida: encabezado incorrecto");
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
  if (led < 1 || led > 3 || tiempo == 0) 
  {
    Serial.println("Trama invalida: datos incorrectos");
    return;
  }

  //encender el LED indicado
  apagarLeds();

  ledActivo = led - 1;
  ultimoLed = inicialLed[ledActivo];
  tDuracion = tiempo;
  tInicio = millis();

  digitalWrite(ledPins[ledActivo], HIGH);

  Serial.print("Encendiendo LED ");
  Serial.println(led);
  Serial.print("durante");
  Serial.print(tiempo);
  Serial.println(" ms");
}
//*************************************/
// Apagar todos los LEDs
//*************************************/
// Apaga los 3 LEDs
void apagarLeds(void) {
  for (int i = 0; i < 3; i++) {
    digitalWrite(ledPins[i], LOW);
  }
  ledActivo = -1;
}