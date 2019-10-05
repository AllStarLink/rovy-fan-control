// https://gist.github.com/dschnabel/b74f7d91b5637240a0705ab0822bc3f7

#include <fstream>
#include <string>
#include <sstream>
#include <iostream>
#include <math.h>

#include <wiringpi/wiringPi.h>

#define PWM_PIN 1

#define FAN_OFF 0

#define FAN_MIN_VOLT 2
#define FAN_MAX_VOLT 5

#define CPU_TARGET_TEMP 45

using namespace std;

float getCPUTemp() {
	string str;
	ifstream file("/sys/class/thermal/thermal_zone0/temp");
    getline(file, str);
    file.close();

    istringstream iss(str);
    float number;
    iss >> number;
    number /= 1000;

    return number;
}

int voltageToSpeed(double volt) {
	double a = -2635.761;
	double b = 4442.3;
	double c = 2645.764;
	double d = 772.5762;
	double e = 110.5112;
	double f = 6.210389;

	// quintic regression
	int speed = (a) + (b * volt) - (c * pow(volt,2)) + (d * pow(volt,3)) - (e * pow(volt,4)) + (f * pow(volt,5));
	return speed > 0 ? speed : 0;
}

void changeFanSpeed(int speed) {
	static int maxSpeed = voltageToSpeed(FAN_MAX_VOLT);

	if (speed != FAN_OFF) {
		pwmWrite(PWM_PIN, maxSpeed);
		delay(10);
	}

	pwmWrite(PWM_PIN, speed);
}

void setup() {
	wiringPiSetup();
	pinMode(PWM_PIN, PWM_OUTPUT);
}

int main(int argc, char *argv[]) {
	setup();

//	istringstream iss(argv[1]);
//	int speed;
//	iss >> speed;
//	changeFanSpeed(speed);

	while (getCPUTemp() < 45) {
		delay(5000);
	}

	float sum = 0;
	bool isLimit = false;

	while (1) {
		float temp = getCPUTemp();
		float diff = temp - CPU_TARGET_TEMP;
		sum += isLimit ? 0 : diff;
		float pDiff = diff * 0.3;//diff * 5;
		float iDiff = sum * 0.08;//sum * 4;
		double volt = FAN_MIN_VOLT - 1 + pDiff + iDiff;

		isLimit = false;
		if (volt > FAN_MAX_VOLT) {
			isLimit = true;
			volt = FAN_MAX_VOLT;
		}
		if (volt < FAN_MIN_VOLT) {
			isLimit = true;
			volt = FAN_OFF;
		}

		int speed = voltageToSpeed(volt);

//		cout << "temp:" << temp << ",diff:" << diff << ",sum:" << sum <<
//				",pDiff:" << pDiff << ",iDiff:" << iDiff << ",volt:" << volt << ",speed:" << speed << endl;

		changeFanSpeed(speed);
		delay(5000);
	}

	return 0;
}
