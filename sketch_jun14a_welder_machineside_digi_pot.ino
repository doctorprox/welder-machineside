/**
 * A BLE client example that is rich in capabilities.
 * There is a lot new capabilities implemented.
 * author unknown
 * updated by chegewara
 */

#include "Arduino.h"
#include "GeneralUtils.h"
#include "BLEDevice.h"
#include "BLEScan.h"


#include "esp_system.h"

const int button = 0;         //gpio to use to trigger delay
const int wdtTimeout = 3000;  //time in ms to trigger the watchdog
hw_timer_t *timer = NULL;

static int8_t pot_current = 0x1F;



 // The remote service we wish to connect to.
static BLEUUID serviceUUID("c2794909-7503-4da2-81cf-89fce41b13bf");
// The characteristic of the remote service we are interested in.
static BLEUUID    charUUID("665238f7-3cd4-4f18-9fe8-786548a68fdc");

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEAdvertisedDevice* myDevice;

static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
  Serial.print("Notify callback for characteristic ");
  Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
  Serial.print(" of data length ");
  Serial.println(length);
  Serial.print("data: ");
  Serial.println((char*)pData);
}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    Serial.println("onDisconnect");
      doConnect = true;
      doScan = true;
  }
};

bool connectToServer() {
  Serial.print("Forming a connection to ");
  Serial.println(myDevice->getAddress().toString().c_str());

  BLEClient*  pClient = BLEDevice::createClient();
  Serial.println(" - Created client");

  pClient->setClientCallbacks(new MyClientCallback());

  // Connect to the remove BLE Server.
  pClient->connect(myDevice);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
  Serial.println(" - Connected to server");

  // Obtain a reference to the service we are after in the remote BLE server.
  BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == nullptr) {
    Serial.print("Failed to find our service UUID: ");
    Serial.println(serviceUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Found our service");


  // Obtain a reference to the characteristic in the service of the remote BLE server.
  pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
  if (pRemoteCharacteristic == nullptr) {
    Serial.print("Failed to find our characteristic UUID: ");
    Serial.println(charUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Found our characteristic");

  // Read the value of the characteristic.
  if (pRemoteCharacteristic->canRead()) {
    uint32_t value1 = pRemoteCharacteristic->readUInt32();
    Serial.print("The characteristic value was: ");
    Serial.println(value1);
  }

  //if (pRemoteCharacteristic->canNotify())
  //  pRemoteCharacteristic->registerForNotify(notifyCallback);

  connected = true;
}
/**
 * Scan for BLE servers and find the first one that advertises the service we are looking for.
 */
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  /**
    * Called for each advertising BLE server.
    */
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("BLE Advertised Device found: ");
    Serial.println(advertisedDevice.toString().c_str());

    // We have found a device, let us now see if it contains the service we are looking for.
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {
      Serial.print("Found service.");
      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = true;

    } // Found our server
  } // onResult
}; // MyAdvertisedDeviceCallbacks

void IRAM_ATTR resetModule() {
  ets_printf("reboot\n");
  esp_restart();
}

void setup() {
	pinMode(16, OUTPUT);
	pinMode(17, OUTPUT);
	resetzero();
	delay(3000);
	resetzero();
	delay(3000);


	digitalWrite(16, HIGH); //ud high
	delayMicroseconds(5);		
	digitalWrite(17, LOW); //CS Low
	delayMicroseconds(5);
	for (uint8_t i = 0; i < 62; i++) {
		digitalWrite(16, LOW); //ud low
		delayMicroseconds(5);
		digitalWrite(16, HIGH); //ud high
		delayMicroseconds(5);
		
		delay(1000);
	}
	digitalWrite(17, HIGH); //CS high
	delayMicroseconds(1);

	for (uint8_t j = 0; j < 62; j++) {
		setpot(j);
		delay(100);
	}

	for (uint8_t j = 62; j > 0; j--) {
		setpot(j);
		delay(100);
	}

  timer = timerBegin(0, 80, true);                  //timer 0, div 80
  timerAttachInterrupt(timer, &resetModule, true);  //attach callback
  timerAlarmWrite(timer, wdtTimeout * 1000, false); //set time in us
  timerAlarmEnable(timer);  
  Serial.begin(115200);
  Serial.println("Starting Arduino BLE Client application...");
  BLEDevice::init("");
  
  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 5 seconds.
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);
  

} // End of setup.

void resetzero() {
	for (uint8_t i = 0; i < 62; i++) {
		digitalWrite(16, LOW); //ud high
		delayMicroseconds(1);
		digitalWrite(17, LOW); //CS Low
		delayMicroseconds(4);
		digitalWrite(16, HIGH); //ud low
		delayMicroseconds(1);
		digitalWrite(16, LOW); //ud high
		delayMicroseconds(1);
		digitalWrite(17, HIGH); //CS high
		delayMicroseconds(5);
	}
	pot_current = 0;
}

void setpot(uint8_t potval) {
	while(pot_current != potval) {
		if (pot_current > potval) {
			digitalWrite(16, LOW); //ud high
			delayMicroseconds(1);
			digitalWrite(17, LOW); //CS Low
			delayMicroseconds(4);
			digitalWrite(16, HIGH); //ud low
			delayMicroseconds(1);
			digitalWrite(16, LOW); //ud high
			delayMicroseconds(1);
			digitalWrite(17, HIGH); //CS high
			delayMicroseconds(1);

			pot_current--;
		}
		else {
			digitalWrite(16, HIGH); //ud high
			delayMicroseconds(1);
			digitalWrite(17, LOW); //CS Low
			delayMicroseconds(4);
			digitalWrite(16, LOW); //ud low
			delayMicroseconds(1);
			digitalWrite(16, HIGH); //ud high
			delayMicroseconds(1);
			digitalWrite(17, HIGH); //CS high
			delayMicroseconds(1);
			pot_current++;
		}
	}
}
// This is the Arduino main loop function.
void loop() {

  timerWrite(timer, 0); //reset timer (feed watchdog)
  
  // If the flag "doConnect" is true then we have scanned for and found the desired
  // BLE Server with which we wish to connect.  Now we connect to it.  Once we are 
  // connected we set the connected flag to be true.
  if (doConnect == true) {
    if (connectToServer()) {
      Serial.println("We are now connected to the BLE Server.");
    }
    else {
      Serial.println("We have failed to connect to the server; there is nothin more we will do.");
    }
    doConnect = false;
  }

  // If we are connected to a peer BLE Server, update the characteristic each time we are reached
  // with the current time since boot.
  if (connected) {
    uint32_t value1 = pRemoteCharacteristic->readUInt32();
    Serial.print("The characteristic value was: ");
	if (value1 > 62) {
		value1 = 62;
	}
    Serial.println(value1);
	setpot((uint8_t)value1);
    //std::string value = pRemoteCharacteristic->readValue();

    //String newValue = "Time since boot: " + String(millis() / 1000);
    //Serial.println(value.c_str());

    // Set the characteristic's value to be the array of bytes that is actually a string.
    //pRemoteCharacteristic->writeValue(newValue.c_str(), newValue.length());
  }
  else if (doScan) {
    BLEDevice::getScan()->start(5);  // this is just eample to start scan after disconnect, most likely there is better way to do it in arduino
  }

  delay(100); // Delay a second between loops.

} // End of loop
