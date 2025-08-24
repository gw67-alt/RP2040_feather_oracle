// Real-Time Gantt Chart Oscilloscope - Minimal Version
// Focuses only on correlation visualization with LED control



#include <Adafruit_NeoPixel.h>

// How many internal neopixels do we have? some boards have more than one!
#define NUMPIXELS        1

Adafruit_NeoPixel pixels(NUMPIXELS, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);

#define NUM_CHANNELS 4
#define SAMPLE_RATE_MS 100      
#define HISTORY_SIZE 20       
#define GANTT_WIDTH 80          
#define GANTT_HISTORY 20        

int analogPins[NUM_CHANNELS] = {A0, A1, A2, A3};

// Minimal data structures
struct GanttData {
  float correlation[NUM_CHANNELS][GANTT_HISTORY];
  float rms[NUM_CHANNELS][GANTT_HISTORY];
  unsigned long timestamps[GANTT_HISTORY];
  int index = 0;
  bool full = false;
};

// Essential variables only
float channelData[NUM_CHANNELS][HISTORY_SIZE];
float instantRMS[NUM_CHANNELS];
float correlation[NUM_CHANNELS];
GanttData gantt;
int dataIndex = 0;
unsigned long lastSample = 0;
bool autoDisplay = true;

void setup() {
  Serial.begin(115200);
  pinMode(13, OUTPUT);  // Set pin 13 as output for LED
      
  #if defined(NEOPIXEL_POWER)
    // If this board has a power control pin, we must set it to output and high
    // in order to enable the NeoPixels. We put this in an #if defined so it can
    // be reused for other boards without compilation errors
    pinMode(NEOPIXEL_POWER, OUTPUT);
    digitalWrite(NEOPIXEL_POWER, HIGH);
  #endif

    pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
    pixels.setBrightness(100); // not so bright
  // Initialize data
  for (int ch = 0; ch < NUM_CHANNELS; ch++) {
    for (int i = 0; i < HISTORY_SIZE; i++) {
      channelData[ch][i] = 0.0;
    }
    instantRMS[ch] = 0.0;
    correlation[ch] = 0.0;
  }
  pinMode(13,OUTPUT);
  //Serial.println("REAL-TIME GANTT CHART OSCILLOSCOPE");
  //Serial.println("==================================");
  //Serial.println("Commands:");
  //Serial.println("  g: Display Gantt chart");
  //Serial.println("  s: Start/stop auto display");
  //Serial.println("LED will light when 3+ channels align");
  //Serial.println("==================================");
}

void loop() {
  unsigned long currentTime = millis();

  // Handle commands
  if (Serial.available()) {
    char cmd = Serial.read();
    if (cmd == 's') {
      autoDisplay = !autoDisplay;
      //Serial.println(autoDisplay ? "Auto display ON" : "Auto display OFF");
    }
  }
  
  // Sample and process data at specified interval
  if (currentTime - lastSample >= SAMPLE_RATE_MS) {
    sampleChannels();
    calculateRMS();
    calculateCorrelation();
    updateGantt();

    // Count channels that meet the alignment criteria
    int mediumSignalCount = 0;
    for (int ch = 0; ch < NUM_CHANNELS; ch++) {
      if (instantRMS[ch] > 0.5 && abs(correlation[ch]) > 0.3) {
        mediumSignalCount++;
      }
    }

    // Light pin 13 LED when 3 or more channels align
    if (mediumSignalCount == 0) {
      // set color to red
      pixels.fill(0x00FF00);
      pixels.show();

    } else {
      
      // turn off
      pixels.fill(0xFF0000);
      pixels.show();
    }
    
    
    dataIndex = (dataIndex + 1) % HISTORY_SIZE;
    lastSample = currentTime;
  }
}

void sampleChannels() {
  for (int ch = 0; ch < NUM_CHANNELS; ch++) {
    int rawValue = analogRead(analogPins[ch]);
    float voltage = (rawValue / 1023.0) * 5.0;
    channelData[ch][dataIndex] = voltage;
  }
}

void calculateRMS() {
  for (int ch = 0; ch < NUM_CHANNELS; ch++) {
    float sumSquares = 0;
    int window = min(5, HISTORY_SIZE);
    
    for (int i = 0; i < window; i++) {
      int idx = (dataIndex - i + HISTORY_SIZE) % HISTORY_SIZE;
      float value = channelData[ch][idx];
      sumSquares += value * value;
    }
    
    instantRMS[ch] = sqrt(sumSquares / window);
  }
}

void calculateCorrelation() {
  for (int ch = 0; ch < NUM_CHANNELS - 1; ch++) {
    float ch1_mean = 0, ch2_mean = 0;
    int window = min(10, HISTORY_SIZE);
    
    for (int i = 0; i < window; i++) {
      int idx = (dataIndex - i + HISTORY_SIZE) % HISTORY_SIZE;
      ch1_mean += channelData[ch][idx];
      ch2_mean += channelData[ch + 1][idx];
    }
    ch1_mean /= window;
    ch2_mean /= window;
    
    float numerator = 0, var1 = 0, var2 = 0;
    for (int i = 0; i < window; i++) {
      int idx = (dataIndex - i + HISTORY_SIZE) % HISTORY_SIZE;
      float diff1 = channelData[ch][idx] - ch1_mean;
      float diff2 = channelData[ch + 1][idx] - ch2_mean;
      
      numerator += diff1 * diff2;
      var1 += diff1 * diff1;
      var2 += diff2 * diff2;
    }
    
    if (var1 > 0 && var2 > 0) {
      correlation[ch] = numerator / sqrt(var1 * var2);
    } else {
      correlation[ch] = 0;
    }
  }
}

void updateGantt() {
  int idx = gantt.index;
  
  for (int ch = 0; ch < NUM_CHANNELS; ch++) {
    gantt.correlation[ch][idx] = (ch < NUM_CHANNELS - 1) ? correlation[ch] : 0.0;
    gantt.rms[ch][idx] = instantRMS[ch];
  }
  
  gantt.timestamps[idx] = millis();
  gantt.index = (idx + 1) % GANTT_HISTORY;
  
  if (!gantt.full && gantt.index == 0) {
    gantt.full = true;
  }
}
