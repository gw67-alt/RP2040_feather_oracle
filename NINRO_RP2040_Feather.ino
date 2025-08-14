// Real-Time Gantt Chart Oscilloscope - Minimal Version
// Focuses only on correlation visualization

#define NUM_CHANNELS 4
#define SAMPLE_RATE_MS 100    // Slower for real-time visibility
#define HISTORY_SIZE 200      // Smaller buffer for speed
#define GANTT_WIDTH 40       // Compact display
#define GANTT_HISTORY 20     // Real-time window

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

void setup() {
  Serial.begin(115200);
  
  // Initialize data
  for (int ch = 0; ch < NUM_CHANNELS; ch++) {
    for (int i = 0; i < HISTORY_SIZE; i++) {
      channelData[ch][i] = 0.0;
    }
    instantRMS[ch] = 0.0;
    correlation[ch] = 0.0;
  }
  
  Serial.println("REAL-TIME GANTT CHART OSCILLOSCOPE");
  Serial.println("==================================");
  Serial.println("Commands:");
  Serial.println("  g: Display Gantt chart");
  Serial.println("  s: Start/stop auto display");
  Serial.println("==================================");
}

bool autoDisplay = true;

void loop() {
  unsigned long currentTime = millis();
  
  // Handle commands
  if (Serial.available()) {
    char cmd = Serial.read();
    if (cmd == 'g') {
      displayGanttChart();
    } else if (cmd == 's') {
      autoDisplay = !autoDisplay;
      Serial.println(autoDisplay ? "Auto display ON" : "Auto display OFF");
    }
  }
  
  // Sample and process data
  if (currentTime - lastSample >= SAMPLE_RATE_MS) {
    sampleChannels();
    calculateRMS();
    calculateCorrelation();
    updateGantt();
    
    // Auto-display Gantt chart in real-time
    if (autoDisplay && gantt.index % 2 == 0) { // Update every 2nd sample
      displayGanttChart();
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
    int window = min(5, HISTORY_SIZE); // Small window for speed
    
    for (int i = 0; i < window; i++) {
      int idx = (dataIndex - i + HISTORY_SIZE) % HISTORY_SIZE;
      float value = channelData[ch][idx];
      sumSquares += value * value;
    }
    
    instantRMS[ch] = sqrt(sumSquares / window);
  }
}

void calculateCorrelation() {
  // Simple correlation between channels
  for (int ch = 0; ch < NUM_CHANNELS - 1; ch++) {
    float ch1_mean = 0, ch2_mean = 0;
    int window = min(10, HISTORY_SIZE);
    
    // Calculate means
    for (int i = 0; i < window; i++) {
      int idx = (dataIndex - i + HISTORY_SIZE) % HISTORY_SIZE;
      ch1_mean += channelData[ch][idx];
      ch2_mean += channelData[ch + 1][idx];
    }
    ch1_mean /= window;
    ch2_mean /= window;
    
    // Calculate correlation
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
  
  // Store current data
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

void displayGanttChart() {
  // Clear screen for real-time effect
  Serial.println("████ REAL-TIME CORRELATION GANTT ███");
  
  int entries = gantt.full ? GANTT_HISTORY : gantt.index;
  if (entries < 2) return;
  
  // Header
  Serial.print("CH |");
  for (int i = 0; i < min(entries, GANTT_WIDTH); i++) {
    if (i % 5 == 0) Serial.print("|");
    else Serial.print("-");
  }
  Serial.println("| RMS  | CORR");
  
  // Display channels
  for (int ch = 0; ch < NUM_CHANNELS; ch++) {
    if (instantRMS[ch] < 0.01) continue;
    
    Serial.print("A"); Serial.print(ch); Serial.print(" |");
    
    // Show correlation timeline
    for (int i = 0; i < min(entries, GANTT_WIDTH); i++) {
      int idx = (gantt.index - entries + i + GANTT_HISTORY) % GANTT_HISTORY;
      float corr = gantt.correlation[ch][idx];
      float rms = gantt.rms[ch][idx];
      
      // Visual representation based on both correlation and RMS
      if (rms > 1.0 && abs(corr) > 0.6) {
        Serial.print("█"); // Strong signal + high correlation
      } else if (rms > 0.5 && abs(corr) > 0.3) {
        Serial.print("▓"); // Medium signal + medium correlation  
      } else if (rms > 0.1) {
        Serial.print("░"); // Weak signal
      } else {
        Serial.print(" "); // No signal
      }
    }
    
    // Current values
    Serial.print("| ");
    Serial.print(instantRMS[ch], 2);
    Serial.print("| ");
    if (ch < NUM_CHANNELS - 1) {
      Serial.print(correlation[ch], 2);
    } else {
      Serial.print("  - ");
    }
    Serial.println();
  }
  
  // Footer
  Serial.print("   |");
  for (int i = 0; i < min(entries, GANTT_WIDTH); i++) {
    if (i % 5 == 0) Serial.print("|");
    else Serial.print("-");
  }
  Serial.println("|");
  
  // Time indicators
  Serial.print("Time: ");
  for (int i = 0; i < min(entries, GANTT_WIDTH); i += 5) {
    int idx = (gantt.index - entries + i + GANTT_HISTORY) % GANTT_HISTORY;
    unsigned long timeAgo = (millis() - gantt.timestamps[idx]) / 1000;
    Serial.print(timeAgo);
    Serial.print("s  ");
  }
  Serial.println();
  
  Serial.println("Legend: █=Strong ▓=Medium ░=Weak  =None");
  Serial.print("Sample Rate: "); Serial.print(SAMPLE_RATE_MS); Serial.println("ms");
  Serial.println("████████████████████████████████████");
}
