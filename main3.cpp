#include <chrono>
#include <thread>
#include <pigpio.h>
using namespace std::chrono_literals;

#include <cpprest/http_msg.h>
// #include <wiringPi.h>

#include "metronome.hpp"
#include "rest.hpp"

// ** Remember to update these numbers to your personal setup. **
#define LED_RED   27
#define LED_GREEN 27
#define BTN_MODE  24
#define BTN_TAP   24

// Mark as volatile to ensure it works multi-threaded.
volatile bool btn_mode_pressed = false;

// Run an additional loop separate from the main one.
void blink() {
	bool on = false;
	// ** This loop manages LED blinking. **
	while (true) {
		// The LED state will toggle once every second.
		std::this_thread::sleep_for(1s);

		// Perform the blink if we are pressed,
		// otherwise just set it to off.
		if (btn_mode_pressed)
			on = !on;
		else
			on = false;
		// HIGH/LOW probably equal 1/0, but be explicit anyways.
		gpioWrite(LED_RED, (on) ? PI_HIGH : PI_LOW);
	}
}

// This is called when a GET request is made to "/answer".
void get42(web::http::http_request msg) {
	msg.reply(200, web::json::value::number(42));
}

// This program shows an example of input/output with GPIO, along with
// a dummy REST service.
// ** You will have to replace this with your metronome logic, but the
//    structure of this program is very relevant to what you need. **
int main() {
	// WiringPi must be initialized at the very start.
	// This setup method uses the Broadcom pin numbers. These are the
	// larger numbers like 17, 24, etc, not the 0-16 virtual ones.
	// wiringPiSetupGpio();
	if (gpioInitialise() < 0) {
        std::cout << "pigpio initialization failed." << std::endl;
        return 1;
    }

	// Set up the directions of the pins.
	// Be careful here, an input pin set as an output could burn out.
	// pinMode(LED_RED, OUTPUT);
	// pinMode(BTN_MODE, INPUT);
	gpioSetMode(LED_RED, PI_OUTPUT);
    gpioSetMode(BTN_MODE, PI_INPUT);
    gpioSetPullUpDown(BTN_MODE, PI_PUD_UP);
	// Note that you can also set a pull-down here for the button,
	// if you do not want to use the physical resistor.
	//pullUpDnControl(BTN_MODE, PUD_DOWN);

	// Configure the rest services after setting up the pins,
	// but before we start using them.
	// ** You should replace these with the BPM endpoints. **
	auto get42_rest = rest::make_endpoint("/answer");
	get42_rest.support(web::http::methods::GET, get42);

	// Start the endpoints in sequence.
	get42_rest.open().wait();

	int pinState = gpioRead(BTN_MODE);
    std::cout << "GPIO 24 state is: " << pinState << std::endl;
	// Use a separate thread for the blinking.
	// This way we do not have to worry about any delays
	// caused by polling the button state / etc.
	std::thread blink_thread(blink);
	blink_thread.detach();

	// ** This loop manages reading button state. **
	while (true) {
		// We are using a pull-down, so pressed = HIGH.
		// If you used a pull-up, compare with LOW instead.
		btn_mode_pressed = (gpioRead(BTN_MODE) == PI_LOW);
		// Delay a little bit so we do not poll too heavily.
		std::this_thread::sleep_for(10ms);
		// if (btn_mode_pressed == true){
		// 	std::cout<< btn_mode_pressed<<std::endl;
		// }

		// ** Note that the above is for testing when the button
		// is held down. For detecting "taps", or momentary
		// pushes, you need to check for a "rising edge" -
		// this is when the button changes from LOW to HIGH... **
	}
	gpioTerminate();
	return 0;
}
