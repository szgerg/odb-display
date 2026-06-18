#define SIGNAL_PIN 21

const int pwmFreq = 4000;    // 4 kHz frequency
const int pwmResolution = 8; // 8-bit resolution (values 0-255)
const int pwmDuty = 128;     // 50% duty cycle (128 / 255)

void beep_start()
{
    ledcAttach(SIGNAL_PIN, pwmFreq, pwmResolution);

    ledcWrite(SIGNAL_PIN, pwmDuty);
}

void beep_stop()
{
    ledcWrite(SIGNAL_PIN, 0);
}
