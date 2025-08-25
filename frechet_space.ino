// ====================================================================
// Real-Time Mathematical Fréchet Distance Oscilloscope
// ====================================================================
// Implements true discrete Fréchet distance using dynamic programming
// Based on: F(A,B) = inf_{α,β} max_{t∈[0,1]} d(A(α(t)), B(β(t)))
// ====================================================================

#define NUM_CHANNELS 4
#define SAMPLE_RATE_MS 100    
#define HISTORY_SIZE 50       // Smaller for DP algorithm
#define FRECHET_WINDOW 20     // Window size for Fréchet computation

int analogPins[NUM_CHANNELS] = {A0, A1, A2, A3};

// Mathematical structures
float channelData[NUM_CHANNELS][HISTORY_SIZE];
float instantRMS[NUM_CHANNELS];
float frechetDistances[NUM_CHANNELS];

int dataIndex = 0;
unsigned long lastSample = 0;
bool autoOutput = true;

// DP table for discrete Fréchet distance (static allocation)
float dpTable[FRECHET_WINDOW][FRECHET_WINDOW];

// ----------------------------- SETUP --------------------------------

void setup() {
  Serial.begin(115200);

  for (int ch = 0; ch < NUM_CHANNELS; ch++) {
    for (int i = 0; i < HISTORY_SIZE; i++) {
      channelData[ch][i] = 0.0;
    }
    instantRMS[ch] = 0.0;
    frechetDistances[ch] = 0.0;
  }

  // Mathematical CSV header
  Serial.println("timestamp,A0_rms,A0_frechet,A1_rms,A1_frechet,A2_rms,A2_frechet,A3_rms,A3_frechet");
}

// ----------------------------- LOOP ---------------------------------

void loop() {
  unsigned long currentTime = millis();

  if (Serial.available()) {
    char cmd = Serial.read();
    if (cmd == 's') {
      autoOutput = !autoOutput;
      Serial.print("# Auto output ");
      Serial.println(autoOutput ? "ON" : "OFF");
    }
  }

  if (currentTime - lastSample >= SAMPLE_RATE_MS) {
    sampleChannels();
    calculateRMS();
    calculateDiscreteFrechetDistances();
    
    if (autoOutput) {
      outputMathematicalValues();
    }

    dataIndex = (dataIndex + 1) % HISTORY_SIZE;
    lastSample = currentTime;
  }
}

// ---------------------------- SAMPLING -------------------------------

void sampleChannels() {
  for (int ch = 0; ch < NUM_CHANNELS; ch++) {
    int rawValue = analogRead(analogPins[ch]);
    float voltage = (rawValue / 1023.0) * 5.0;
    channelData[ch][dataIndex] = voltage;
  }
}

// ----------------------------- RMS ----------------------------------

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

// ---------------------- MATHEMATICAL FRÉCHET DISTANCE ---------------

// Euclidean distance between two points
inline float euclideanDistance(float a, float b) {
  return abs(a - b);
}

// True discrete Fréchet distance using dynamic programming
// F(A,B) = min over all reparameterizations of max point-to-point distance
float discreteFrechetDistance(int ch1, int ch2, int windowSize) {
  // Extract recent data windows for both channels
  float curve1[FRECHET_WINDOW];
  float curve2[FRECHET_WINDOW];
  
  int actualWindow = min(windowSize, min(FRECHET_WINDOW, HISTORY_SIZE));
  
  for (int i = 0; i < actualWindow; i++) {
    int idx = (dataIndex - actualWindow + 1 + i + HISTORY_SIZE) % HISTORY_SIZE;
    curve1[i] = channelData[ch1][idx];
    curve2[i] = channelData[ch2][idx];
  }

  // Initialize DP table
  for (int i = 0; i < actualWindow; i++) {
    for (int j = 0; j < actualWindow; j++) {
      dpTable[i][j] = -1.0;
    }
  }

  // Recursive DP function (with memoization)
  return frechetDP(curve1, curve2, actualWindow - 1, actualWindow - 1, actualWindow);
}

// Dynamic programming recurrence for discrete Fréchet distance
float frechetDP(float* curve1, float* curve2, int i, int j, int windowSize) {
  if (i < 0 || j < 0) return INFINITY;
  if (dpTable[i][j] >= 0) return dpTable[i][j];
  
  float pointDistance = euclideanDistance(curve1[i], curve2[j]);
  
  if (i == 0 && j == 0) {
    dpTable[i][j] = pointDistance;
  } else if (i > 0 && j == 0) {
    dpTable[i][j] = max(frechetDP(curve1, curve2, i-1, 0, windowSize), pointDistance);
  } else if (i == 0 && j > 0) {
    dpTable[i][j] = max(frechetDP(curve1, curve2, 0, j-1, windowSize), pointDistance);
  } else {
    // Main recurrence: F(i,j) = max(min(F(i-1,j), F(i-1,j-1), F(i,j-1)), d(pi,qj))
    float option1 = frechetDP(curve1, curve2, i-1, j, windowSize);
    float option2 = frechetDP(curve1, curve2, i-1, j-1, windowSize);
    float option3 = frechetDP(curve1, curve2, i, j-1, windowSize);
    
    dpTable[i][j] = max(min(option1, min(option2, option3)), pointDistance);
  }
  
  return dpTable[i][j];
}

// Calculate Fréchet distances between adjacent channel pairs
void calculateDiscreteFrechetDistances() {
  int window = min(FRECHET_WINDOW, min(20, dataIndex + 1));
  
  if (window < 3) { // Need minimum data for meaningful computation
    for (int ch = 0; ch < NUM_CHANNELS; ch++) {
      frechetDistances[ch] = 0.0;
    }
    return;
  }

  for (int ch = 0; ch < NUM_CHANNELS - 1; ch++) {
    frechetDistances[ch] = discreteFrechetDistance(ch, ch + 1, window);
  }
  frechetDistances[NUM_CHANNELS - 1] = 0.0; // No pair for last channel
}

// ---------------------------- OUTPUT --------------------------------

void outputMathematicalValues() {
  Serial.print(millis());
  Serial.print(" ");
  Serial.print(frechetDistances[NUM_CHANNELS-2], 6);
 
  Serial.println();
}

// Utility function for max of two floats
float max(float a, float b) {
  return (a > b) ? a : b;
}

// Utility function for min of two floats  
float min(float a, float b) {
  return (a < b) ? a : b;
}
