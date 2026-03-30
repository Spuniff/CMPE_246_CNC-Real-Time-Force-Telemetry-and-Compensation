#include "HX711.h"



// ============================================================

//  HARDWARE CONFIG

// ============================================================

const int DOUT_PIN = 18;

const int SCK_PIN  = 19;

HX711 scale;



// ============================================================

//  PHYSICS CALIBRATION

//  To find this: apply a known weight (e.g. 1kg = 9.81N),

//  read raw - baseline, divide: SCALE_FACTOR = 9.81 / that_value

// ============================================================

const float SCALE_FACTOR = 0.00045f; // N per ADC tick — MUST CALIBRATE



// ============================================================

//  STAGE 1 — MEDIAN FILTER (Impulse / spike rejection)

//  Kills electrical glitches before they pollute the EMA.

//  Odd number. Larger = better rejection, more latency.

// ============================================================

const int   MEDIAN_N = 9;

long        med_buf[MEDIAN_N];

int         med_head  = 0;

bool        med_full  = false;



long runMedian(long newSample) {

  med_buf[med_head] = newSample;

  med_head = (med_head + 1) % MEDIAN_N;

  if (med_head == 0) med_full = true;



  int  count = med_full ? MEDIAN_N : med_head;

  long tmp[MEDIAN_N];

  memcpy(tmp, med_buf, count * sizeof(long));



  // Insertion sort — totally fine for N ≤ 9

  for (int i = 1; i < count; i++) {

    long k = tmp[i]; int j = i - 1;

    while (j >= 0 && tmp[j] > k) { tmp[j+1] = tmp[j]; j--; }

    tmp[j+1] = k;

  }

  return tmp[count / 2];

}



// ============================================================

//  STAGE 2 — SIGNAL EMA (vibration / HF noise smoothing)

//  α = 0.08 ≈ 12-sample effective window at ~80Hz HX711 rate

//  Lower α → smoother, more lag. Don't go below 0.05.

// ============================================================

const float SIG_ALPHA = 0.08f;

float        sig_ema   = 0;



// ============================================================

//  STAGE 3 — THERMAL BASELINE TRACKER

//  Tracks the DC drift with a very long time constant.

//  At 80Hz: α=0.0003 → ~3333 sample window ≈ 42 seconds

//  This is the "floor" the force rides on top of.

// ============================================================

const float DRIFT_ALPHA = 0.0003f;

float        thermal_baseline = 0;



// ============================================================

//  STAGE 4 — DERIVATIVE DRIFT CORRECTION

//

//  Core insight: thermal drift contributes a near-constant

//  offset to dN/dt. We track this offset (drift_rate_N)

//  and subtract it from every delta.

//

//  CRITICAL GATE: We only UPDATE the drift rate estimate

//  when the corrected delta is small — i.e., when we're

//  confident no real force event is occurring.

//  This prevents the tracker from "learning" real impacts.

//

//  DERIV_DRIFT_ALPHA: how fast we learn the drift rate.

//  At 80Hz: 0.003 → ~333 sample window ≈ 4 seconds

//

//  GATE_THRESHOLD_N: max |ΔN| to still consider "quiet".

//  Set this just above your noise floor. Start at 2.0N.

// ============================================================

const float DERIV_DRIFT_ALPHA   = 0.003f;

const float GATE_THRESHOLD_N    = 2.0f;   // N — tune to your noise floor

float        drift_rate_N        = 0;



// ============================================================

//  E-STOP

//  Now operating on CORRECTED delta, so thermally immune.

// ============================================================

const float CRASH_THRESHOLD_N = 50.0f;



// ============================================================

//  STATE

// ============================================================

float prev_newtons = 0;

bool  initialized  = false;



// ============================================================



void setup() {

  Serial.begin(115200);

  scale.begin(DOUT_PIN, SCK_PIN);

  scale.set_gain(128);

  delay(500);

}



void loop() {

  if (!scale.is_ready()) return;



  long raw = scale.read();



  // --- Stage 1: Median ---

  long med = runMedian(raw);



  // --- Seed everything on first valid sample ---

  if (!initialized) {

    sig_ema          = (float)med;

    thermal_baseline = (float)med;

    drift_rate_N     = 0;

    prev_newtons     = 0;

    initialized      = true;

    return;

  }



  // --- Stage 2: Signal EMA ---

  sig_ema = SIG_ALPHA * med + (1.0f - SIG_ALPHA) * sig_ema;



  // --- Stage 3: Thermal baseline update ---

  thermal_baseline = DRIFT_ALPHA * sig_ema + (1.0f - DRIFT_ALPHA) * thermal_baseline;



  // --- Physics: pure force signal in Newtons ---

  float current_N = (sig_ema - thermal_baseline) * SCALE_FACTOR;



  // --- Stage 4: Derivative drift correction ---

  float raw_delta_N = current_N - prev_newtons;



  //  Gate: only learn the drift rate when the system is "quiet"

  //  so real force events don't corrupt the drift estimate.

  if (fabsf(raw_delta_N) < GATE_THRESHOLD_N) {

    drift_rate_N = DERIV_DRIFT_ALPHA * raw_delta_N


