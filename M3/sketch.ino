#include <Arduino.h>
#include <driver/i2s.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/version.h"

#include "speech_model_data.h"  

#define I2S_WS   15  
#define I2S_SCK  14  
#define I2S_SD   32  
#define SAMPLE_RATE 16000
#define BUFFER_SIZE 1024

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SDA_PIN 8
#define SCL_PIN 7

uint8_t SCREEN_ADDRESS = 0x3C;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

int16_t audio_buffer[BUFFER_SIZE];

constexpr int kTensorArenaSize = 10 * 1024;
uint8_t tensor_arena[kTensorArenaSize];

static tflite::MicroErrorReporter micro_error_reporter;
static tflite::AllOpsResolver resolver;
const tflite::Model* model = nullptr;
tflite::MicroInterpreter* interpreter = nullptr;
TfLiteTensor* input_tensor = nullptr;
TfLiteTensor* output_tensor = nullptr;

void setupI2S() {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S,
    .intr_alloc_flags = 0,
    .dma_buf_count = 8,
    .dma_buf_len = BUFFER_SIZE,
    .use_apll = false
  };
  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_SD
  };
  i2s_set_pin(I2S_NUM_0, &pin_config);
}

void setupTensorFlow() {
  model = tflite::GetModel(speech_model_data);
  if (model->version() != TFLITE_SCHEMA_VERSION) {
    Serial.println("Model schema version mismatch!");
    return;
  }
  interpreter = new tflite::MicroInterpreter(model, resolver, tensor_arena, kTensorArenaSize, &micro_error_reporter);
  interpreter->AllocateTensors();
  input_tensor = interpreter->input(0);
  output_tensor = interpreter->output(0);
}

void scanI2CDevices() {
  Serial.println("Scanning I2C devices...");
  for (byte address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    if (Wire.endTransmission() == 0) {
      Serial.print("Found I2C device at 0x");
      Serial.println(address, HEX);
      SCREEN_ADDRESS = address;
    }
  }
}

void setupDisplay() {
  Wire.begin(SDA_PIN, SCL_PIN);
  scanI2CDevices();
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    while (1);
  }
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
}

void displayWord(const char* word) {
  display.clearDisplay();
  display.setCursor(10, 20);
  display.println(word);
  display.display();
}

void setup() {
  Serial.begin(115200);
  setupI2S();
  setupTensorFlow();
  setupDisplay();
}

void loop() {
  size_t bytes_read;
  i2s_read(I2S_NUM_0, audio_buffer, sizeof(audio_buffer), &bytes_read, portMAX_DELAY);

  for (int i = 0; i < BUFFER_SIZE; i++) {
    input_tensor->data.int8[i] = audio_buffer[i] >> 8;
  }

  interpreter->Invoke();

  int predicted_class = -1;
  int max_score = -128;
  for (int i = 0; i < output_tensor->dims->data[1]; i++) {
    int score = output_tensor->data.int8[i];
    if (score > max_score) {
      max_score = score;
      predicted_class = i;
    }
  }

  const char* detected_word = "Unknown";
  switch (predicted_class) {
    case 0: detected_word = "Yes"; break;
    case 1: detected_word = "No"; break;
    case 2: detected_word = "Stop"; break;
    default: detected_word = "Unknown"; break;
  }

  Serial.print("Detected word: ");
  Serial.println(detected_word);

  displayWord(detected_word);

  delay(1000);
}
