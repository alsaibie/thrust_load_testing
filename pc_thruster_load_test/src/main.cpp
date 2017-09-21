#include <iostream>
#include <fstream>
#include <unistd.h>
#include <cstdlib>
#include <string>
#include "arduino_interface.h"
#include "json.hpp"
#include "usbscale.h"
#define DEBUG
#define wait_a_sec 1000000L

using json = nlohmann::json;
using namespace std;

int main()
{
  /*Get user inputs*/
  bool user_input_sucess = false;
  string sampleno_input = "";
  while(!user_input_sucess){
    /* Get test number */
    cout <<"Please type the test number:"<<endl;
    getline(cin, sampleno_input);
//    cin >> sampleno_input;
    cout <<"You entered: " << sampleno_input << ". To Continue Press Y, or any other key to re-enter value."<< endl;
    string yesnay = "";
    getline(cin, yesnay);
    if (yesnay == "Y"){
      user_input_sucess = true;
    }
  }


  string out_file_name_ = "../data/test_output" + sampleno_input + ".txt";
  ofstream out_file_(out_file_name_.c_str(), ofstream::out);
  if (!out_file_.is_open()) {
    cerr << "Cannot open input file: " << out_file_name_ << endl;
    return -1;
  }

  /* just a convenient serial interface */
  arduino_interface arduino;
  usleep(2*wait_a_sec);
  bool test_finished = false;
  unsigned int number_of_samples = 10;
  json msgJson;
  msgJson["Event"] = "Command";
  msgJson["StartCommand"] = 'S';
  msgJson["SNo"] = number_of_samples;
  msgJson["Type"] = "Ramp";
  std::string s_out = msgJson.dump();
  arduino.send_string(s_out);
  cout<<"outgoing: " << s_out <<endl;


  /* Setup scale */
  USBScale myscale;
  if(myscale.open_scale_device() == -1){
    cerr << "Cannot Open Scale Device" << std::endl;
    return -1;
  }
  while(!test_finished){

    usleep(wait_a_sec);
    string incomingString;
    //auto measurement = myscale.get_measurement();
    int measurement = 1;
    if(arduino.receive_string(incomingString) && incomingString !="") {
      cout<<"incoming: " << incomingString <<endl;
      auto msgJsonIncoming = json::parse(incomingString);
      /* Log Data */


      out_file_ << msgJsonIncoming["SampleNo"] << "\t";
      out_file_ << msgJsonIncoming["PWM"] << "\t";
      out_file_ << msgJsonIncoming["Current"] << "\t";
      out_file_ << measurement << "\n";
      test_finished = msgJsonIncoming["TestFinished"];

      cout << msgJsonIncoming["SampleNo"] << "\t";
      cout << msgJsonIncoming["PWM"] << "\t";
      cout << msgJsonIncoming["Current"] << "\t";
      cout << measurement << "\t";
      cout << msgJsonIncoming["TestFinished"] << "\n";

    }
    /* Send a heartbeat */
  }
  // close files
  if (out_file_.is_open()) {
    out_file_.close();
  }

  return 0;

}
