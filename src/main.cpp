/**
 * Blink
 *
 * Turns on an LED on for one second,
 * then off for one second, repeatedly.
 */
#include "Arduino.h"
#include <string>
#include "wifi.h"
#include "hx711.h"
#include "PmSens.h"
#include "cat.h"
#include "bm.h"
#include <cmath>
#include <EEPROM.h>
#include <vector>
using namespace std;

#define MAX_MOVEMENT_MINS 5

wifi weefee { "YOUR_WIFI_NAME", "YOUR_WIFI_PASSWORD" };
WiFiServer webserver(8080);
HX711 cell { 4, 5 };
// seed pm2.5 sensor
PmSens pmSensor;


const int NUM_CATS = 2;
cat bee { "Bee", 13 };
cat mrcat { "Mr. Cat", 11 };
cat cats[NUM_CATS] = {bee,mrcat};
float catWeightRange = 0.5;
int currentCat;

// TODO Save/load leaderboards from EEPROM
const int MAX_LEADERBOARD_SIZE = 10;
vector<BM> heaviestLeaderBoard;
vector<BM> stinkiestLeaderBoard;

vector<int> pmValues;

int lastMinute = 0;
int lastSecond = 0;
bool catPresent = false;
bool waitBeforeReset = false;

BM currentBm;
const int MAX_BM_SIZE = 2;


void setup() {
  Serial.begin(57600); delay(10);
  Serial.println();
  Serial.println("Starting...");
  // Init Scale
  // NOTE: Trying to init the scale after wifi causes it to fail (╯°□°)╯︵ ┻━┻
  float scale;
  EEPROM.get(0, scale);
  cell.setup();
  cell.setScaleValue(scale);

  // init pmSensor
  pmSensor.init();

  Serial.println(weefee.repr().c_str());
  weefee.setup();
  webserver.begin();


}


// Maths

// round to the nearest 0.05
float myround(float x){
  return round(0.05 * round(x/0.05) * 100)/100;
}

float average(std::vector<float> v){
  if(v.empty()){
    return 0;
  }
  float avg = v.at(0);
  for ( int i = 1;i < v.size(); i ++){
    avg += v.at(i);
  }
  return avg / v.size();
}

float average(std::vector<int> v){
  if(v.empty()){
    return 0;
  }
  float avg = v.at(0);
  for ( int i = 1;i < v.size(); i ++){
    avg += v.at(i);
  }
  return avg / v.size();
}

float peak(std::vector<float> v){
  if(v.empty()){
    return 0;
  }
  float largest = 0;
  for ( int i = 0;i < v.size(); i ++){
    if( v.at(i) > largest){
      largest = v.at(i);
    }
  }
  return largest;
}

// end maths

// TODO LCD screen setup
// TODO BUTTON Interrupts
// TODO Create a bowel_movement class

bool isCatPresent(){
  return cell.getData() > 5;
}

bool detectCat(){
  // get weight data
  float weight = cell.getData();

  for(int i = 0; i < NUM_CATS; i++ ){
    // current weight is within $catWeightRange of the cat
    Serial.println(cats[i].weight-catWeightRange);
    Serial.println(weight);
    Serial.println(cats[i].weight+catWeightRange);
    if( weight > (cats[i].weight - catWeightRange) && weight < (cats[i].weight + catWeightRange)){
      currentCat = i;
      return true;
    }
  }
  return false;
}

bool sortHeaviestLeaderboard(BM bm1, BM bm2){
  return bm1.weight > bm2.weight;
}

bool sortStinkiestLeaderboard(BM bm1, BM bm2){
  return bm1.peakPM > bm2.peakPM;
}

void updateLeaderBoards(BM bowel_movement){
  // leaderboard is full, is this bm bigger than our last entry?
  if(heaviestLeaderBoard.size() == MAX_LEADERBOARD_SIZE && bowel_movement.weight > heaviestLeaderBoard.back().weight){
    heaviestLeaderBoard.at(MAX_LEADERBOARD_SIZE - 1) = bowel_movement;
    sort(heaviestLeaderBoard.begin(),heaviestLeaderBoard.end(), sortHeaviestLeaderboard);
  // leaderboard is smaller than our max, automatically add it.
  } else if (heaviestLeaderBoard.size() < MAX_LEADERBOARD_SIZE ){
    heaviestLeaderBoard.push_back(bowel_movement);
    sort(heaviestLeaderBoard.begin(),heaviestLeaderBoard.end(), sortHeaviestLeaderboard);
  }

  // repeat logic for smell, TODO DRY
  // leaderboard is full, is this bm bigger than our last entry?
  if(stinkiestLeaderBoard.size() == MAX_LEADERBOARD_SIZE && bowel_movement.weight > stinkiestLeaderBoard.back().weight){
    stinkiestLeaderBoard.at(MAX_LEADERBOARD_SIZE - 1) = bowel_movement;
    sort(stinkiestLeaderBoard.begin(),stinkiestLeaderBoard.end(), sortStinkiestLeaderboard);
  // leaderboard is smaller than our max, automatically add it.
  } else if (stinkiestLeaderBoard.size() < MAX_LEADERBOARD_SIZE ){
    stinkiestLeaderBoard.push_back(bowel_movement);
    sort(stinkiestLeaderBoard.begin(),stinkiestLeaderBoard.end(), sortStinkiestLeaderboard);
  }
}

void sensor_loop(){
  cell.update();
  pmSensor.update();

  // cat was not present but now is
  if(isCatPresent() && !catPresent){
    if(detectCat()){
      catPresent = true;
      currentBm = BM{currentCat};
      // create bowel_movement class
    }
    // cat was present and no longer is
  } else if (!isCatPresent() && catPresent && !waitBeforeReset){
    waitBeforeReset = true;
  } else if (waitBeforeReset) {
    if(cell.getData() < MAX_BM_SIZE){
      // update and register bowel movement
      currentBm.weight = myround(cell.getData());
      currentBm.peakPM = pmSensor.getValue();
      currentBm.averagePm = average(pmValues);
      updateLeaderBoards(currentBm);

      waitBeforeReset = false;
      catPresent = false;

      // reset tare to account for added weight
      cell.tareNoDelay();

      // clear sensor vectors
      pmValues.clear();
    }
    // cat is present, and still is
  } else if (catPresent){
    // increment sensor vectors
    pmValues.push_back(pmSensor.getValue());
  }
}


// send 200 ok
void send_200(WiFiClient client){
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Connection: close");
  client.println();
}

// send 404 not found
void send_404(WiFiClient client){
  client.println("HTTP/1.1 404 Not Found");
  client.println("Content-Type: text/html");
  client.println("Connection: close");
  client.println();
}

// Send small responses
void send_response(WiFiClient client, int res_code, String res_body = ""){
  switch (res_code){
    case 200:
      send_200(client);
      client.println("<html>");
      client.print(res_body);
      client.println("</html>");
      break;
    case 404:
      send_404(client);
      break;
  }
}

// sends the CSS!
void sendCSS(WiFiClient client){
  const char *css = "<style>"
  "table {border-collapse: collapse; padding-bottom: 10px;}"
  "tr:nth-child(odd) {background-color: #f2f2f2;}"
"tr:hover {background-color: #ddd;}"
"</style>";

  client.println(css);
}

// leaderboard display
void sendLeaderBoard(String name,WiFiClient client, vector<BM> leaderboard){
  client.print("<div><table><th colspan='4'>");
  client.print(name);
  client.print("</th>");
  client.print("<tr><th>Rank</th><th>Cat</th><th>Smell</th><th>Weight(lb)</th></tr>");
  for(int i = 0; i < leaderboard.size(); i ++ ){
    BM bm = leaderboard.at(i);
    client.print("<tr>");
    client.print("<td>");
    client.print(i+1);
    client.print("</td><td>");
    client.print(cats[bm.catIndex].name.c_str());
    client.print("</td><td>");
    client.print(String(bm.peakPM));
    client.print("</td><td>");
    client.print(String(bm.weight));
    client.print("</td></tr>");
  }
  client.println("</table></div>");
}


// handle HTTP Request (badly)
void handle_request(WiFiClient client){
  String request = "";
  boolean currentLineIsBlank = true;
  while (client.connected()) {
    if (client.available()) {
      char c = client.read();
      request += c;
      if (c == '\n' && currentLineIsBlank) {
        break;
      }
      if (c == '\n') {
        // you're starting a new line
        currentLineIsBlank = true;
      } else if (c != '\r') {
        // you've gotten a character on the current line
        currentLineIsBlank = false;
      }
    }
  }
  int get_pos = request.indexOf("GET ");
  int http_pos = request.indexOf(" HTTP");
  if (get_pos != -1 && http_pos !=-1) {
    String request_path = request.substring(4,http_pos);
    Serial.println(request_path.c_str());
    if(request_path == "/"){
      send_200(client);
      client.println("<html>");
      sendCSS(client);
      sendLeaderBoard("Biggest Bowel Movements",client,heaviestLeaderBoard);
      sendLeaderBoard("Smelliest Bowel Movements",client,stinkiestLeaderBoard);
      client.println("</html>");
      // send_response(client,200);
    } else if (request_path == "/weight"){
      send_response(client,200, String(cell.getData()));
    }else if (request_path == "/pm"){
      send_response(client,200, String(pmSensor.getValue()));
    }
    else {
      send_response(client,404);
    }
  }
  client.stop();
  Serial.write(request.c_str());
}

// webserver loop
void webserver_loop(){
  WiFiClient client = webserver.available();
  if(client){
    handle_request(client);
  }
}


void loop() {
  Serial.println(String(pmSensor.getValue()));
  sensor_loop();
  cell.print();
  webserver_loop();
  // weefee.printWifiStatus();

  // TODO sensor_loop 1/s , webserver_loop all the time
  delay(1000);
}
