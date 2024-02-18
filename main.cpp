#include <chrono>
#include <thread>
#include <string>
using namespace std::chrono_literals;
#include <cpprest/json.h>
#include <cpprest/http_msg.h>
// #include <wiringPi.h>
#include <pigpio.h>

#include "metronome.hpp"
#include "rest.hpp"

// ** Remember to update these numbers to your personal setup. **
#define LED_RED   17
#define LED_GREEN 27
#define BTN_MODE  23
#define BTN_TAP   24

#define MIN 0
#define MAX 1

// Mark as volatile to enable thread safety.
volatile bool btn_mode_pressed = false;
volatile bool is_startup = true;
size_t blink_frequency;
metronome metro;
std::vector<size_t> bpm_list;

// Run an additional loop separate from the main one.
void blink() {
	bool on = false;
	
	// ** This loop manages LED blinking. **
	while (true) {
		// The LED state will toggle once every second.
		std::this_thread::sleep_for(std::chrono::milliseconds(blink_frequency));
		// Perform the blink if we are pressed,
		// otherwise just set it to off.
		if (!metro.is_timing() && !is_startup){
			gpioWrite(LED_GREEN,PI_HIGH);
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			gpioWrite(LED_GREEN,PI_LOW);
		}
	}
}

static void check_tap_button () {
	gpioWrite(LED_RED,PI_HIGH);
	std::this_thread::sleep_for(std::chrono::milliseconds(10));
	gpioWrite(LED_RED,PI_LOW);

}

/**
 * @brief Override the msg.reply to add the Access-Control-Allow-Origin header and the content type
 * @param msg the http request
 * @param content the content to be sent
 * @param mode 1 to send the json witg number(BPM), 0 to send response
*/
void msg_reply(web::http::http_request msg, size_t content, int mode) {
	web::http::http_response response(web::http::status_codes::OK);
	response.headers().set_content_type(U("application/json"));
    response.headers().add(U("Access-Control-Allow-Origin"), U("*"));
	web::json::value jsonResponse;
	if(mode == 1){
    	jsonResponse[U("bpm")] = web::json::value::number(content);
		response.set_body(jsonResponse);
	}
	// }else if(mode == 2){
	// 	web::json::value jsonResponse;
	// 	web::json::value json_bpm_list = web::json::value::array();
    //     // 如果 bpm_list 不为空，则填充 JSON 数组
    //     if (!bpm_list.empty()) {
    //         for (size_t i = 0; i < bpm_list.size(); ++i) {
    //         	json_bpm_list[i] = web::json::value::number(bpm_list[i]);
    //     	}
    //     }
    //     jsonResponse[U("bpm_list")] = json_bpm_list;
	// 	response.set_body(jsonResponse);
	// }
	else{
    	jsonResponse[U("msg")] = web::json::value::string("your request has been processed successfully!");
		response.set_body(jsonResponse);
	}
	msg.reply(response);
}

void msg_wrong_reply(web::http::http_request msg, std::string content, web::http::status_code status_code){
	web::http::http_response response(status_code);
	response.headers().set_content_type(U("application/json"));
	response.headers().add(U("Access-Control-Allow-Origin"), U("*"));
	web::json::value jsonResponse;
	jsonResponse[U("msg")] = web::json::value::string(content);
	response.set_body(jsonResponse);
	msg.reply(response);
}


void updateBPM() {
	blink_frequency = metro.get_bpm();
	if(blink_frequency != 0){
		blink_frequency = (60*1000) / blink_frequency;
		is_startup = false;
	} else{
		is_startup = true;	
	}
}


void getBPM(web::http::http_request msg) {
	try{
		msg_reply(msg, metro.get_bpm(), 1);
	}catch(const std::exception& e){
		// msg.reply(500, U("Internal Server Error"));
		msg_wrong_reply(msg, "Internal Server Error", web::http::status_codes::InternalError);
	}
}

void getBPMlist(web::http::http_request msg) {
	bpm_list = metro.get_bpm_list();
	try{
		web::http::http_response response(web::http::status_codes::OK);
		response.headers().set_content_type(U("application/json"));
   		response.headers().add(U("Access-Control-Allow-Origin"), U("*"));
		web::json::value jsonResponse;
		web::json::value json_bpm_list = web::json::value::array();
        if (!bpm_list.empty()) {
            for (size_t i = 0; i < bpm_list.size(); ++i) {
            	json_bpm_list[i] = web::json::value::number(bpm_list[i]);
        	}
        }
        jsonResponse[U("bpm_list")] = json_bpm_list;
		response.set_body(jsonResponse);
		msg.reply(response);
	}catch(const std::exception& e){
		// msg.reply(500, U("Internal Server Error"));
		msg_wrong_reply(msg, "Internal Server Error", web::http::status_codes::InternalError);
	}
}

void getMIN(web::http::http_request msg) {
	msg_reply(msg, metro.getMIN_MAX(MIN), 1);
}
void getMAX(web::http::http_request msg) {
	msg_reply(msg, metro.getMIN_MAX(MAX), 1);
}

void delMIN(web::http::http_request msg) {
	metro.delMIN_MAX(MIN);
	updateBPM();
	msg_reply(msg, 0 ,0);
}

void delMAX(web::http::http_request msg) {
	metro.delMIN_MAX(MAX);
	updateBPM();
	msg_reply(msg, 0 ,0);
}

void setBPM(web::http::http_request msg){
	msg.extract_json().then([=](web::json::value req_body){
		try{
			auto bpmValue = req_body[U("bpm")].as_integer();
            metro.addNewBPM(bpmValue);
			updateBPM();
            msg_reply(msg, 0, 0);
		}catch(const std::exception& e){
			// msg.reply(400, U("Bad Request"));
			msg_wrong_reply(msg, "Bad Request", web::http::status_codes::BadRequest);
			std::cerr << e.what() << std::endl;
		}
	}).wait();
}

void deleteBPM(web::http::http_request msg){
	msg.extract_json().then([=](web::json::value req_body){
		try{
			auto bpmValue = req_body[U("bpm")].as_integer();
			bool isSuccess = metro.deleteBeatByValue(bpmValue);
			updateBPM();
			if (isSuccess == false){
				// msg.reply(400, U("Bad Request, please check the bpm value"));
				msg_wrong_reply(msg, "Bad Request, please check the bpm value", web::http::status_codes::BadRequest);
			}else {
				msg_reply(msg, 0, 0);
			}
		}catch(const std::exception& e){
			// msg.reply(400, U("Bad Request, please check the bpm value"));
			msg_wrong_reply(msg, "Bad Request, please check the bpm value", web::http::status_codes::BadRequest);
			std::cerr << e.what() << std::endl;
		}
	});
}

// This program shows an example of input/output with GPIO, along with
// a dummy REST service.
// ** You will have to replace this with your metronome logic, but the
//    structure of this program is very relevant to what you need. **
int main() {
	// WiringPi must be initialized at the very start.
	// This setup method uses the Broadcom pin numbers. These are the
	// larger numbers like 17, 24, etc, not the 0-16 virtual ones.
	if (gpioInitialise() < 0) {
        std::cerr << "pigpio initialization failed." << std::endl;
        return 1;
    }

	// Set up the directions of the pins.
	// Be careful here, an input pin set as an output could burn out.
	gpioSetMode(LED_RED, PI_OUTPUT);
	gpioSetMode(LED_GREEN, PI_OUTPUT);
    gpioSetMode(BTN_MODE, PI_INPUT);
	gpioSetMode(BTN_TAP, PI_INPUT);
    gpioSetPullUpDown(BTN_MODE, PI_PUD_UP);
	gpioSetPullUpDown(BTN_TAP, PI_PUD_UP);
	// Note that you can also set a pull-down here for the button,
	// if you do not want to use the physical resistor.
	//pullUpDnControl(BTN_MODE, PUD_DOWN);

	// Configure the rest services after setting up the pins,
	// but before we start using them.
	// ** You should replace these with the BPM endpoints. **

	auto getBPM_rest = rest::make_endpoint("/bpm");
	auto getBPMLIST_rest = rest::make_endpoint("/bpm/list");
	auto getMIN_rest = rest::make_endpoint("/bpm/min");
	auto getMAX_rest = rest::make_endpoint("/bpm/max");

	getBPM_rest.support(web::http::methods::GET, getBPM);
	getBPM_rest.support(web::http::methods::PUT, setBPM);
	getMIN_rest.support(web::http::methods::GET, getMIN);
	getMIN_rest.support(web::http::methods::DEL, delMIN);
	getMAX_rest.support(web::http::methods::GET, getMAX);
	getMAX_rest.support(web::http::methods::DEL, delMAX);
	getBPMLIST_rest.support(web::http::methods::GET, getBPMlist);
	getBPMLIST_rest.support(web::http::methods::DEL, deleteBPM);

	// Start the endpoints in sequence.

	getBPM_rest.open().wait();
	getMIN_rest.open().wait();
	getMAX_rest.open().wait();
	getBPMLIST_rest.open().wait();
	//metronome metro;

	// Use a separate thread for the blinking.
	// This way we do not have to worry about any delays
	// caused by polling the button state / etc.
	std::thread blink_thread(blink);
	blink_thread.detach();


	//initial the blink frequency
	blink_frequency = 1000;
	

	// ** This loop manages reading button state. **
	while (true) {
		// We are using a pull-down, so pressed = HIGH.
		// If you used a pull-up, compare with LOW instead.
		//btn_mode_pressed = (digitalRead(BTN_MODE) == HIGH);
		
		//Learn Mode
		if(gpioRead(BTN_MODE) == PI_LOW){
			std::cout << "Mode button pressed, now in learn mode" << std::endl;			
			if(!metro.is_timing())
				metro.start_timing();
			else {
				metro.stop_timing();
				updateBPM();
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(200));
		}
		
		
		if(metro.is_timing()){
			if(gpioRead(BTN_TAP) == PI_LOW){
				std::cout << "Tap button pressed" << std::endl;
				metro.tap();
				check_tap_button();
				std::this_thread::sleep_for(std::chrono::milliseconds(200));
			}
		}
		
		std::this_thread::sleep_for(10ms);

		// ** Note that the above is for testing when the button
		// is held down. For detecting "taps", or momentary
		// pushes, you need to check for a "rising edge" -
		// this is when the button changes from LOW to HIGH... **
	}
	gpioTerminate();
	return 0;
}