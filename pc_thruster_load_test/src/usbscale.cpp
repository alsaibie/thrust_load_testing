/****
    usbscale

    CPP utility to read weight from a USB scale.
    Copyright (C) 2011 Eric Jiang

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

 - Modified by Ali AlSaibie from C to CPP and to suit <alsaibie@gatech.edu>

*/

#include "../include/usbscale.h"


USBScale::USBScale(): weigh_count(WEIGH_COUNT-1),
                      scale_result(-1)
{


}

int USBScale::open_scale_device() {

  //
  // We first try to init libusb.
  //
  r = libusb_init(NULL);
  //
  // If `libusb_init` errored, then we quit immediately.
  //
  if (r < 0)
    return r;

  #ifdef DEBUG
    libusb_set_debug(NULL, 3);
  #endif

  //
  // Next, we try to get a list of USB devices on this computer.
  cnt = libusb_get_device_list(NULL, &devs);
  if (cnt < 0)
    return (int) cnt;

  //
  // Once we have the list, we use **find_scale** to loop through and match
  // every device against the scales.h list. **find_scale** will return the
  // first device that matches, or 0 if none of them matched.
  //
  dev = find_scale(devs);
  if(dev == 0) {
    std::cerr<<"No USB scale found on this computer."<<std::endl;
    return -1;
  }

  //
  // Once we have a pointer to the USB scale in question, we open it.
  //
  r = libusb_open(dev, &handle);
  //
  // Note that this requires that we have permission to access this device.
  // If you get the "permission denied" error, check your udev rules.
  //
  if(r < 0) {
    if(r == LIBUSB_ERROR_ACCESS) {
      std::cerr<<"Permission denied to scale."<<std::endl;
    }
    else if(r == LIBUSB_ERROR_NO_DEVICE) {
      std::cerr<<"Scale has been disconnected"<<std::endl;
    }
    return -1;
  }
  //
  // On Linux, we typically need to detach the kernel driver so that we can
  // handle this USB device. We are a userspace tool, after all.
  //
#ifdef __linux__
  libusb_detach_kernel_driver(handle, 0);
#endif
  //
  // Finally, we can claim the interface to this device and begin I/O.
  //
  libusb_claim_interface(handle, 0);

  //
  // For some reason, we get old data the first time, so let's just get that
  // out of the way now. It can't hurt to grab another packet from the scale.
  //
  r = libusb_interrupt_transfer(
          handle,
          //bmRequestType => direction: in, type: class,
          //    recipient: interface
          LIBUSB_ENDPOINT_IN | //LIBUSB_REQUEST_TYPE_CLASS |
          LIBUSB_RECIPIENT_INTERFACE,
          data,
          WEIGH_REPORT_SIZE, // length of data
          &len,
          10000 //timeout => 10 sec
  );

  return r;


};

double USBScale::get_measurement() {

  bool new_measurement = false;
  while (!new_measurement) {
    //
    // A `libusb_interrupt_transfer` of 6 bytes from the scale is the
    // typical scale data packet, and the usage is laid out in *HID Point
    // of Sale Usage Tables*, version 1.02.
    //
    r = libusb_interrupt_transfer(
            handle,
            //bmRequestType => direction: in, type: class,
            //    recipient: interface
            get_first_endpoint_address(dev),
            data,
            WEIGH_REPORT_SIZE, // length of data
            &len,
            200 //timeout => 0.2 sec
    );
    //
    // If the data transfer succeeded, then we pass along the data we
    // received tot **print_scale_data**.
    //
    if (r == 0) {
      #ifdef DEBUG
      //        int i;
      //                for(i = 0; i < WEIGH_REPORT_SIZE; i++) {
      //                    printf("%x\n", data[i]);
      //                }
      #endif

      if (weigh_count < 1) {

        //
        // We keep around `lastStatus` so that we're not constantly printing the
        // same status message while waiting for a weighing. If the status hasn't
        // changed from last time, **print_scale_data** prints nothing.
        //
        static uint8_t lastStatus = 0;

        //
        // Gently rip apart the scale's data packet according to *HID Point of Sale
        // Usage Tables*.
        //
        uint8_t report = data[0];
        uint8_t status = data[1];
        uint8_t unit   = data[2];
        // Accoring to the docs, scaling applied to the data as a base ten exponent
        int8_t  expt   = data[3];
        // convert to machine order at all times
        double weight = (double) le16toh(data[5] << 8 | data[4]);
        // since the expt is signed, we do not need no trickery
        weight = weight * pow(10, expt);

        //
        // The scale's first byte, its "report", is always 3.
        //
        if(report != 0x03 && report != 0x04) {
          std::cerr<<"Error reading scale data"<<std::endl;
          return -1;
        }

        //
        // Switch on the status byte given by the scale. Note that we make a
        // distinction between statuses that we simply wait on, and statuses that
        // cause us to stop (`return -1`).
        //
        switch(status) {
          case 0x01:
            std::cerr<<"Scale reports Fault"<<std::endl;
            return -1;
          case 0x02:
            if(status != lastStatus)
              std::cerr<<"Scale is zero'd..."<<std::endl;
            break;
          case 0x03:
            if(status != lastStatus)
              std::cerr<<"Weighing..."<<std::endl;
            break;
            //
            // 0x04 is the only final, successful status, and it indicates that we
            // have a finalized weight ready to print. Here is where we make use of
            // the `UNITS` lookup table for unit names.
            //
          case 0x04:
//          std::cout<<weight<<" "<<UNITS[unit]<<std::endl;
            return weight;
          case 0x05:
            if(status != lastStatus)
              std::cerr<<"Scale reports Under Zero"<<std::endl;
            break;
          case 0x06:
            if(status != lastStatus)
              std::cerr<<"Scale reports Over Weight"<<std::endl;
            break;
          case 0x07:
            if(status != lastStatus)
              std::cerr<<"Scale reports Calibration Needed"<<std::endl;
            break;
          case 0x08:
            if(status != lastStatus)
              std::cerr<<"Scale reports Re-zeroing Needed!"<<std::endl;
            break;
          default:
            if(status != lastStatus)
              std::cerr<<"Unknown status code: "<<status<<std::endl;
            return -1;
        }

        lastStatus = status;
      }
      weigh_count--;
    }
    else {
      std::cerr << "Error in USB transfer" << std::endl;
      scale_result = -1;
      return  scale_result;
    }


  }

}

USBScale::~USBScale() {

  //
  // At the end, we make sure that we reattach the kernel driver that we
  // detached earlier, close the handle to the device, free the device list
  // that we retrieved, and exit libusb.
  //
  #ifdef __linux__
    libusb_attach_kernel_driver(handle, 0);
  #endif
    libusb_close(handle);
    libusb_free_device_list(devs, 1);
    libusb_exit(NULL);
}


//
// find_scale
// ----------
// 
// **find_scale** takes a `libusb_device\*\*` list and loop through it,
// matching each device's vendor and product IDs to the scales.h list. It
// return the first matching `libusb_device\*` or 0 if no matching device is
// found.
//
libusb_device * USBScale::find_scale(libusb_device **devs)
{

    int i = 0;
    libusb_device* dev = 0;

    //
    // Loop through each USB device, and for each device, loop through the
    // scales list to see if it's one of our listed scales.
    //
    while ((dev = devs[i++]) != NULL) {
        struct libusb_device_descriptor desc;
        int r = libusb_get_device_descriptor(dev, &desc);
        if (r < 0) {
          std::cerr<<"failed to get device descriptor"<<std::endl;
            return NULL;
        }
        int i;
        for (i = 0; i < NSCALES; i++) {
            if(desc.idVendor  == scales[i][0] && 
               desc.idProduct == scales[i][1]) {
                /*
                 * Debugging data about found scale
                 */
#ifdef DEBUG

              std::cerr<<"Found scale "<<desc.idVendor<<":"<<desc.idProduct<<" bus "
                       << static_cast<unsigned>(libusb_get_bus_number(dev))
                       <<" device "<< static_cast<unsigned>(libusb_get_device_address(dev))<<std::endl;
              std::cerr<<"It has descriptors:\n"
                       <<"manufc: "    << static_cast<unsigned>(desc.iManufacturer)   <<"\n"
                       <<"tprodct: "   << static_cast<unsigned>(desc.iProduct)        <<"\n"
                       <<"serial: "    << static_cast<unsigned>(desc.iSerialNumber)   <<"\n"
                       <<"class: "     << static_cast<unsigned>(desc.bDeviceClass)    <<"\n"
                       <<"subclass: "  << static_cast<unsigned>(desc.bDeviceSubClass) <<std::endl;

                    /*
                     * A char buffer to pull string descriptors in from the device
                     */
                    unsigned char string[256];
                    libusb_device_handle* hand;
                    libusb_open(dev, &hand);

                    r = libusb_get_string_descriptor_ascii(hand, desc.iManufacturer,
                            string, 256);
              std::cerr<<"Manufacturer: "<<string<<std::endl;
                    libusb_close(hand);
#endif
                    return dev;

            }
        }
    }
    return NULL;
}

uint8_t USBScale::get_first_endpoint_address(libusb_device* dev)
{
    // default value
    uint8_t endpoint_address = LIBUSB_ENDPOINT_IN | LIBUSB_RECIPIENT_INTERFACE; //| LIBUSB_RECIPIENT_ENDPOINT;

    struct libusb_config_descriptor *config;
    int r = libusb_get_config_descriptor(dev, 0, &config);
    if (r == 0) {
        // assuming we have only one endpoint
        endpoint_address = config->interface[0].altsetting[0].endpoint[0].bEndpointAddress;
        libusb_free_config_descriptor(config);
    }

    #ifdef DEBUG
    printf("bEndpointAddress 0x%02x\n", endpoint_address);
    #endif

    return endpoint_address;
}
