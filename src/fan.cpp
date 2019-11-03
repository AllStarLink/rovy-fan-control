// https://gist.github.com/dschnabel/b74f7d91b5637240a0705ab0822bc3f7

#include <fstream>
#include <string>
#include <sstream>
#include <iostream>
#include <math.h>

#include <wiringpi/wiringPi.h>
#include <wiringpi/softPwm.h>

#define PWM_PIN 0

#define FAN_OFF 0

#define FAN_MIN_VOLT 2.45
#define FAN_MAX_VOLT 5.20

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
	if (volt <= 0) return FAN_OFF;

	double a = 3899.414;
	double b = 5013.603;
	double c = 2265.318;
	double d = 421.6892;
	double e = 28.97351;

	// quartic regression
	int speed = (a) - (b * volt) + (c * pow(volt,2)) - (d * pow(volt,3)) + (e * pow(volt,4));
	return min(speed, 1000);
}

void changeFanSpeed(int speed) {
	static int maxSpeed = voltageToSpeed(FAN_MAX_VOLT);

	if (speed != FAN_OFF) {
		softPwmWrite(PWM_PIN, maxSpeed);
		delay(100);
	}

	softPwmWrite(PWM_PIN, speed);
}

void setup() {
	wiringPiSetup();
	softPwmCreate(PWM_PIN, 0, 1000);
}

int main(int argc, char *argv[]) {
	setup();

//	istringstream iss(argv[1]);
//	int speed;
//	iss >> speed;
//	softPwmWrite(PWM_PIN, 1000);
//	delay(100);
//	softPwmWrite(PWM_PIN, speed);
//	while (1) delay(5000);

	while (getCPUTemp() < 45) {
		delay(5000);
	}

	float sum = 0;
	bool isLimit = false;

	while (1) {
		float temp = getCPUTemp();
		float diff = temp - CPU_TARGET_TEMP;
		sum += isLimit ? 0 : diff;
		float pDiff = diff * 0.4;
		float iDiff = sum * 0.2;
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
