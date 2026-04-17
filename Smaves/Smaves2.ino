#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_camera.h"
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <dwyn-project-1_inferencing.h>
#include "edge-impulse-sdk/dsp/image/image.hpp"

const char* ssid = "ROGPHONE";
const char* password = "87654321";

// Initialize Telegram BOT
const char* BOTtoken = "7040869334:AAH-eLcJBJAQKaHHZ-O6LSeLwyy3a_YJMf8";  // your Bot Token (Get from Botfather)

// Chat ID
const char* CHAT_ID = "1361824109"; // Make sure this is the correct chat ID

bool sendPhoto = false;

WiFiClientSecure clientTCP;
UniversalTelegramBot bot(BOTtoken, clientTCP);

#define FLASH_LED_PIN 4
bool flashState = LOW;

// Checks for new messages every 1 second
const int botRequestDelay = 1000;
unsigned long lastTimeBotRan = 0;

// Select camera model
#define CAMERA_MODEL_AI_THINKER // Has PSRAM

#if defined(CAMERA_MODEL_AI_THINKER)
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

#else
#error "Camera model not selected"
#endif

/* Constant defines -------------------------------------------------------- */
#define EI_CAMERA_RAW_FRAME_BUFFER_COLS           320
#define EI_CAMERA_RAW_FRAME_BUFFER_ROWS           240
#define EI_CAMERA_FRAME_BYTE_SIZE                 3

/* Private variables ------------------------------------------------------- */
static bool debug_nn = false; // Set this to true to see e.g. features generated from the raw signal
static bool is_initialised = false;
uint8_t *snapshot_buf; //points to the output of the capture

static camera_config_t camera_config = {
    .pin_pwdn = PWDN_GPIO_NUM,
    .pin_reset = RESET_GPIO_NUM,
    .pin_xclk = XCLK_GPIO_NUM,
    .pin_sscb_sda = SIOD_GPIO_NUM,
    .pin_sscb_scl = SIOC_GPIO_NUM,

    .pin_d7 = Y9_GPIO_NUM,
    .pin_d6 = Y8_GPIO_NUM,
    .pin_d5 = Y7_GPIO_NUM,
    .pin_d4 = Y6_GPIO_NUM,
    .pin_d3 = Y5_GPIO_NUM,
    .pin_d2 = Y4_GPIO_NUM,
    .pin_d1 = Y3_GPIO_NUM,
    .pin_d0 = Y2_GPIO_NUM,
    .pin_vsync = VSYNC_GPIO_NUM,
    .pin_href = HREF_GPIO_NUM,
    .pin_pclk = PCLK_GPIO_NUM,

    //XCLK 20MHz or 10MHz for OV2640 double FPS (Experimental)
    .xclk_freq_hz = 20000000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_JPEG, //YUV422,GRAYSCALE,RGB565,JPEG
    .frame_size = FRAMESIZE_QVGA,    //QQVGA-UXGA Do not use sizes above QVGA when not JPEG

    .jpeg_quality = 12, //0-63 lower number means higher quality
    .fb_count = 1,       //if more than one, i2s runs in continuous mode. Use only with JPEG
    .fb_location = CAMERA_FB_IN_PSRAM,
    .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
};

/* Function definitions ------------------------------------------------------- */
bool ei_camera_init(void);
void ei_camera_deinit(void);
bool ei_camera_capture(uint32_t img_width, uint32_t img_height, uint8_t *out_buf);
void handleNewMessages(int numNewMessages);
String sendPhotoTelegram();
void configInitCamera();
static int ei_camera_get_data(size_t offset, size_t length, float *out_ptr);
void resizeImage(const uint8_t* src, int srcWidth, int srcHeight, uint8_t* dst, int dstWidth, int dstHeight);

/**
* @brief      Arduino setup function
*/
void setup() {
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
    Serial.begin(115200);
    pinMode(FLASH_LED_PIN, OUTPUT);
    digitalWrite(FLASH_LED_PIN, flashState);

    if (!ei_camera_init()) {
        Serial.println("Failed to initialize Camera!");
    } else {
        Serial.println("Camera initialized");
    }

    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    clientTCP.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Add root certificate for api.telegram.org
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(500);
    }
    Serial.println();
    Serial.print("ESP32-CAM IP Address: ");
    Serial.println(WiFi.localIP());
}

/**
* @brief      Get data and run inferencing
*
* @param[in]  debug  Get debug info if true
*/
void loop() {
    if (sendPhoto) {
        Serial.println("Preparing photo");
        sendPhotoTelegram();
        sendPhoto = false;
    }
    if (millis() > lastTimeBotRan + botRequestDelay) {
        int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
        while (numNewMessages) {
            handleNewMessages(numNewMessages);
            numNewMessages = bot.getUpdates(bot.last_message_received + 1);
        }
        lastTimeBotRan = millis();
    }

    if (ei_sleep(5) != EI_IMPULSE_OK) {
        return;
    }

    snapshot_buf = (uint8_t*)malloc(EI_CAMERA_RAW_FRAME_BUFFER_COLS * EI_CAMERA_RAW_FRAME_BUFFER_ROWS * EI_CAMERA_FRAME_BYTE_SIZE);
    if (snapshot_buf == nullptr) {
        Serial.println("ERR: Failed to allocate snapshot buffer!");
        return;
    }

    ei::signal_t signal;
    signal.total_length = EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT;
    signal.get_data = &ei_camera_get_data;

    if (!ei_camera_capture((size_t)EI_CLASSIFIER_INPUT_WIDTH, (size_t)EI_CLASSIFIER_INPUT_HEIGHT, snapshot_buf)) {
        Serial.println("Failed to capture image");
        free(snapshot_buf);
        return;
    }

    ei_impulse_result_t result = { 0 };
    EI_IMPULSE_ERROR err = run_classifier(&signal, &result, debug_nn);
    if (err != EI_IMPULSE_OK) {
        Serial.printf("ERR: Failed to run classifier (%d)\n", err);
        free(snapshot_buf);
        return;
    }

    Serial.printf("Predictions (DSP: %d ms., Classification: %d ms., Anomaly: %d ms.): \n",
                  result.timing.dsp, result.timing.classification, result.timing.anomaly);

#if EI_CLASSIFIER_OBJECT_DETECTION == 1
    bool bb_found = result.bounding_boxes[0].value > 0;
    for (size_t ix = 0; ix < result.bounding_boxes_count; ix++) {
        auto bb = result.bounding_boxes[ix];
        if (bb.value == 0) {
            continue;
        }
        Serial.printf("    %s (%f) [ x: %u, y: %u, width: %u, height: %u ]\n", bb.label, bb.value, bb.x, bb.y, bb.width, bb.height);
        if (String(bb.label) == "person" && bb.value > 0.5) { // If a person is detected with confidence > 0.8
            sendPhoto = true;
        }
    }
    if (!bb_found) {
        Serial.println("    No objects found");
    }
#else
    for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
        Serial.printf("    %s: %.5f\n", result.classification[ix].label, result.classification[ix].value);
        if (String(result.classification[ix].label) == "person" && result.classification[ix].value > 0.5) { // If a person is detected with confidence > 0.8
            sendPhoto = true;
        }
    }
#endif

#if EI_CLASSIFIER_HAS_ANOMALY == 1
    Serial.printf("    anomaly score: %.3f\n", result.anomaly);
#endif

    free(snapshot_buf);
}

bool ei_camera_init(void) {
    if (is_initialised) return true;

    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK) {
        Serial.printf("Camera init failed with error 0x%x", err);
        return false;
    }

    sensor_t *s = esp_camera_sensor_get();
    if (s->id.PID == OV3660_PID) {
        s->set_vflip(s, 1); // flip it back
        s->set_brightness(s, 1); // up the brightness just a bit
        s->set_saturation(s, -2); // lower the saturation
    }
    s->set_framesize(s, FRAMESIZE_QVGA);

    is_initialised = true;

    return true;
}

void ei_camera_deinit(void) {
    // Placeholder for potential future implementation
}

bool ei_camera_capture(uint32_t img_width, uint32_t img_height, uint8_t *out_buf) {
    camera_fb_t * fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("Camera capture failed");
        return false;
    }

    if (fb->width != img_width || fb->height != img_height) {
        resizeImage(fb->buf, fb->width, fb->height, out_buf, img_width, img_height);
    } else {
        memcpy(out_buf, fb->buf, fb->len);
    }

    esp_camera_fb_return(fb);
    return true;
}

void resizeImage(const uint8_t* src, int srcWidth, int srcHeight, uint8_t* dst, int dstWidth, int dstHeight) {
    // Nearest neighbor resizing algorithm
    for (int y = 0; y < dstHeight; y++) {
        for (int x = 0; x < dstWidth; x++) {
            int srcX = x * srcWidth / dstWidth;
            int srcY = y * srcHeight / dstHeight;
            dst[(y * dstWidth + x) * 3 + 0] = src[(srcY * srcWidth + srcX) * 3 + 0];
            dst[(y * dstWidth + x) * 3 + 1] = src[(srcY * srcWidth + srcX) * 3 + 1];
            dst[(y * dstWidth + x) * 3 + 2] = src[(srcY * srcWidth + srcX) * 3 + 2];
        }
    }
}

void handleNewMessages(int numNewMessages) {
    Serial.print("Handle New Messages: ");
    Serial.println(numNewMessages);

    for (int i = 0; i < numNewMessages; i++) {
        String chat_id = String(bot.messages[i].chat_id);
        Serial.print("Received Chat ID: ");
        Serial.println(chat_id);

        if (chat_id != CHAT_ID) {
            bot.sendMessage(chat_id, "Unauthorized user", "");
            continue;
        }

        // Print the received message
        String text = bot.messages[i].text;
        Serial.print("Message Text: ");
        Serial.println(text);

        String from_name = bot.messages[i].from_name;
        if (text == "/start") {
            String welcome = "Welcome, " + from_name + "\n";
            welcome += "Use the following commands to interact with the ESP32-CAM \n";
            welcome += "/photo : takes a new photo\n";
            welcome += "/flash : toggles flash LED \n";
            bot.sendMessage(CHAT_ID, welcome, "");
        }
        if (text == "/flash") {
            flashState = !flashState;
            digitalWrite(FLASH_LED_PIN, flashState);
            Serial.println("Change flash LED state");
        }
        if (text == "/photo") {
            sendPhoto = true;
            Serial.println("New photo request");
        }
    }
}

String sendPhotoTelegram() {
    const char* myDomain = "api.telegram.org";
    String getAll = "";
    String getBody = "";

    // Dispose first picture because of bad quality
    camera_fb_t *fb = esp_camera_fb_get();
    esp_camera_fb_return(fb); // dispose the buffered image

    // Take a new photo
    fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("Camera capture failed");
        delay(1000);
        ESP.restart();
        return "Camera capture failed";
    }

    Serial.println("Connect to " + String(myDomain));

    if (clientTCP.connect(myDomain, 443)) {
        Serial.println("Connection successful");

        String head = "--RandomNerdTutorials\r\nContent-Disposition: form-data; name=\"chat_id\"; \r\n\r\n" + String(CHAT_ID) + "\r\n--RandomNerdTutorials\r\nContent-Disposition: form-data; name=\"photo\"; filename=\"esp32-cam.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
        String tail = "\r\n--RandomNerdTutorials--\r\n";

        size_t imageLen = fb->len;
        size_t extraLen = head.length() + tail.length();
        size_t totalLen = imageLen + extraLen;

        clientTCP.println("POST /bot" + String(BOTtoken) + "/sendPhoto HTTP/1.1");
        clientTCP.println("Host: " + String(myDomain));
        clientTCP.println("Content-Length: " + String(totalLen));
        clientTCP.println("Content-Type: multipart/form-data; boundary=RandomNerdTutorials");
        clientTCP.println();
        clientTCP.print(head);

        uint8_t *fbBuf = fb->buf;
        size_t fbLen = fb->len;
        for (size_t n = 0; n < fbLen; n += 1024) {
            if (n + 1024 < fbLen) {
                clientTCP.write(fbBuf, 1024);
                fbBuf += 1024;
            } else if (fbLen % 1024 > 0) {
                size_t remainder = fbLen % 1024;
                clientTCP.write(fbBuf, remainder);
            }
        }

        clientTCP.print(tail);

        esp_camera_fb_return(fb);

        int waitTime = 10000;  // timeout 10 seconds
        long startTimer = millis();
        boolean state = false;

        while ((startTimer + waitTime) > millis()) {
            Serial.print(".");
            delay(100);
            while (clientTCP.available()) {
                char c = clientTCP.read();
                if (state == true) getBody += String(c);
                if (c == '\n') {
                    if (getAll.length() == 0) state = true;
                    getAll = "";
                } else if (c != '\r')
                    getAll += String(c);
                startTimer = millis();
            }
            if (getBody.length() > 0) break;
        }
        clientTCP.stop();
        Serial.println(getBody);
    } else {
        getBody = "Connected to api.telegram.org failed.";
        Serial.println("Connected to api.telegram.org failed.");
    }
    return getBody;
}

static int ei_camera_get_data(size_t offset, size_t length, float *out_ptr) {
    size_t pixels = length / EI_CAMERA_FRAME_BYTE_SIZE;
    for (size_t ix = 0; ix < pixels; ix++) {
        out_ptr[ix] = snapshot_buf[(offset + ix) * EI_CAMERA_FRAME_BYTE_SIZE + 0] / 255.0f;
        out_ptr[ix] += snapshot_buf[(offset + ix) * EI_CAMERA_FRAME_BYTE_SIZE + 1] / 255.0f;
        out_ptr[ix] += snapshot_buf[(offset + ix) * EI_CAMERA_FRAME_BYTE_SIZE + 2] / 255.0f;
        out_ptr[ix] /= 3.0f;
    }
    return 0;
}
