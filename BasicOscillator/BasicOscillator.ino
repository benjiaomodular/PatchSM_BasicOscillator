#include "DaisyDuino.h"

DaisyHardware patch;
static Oscillator osc;
static AdEnv adenv;
Switch button;

float voltsPerNote = 0.0833;
float note = 0;
float attack = 0;
float decay = 0.5;

void AudioCallback(float**  in, float** out, size_t size)
{

    float coarse_tune = 12.f + (patch.GetAdcValue(0) * 72.f);
    float freq = mtof(note + coarse_tune);

    /** Set the oscillators to those frequencies */
    osc.SetFreq(freq);


    /** This loop will allow us to process the individual samples of audio */
    for(size_t i = 0; i < size; i++)
    {       
        float sig = osc.Process(); 
        float env_out = adenv.Process();
        sig *= env_out;

        /** In this example both outputs will be the same */
        out[0][i] = out[1][i] = sig;

        // Light up LED based on envelope output
        patch.WriteCvOut(PIN_PATCH_SM_CV_OUT_1, 5 * env_out);
        patch.WriteCvOut(PIN_PATCH_SM_CV_OUT_2, 5 * env_out);
    }

}

void setup()
{
    patch = DAISY.init(DAISY_PATCH_SM);
    adenv.Init(DAISY.AudioSampleRate());

    osc.Init(DAISY.AudioSampleRate());
    osc.SetWaveform(Oscillator::WAVE_SIN);

    button.Init(1000, true, PIN_PATCH_SM_B7, INPUT_PULLUP);

    // Set envelope parameters
    adenv.SetMin(0.0);
    adenv.SetMax(1);
    adenv.SetCurve(0); // linear

    DAISY.StartAudio(AudioCallback);
}

void loop(){
    button.Debounce();
    patch.DebounceControls();
    patch.ProcessAnalogControls();

    // Read mode
    float mode = 5.0 * patch.GetAdcValue(1);
    if (mode < 1.0) {
      osc.SetWaveform(Oscillator::WAVE_SIN);
    } else if (mode >= 1.0 and mode < 2.0) {
      osc.SetWaveform(Oscillator::WAVE_TRI);
    } else if (mode >= 2.0 and mode < 3.0) {
      osc.SetWaveform(Oscillator::WAVE_SAW);
    } else if (mode >= 3.0 and mode < 4.0) {
      osc.SetWaveform(Oscillator::WAVE_RAMP);
    } else if (mode >= 4.0) {
      osc.SetWaveform(Oscillator::WAVE_SQUARE);
    }

    // Read note value
    float note_v = patch.AnalogReadToVolts(analogRead(PIN_PATCH_SM_CV_5));
    note = int(note_v / voltsPerNote);

    // Read attack value
    float atk_pot = patch.AnalogReadToVolts(analogRead(PIN_PATCH_SM_CV_3));
    float atk_cv = patch.AnalogReadToVolts(analogRead(PIN_PATCH_SM_CV_6));
    attack = 0.0001 + (atk_pot + atk_cv) / 5.0;
    adenv.SetTime(ADENV_SEG_ATTACK, attack);

    // Read decay value
    float dec_pot = patch.AnalogReadToVolts(analogRead(PIN_PATCH_SM_CV_4));
    float dec_cv = patch.AnalogReadToVolts(analogRead(PIN_PATCH_SM_CV_7));
    decay = 0.0001 + (dec_pot + dec_cv) / 5.0;
    adenv.SetTime(ADENV_SEG_DECAY, decay);

    // Handle gate inputs
    bool btn_state = button.Pressed();
    bool gate_state = patch.gateIns[0].State();
    if (gate_state or btn_state) {
      adenv.Trigger();
    }
    
}
