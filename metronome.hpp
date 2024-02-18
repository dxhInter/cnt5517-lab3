#pragma once

#include <cstddef>
#include <chrono>
#include <stdio.h>
class metronome
{
public:
	//store 4 sample beats
	enum { beat_samples = 4 };

public:
	metronome()
	: m_timing(false), m_beat_count(0) {
		// initial the beat samples all to 0
		for(int i = 0; i < beat_samples ; i++){
			m_beats[i] = 0;
		}
	}
	~metronome() {}

public:
	// Call when entering "learn" mode
	void start_timing(){
		std::cout << "Begin timing"<< std::endl;
		m_timing = true;
		firstTap = lastTap = 0;
		m_beat_count = 0;
	}
	// Call when leaving "learn" mode
	void stop_timing(){
		std::cout << "Stop timing"<< std::endl;
		std::cout << "Mode button pressed, now in play mode" << std::endl;
		m_timing = false;
		//at least 4 taps to get new BPM measurment
		if(m_beat_count >= 4) {
			//replace the new BPM with the oldest one
			for(int i = 0 ; i < beat_samples-1 ; i++) {
				m_beats[i] = m_beats[i+1];
			}			
			//calculate seconds between the first and last tap
			double seconds = ((double)(lastTap - firstTap) / (double) 1000);
			if(seconds <= 0){
				// if the time is less than 0, then the BPM is 0
				std::cerr << "Something is wrong with the time calculation"<< lastTap << firstTap << seconds << std::endl;
				m_beats[beat_samples-1] = 0;
			} else{
				// calculate the BPM with minutes
				m_beats[beat_samples-1] = size_t ((double) m_beat_count * ((double) 60 / seconds));
			}
			std::cout << "New BPM calculated: "<< m_beats[beat_samples-1] << std::endl;
		} else {
			std::cerr << "At least four taps for new a BPM measurment" << std::endl;
		}
	}

	/**
	 * @brief Get the current time in milliseconds
	*/
	size_t currentMillis() {
		auto now = std::chrono::system_clock::now();
		auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
		auto epoch = now_ms.time_since_epoch();
		auto value = std::chrono::duration_cast<std::chrono::milliseconds>(epoch);
		return static_cast<size_t>(value.count());
	}

	// Should only record the current time when timing
	// Insert the time at the next free position of m_beats
	void tap(){
		size_t time_stamp = currentMillis();
		if(firstTap == 0){
			//first tap to start the timing
			firstTap = time_stamp;
		}
		lastTap = time_stamp;
		m_beat_count++;
		std::cout<< "beat count: " << m_beat_count << std::endl;
		std::cout<< "m_beats list=[ ";
		for(int i = 0 ; i < beat_samples ; i++){
			std::cout<< m_beats[i] << ", ";
		}
		std::cout<<"]"<<std::endl;
		
	}

	bool is_timing() const { return m_timing; }
	// Calculate the BPM from the deltas between m_beats
	// Return 0 if there are not enough samples
	size_t get_bpm() const {
		//get the latest BPM record
		std::cout<<"Current bpm = "<< m_beats[beat_samples-1] << std::endl;
		return m_beats[beat_samples-1];
		
	}
	std::vector<size_t> get_bpm_list() const {
		std::vector<size_t> bpm_list;
		for(int i = 0 ; i < beat_samples ; i++){
			bpm_list.push_back(m_beats[i]);
		}
		return bpm_list;
	}

	size_t getMIN_MAX(int mode) const {
		size_t ans;
		ans = m_beats[0];
		//mode 0 for get MIN, mode 1 for get MAX
		if (mode == 0) {
			for(int i = 1 ; i < beat_samples ; i++){
				if(m_beats[i] < ans) {
					ans = m_beats[i];
				}
			}
			return ans;
		} else {
			for(int i = 1 ; i < beat_samples ; i++){
				if(m_beats[i] > ans) {
					ans = m_beats[i];
				}
			}
			return ans;
		}
		return 0;
	}

	void delMIN_MAX(int mode) {
		size_t ans = getMIN_MAX(mode);
		for (int i = 0; i< beat_samples ; i++){
			if(m_beats[i] == ans){
				// TODO delete the value
				m_beats[i] = 0;
			}
		}
	}

	//replace the new BPM with the oldest one
	void addNewBPM(size_t val) {
		for(int i = 0 ; i < beat_samples-1 ; i++) {
			m_beats[i] = m_beats[i+1];
		}
		m_beats[beat_samples-1] = val;
	}

	bool deleteBeatByValue(size_t value) {
		if (value == 0) {
			return true;
		}
        for (size_t i = 0; i < beat_samples; ++i) {
            if (m_beats[i] == value) {
                m_beats[i] = 0;
				std::cout << "BPM value "<< value << " deleted" << std::endl;
				return true;
            }
        }
		std::cerr << "BPM value not found" << std::endl;
		return false;
    }
	

private:
	bool m_timing;
	size_t firstTap;
	size_t lastTap;
	// Insert new samples at the end of the array, removing the oldest
	size_t m_beats[beat_samples];
	size_t m_beat_count;
};