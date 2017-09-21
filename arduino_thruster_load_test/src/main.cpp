/****************************************************************************
 *
 *   Copyright (c) 2017 Ali AlSaibie. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/**
 * @file 
 * 
 *
 * @author Ali AlSaibie
 */
#include <Arduino.h>
#include <ArduinoJson.h>
#include <Servo.h>
#define BAUD 115200

#include <stdint.h>

/* TODO: Make sure to sync and update between the class and here */
enum COMMANDS {
    START = 1,
    STOP
};

/* Generate a ramp between -100 and 100: 0 to 100, 100 to -100, -100 to 0 */
int gen_ramp_value(unsigned int sample, unsigned int no_samples){
  if(sample <= no_samples / 4) {
    // Mid to Max
    return (100 *sample) / (no_samples / 4);
  }
  else if(sample > no_samples / 4 && sample <= 3 * no_samples / 4){
    // Max to Min
    return 100 - 200 * (sample - no_samples /4) / (no_samples / 2);

  }
  else if (sample > 3 * no_samples / 4){
    // Min to Mid
    return 100 *(sample - 3 * no_samples / 4) / (no_samples / 4) - 100;
  }
}

unsigned int gen_pwm_ramp(unsigned int sample, unsigned int no_samples){
  return (255 / 2  * gen_ramp_value(sample, no_samples)) / 100 + 255 / 2;
}

void setup() {
  unsigned int pwm_out = 255 / 2;
  unsigned int curr_in = 0;
  unsigned int test_counter = 0;
  unsigned int samples_len = 60;
  int current_pin = 0;
  unsigned int pwm1_pin = 2;
  bool test_finished = false;
  /* Set pins */
//  analogReference(DEFAULT);
//  analogReadResolution(12);
//  analogReadAveraging(32);
  Servo esc1;
  esc1.attach(pwm1_pin);
  esc1.write(0);
  /*JSON Setup */
  StaticJsonBuffer<200> jsonOutgoingBuffer;
  StaticJsonBuffer<200> jsonIncomingBuffer;

  JsonObject &rootOutgoing = jsonOutgoingBuffer.createObject();

  /* Serial Start */
  Serial.begin(BAUD);
  while (!Serial) {}
  Serial.setTimeout(10);
  char start_command = 'P';

  /* Generate PWM Function */


  while (1) {

    if (Serial.available() > 0) {
      String incomingString = Serial.readStringUntil('\n');
      if (incomingString != "") {
        jsonIncomingBuffer.clear();
        JsonObject &rootIncoming = jsonIncomingBuffer.parseObject(incomingString);
        if (rootIncoming["Event"] == "Command") {
          start_command = (char) rootIncoming["StartCommand"];
          if (start_command == 'S') {
            test_finished = false;
            test_counter = 0;
          }
          samples_len = rootIncoming["SNo"];
        }
      }
    }

    /*Make sure output is disabled if not running test */

    if (start_command == 'P') {
      esc1.write(180 / 2);
    }
    if (start_command == 'S' && test_counter < samples_len) {
      test_counter++;
      /*  Loop through PWM outputs */
      pwm_out = gen_pwm_ramp(test_counter, samples_len);
      if (pwm_out > 255) {
        pwm_out = 255 / 2; } // Mid is zero in our test }
        int throttle = map(throttle, 0, 255, 0, 179);
       // esc1.write(throttle);

        /* Wait for speed to climb and current to stabilize */
        /* Also Delay, giving a chance for the lazy slow cheap scale to give a measurement on the PC side */
        delay(3500);

        /* Read current TODO: scale accordingly */
        //curr_in = analogRead(current_pin);

        /* and report current and p   wm */
        rootOutgoing["SampleNo"] = test_counter;
        rootOutgoing["Current"] = curr_in;
        rootOutgoing["PWM"] = pwm_out;
        if (test_counter == samples_len) {
          start_command = 'P';
          test_counter = 0;
          test_finished = true;
        }
        rootOutgoing["TestFinished"] = test_finished;
        rootOutgoing.printTo(Serial);
        Serial.write('\n');

      }

    }
  }


void loop() {
  /* Not Used */
}