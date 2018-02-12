#include <stdio.h>
#include <iostream>
#include <sstream>
#include <string>
#include <curl/curl.h>
#include <pthread.h>
#include <thread>
#include <chrono>
#include <vector>
#include <ncurses.h>
#include <unistd.h>

using namespace std;

const int INPUT_LINE_X = 23;
const int MAX_TEXT_WIDTH = 85;
const int REFRESH_RATE = 3;

CURL *curl;
CURLcode res;
string response_get_messages;
string response_new_message;
vector<char> message_buffer;
int cursorX = INPUT_LINE_X;
int cursorY = 3;
bool should_run_thread = true;
int thread_counter = 0;
int k = 0;
const string base = "http://159.65.35.162:8000";
//const string base = "http://localhost:8000";
string username; // jake
string password; // KCQ

string encrypt(string str, string key)
{
  string output = str;
  for (int i = 0; i < str.size(); i++){
    output[i] ^= key[i % key.size()];
    output[i]++;
  }
  return output;
}

string decrypt(string str, string key)
{
  string output = str;
  for (int i = 0; i < str.size(); i++){
    output[i]--;
    output[i] ^= key[i % key.size()];
  }
  return output;
}

size_t write_to_string(void *ptr, size_t size, size_t count, void *stream) {
  ((string*)stream)->append((char*)ptr, 0, size*count);
  return size*count;
}

bool send_request(string location, string params, string *response)
{

  curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);

  curl_easy_setopt(curl, CURLOPT_URL, (base + location + params).c_str());
  res = curl_easy_perform(curl);

  if (res != CURLE_OK) {
    mvprintw(INPUT_LINE_X + 1, 0, "ERROR: curl_easy_perform() failed: %s", curl_easy_strerror(res));
    refresh();
    //termin();
  } else {
    mvprintw(INPUT_LINE_X + 1, 0, "Successfully connected to server.                                 ", curl_easy_strerror(res));
  }

  return true;

}

void init_curl()
{
  curl = curl_easy_init();

  if (curl == NULL) {
    printf("Could not initiate CURL. Exiting.\n");
    exit(0);
  }

  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_string);

}

void init_ncurses()
{
  initscr();
  raw();
  keypad(stdscr,TRUE);
  noecho();
}

void init_screen()
{
  move(cursorX, cursorY);
  mvprintw(INPUT_LINE_X, 0, " > ");
}

void *DisplayScreen(void *threadid)
{
  this_thread::sleep_for(chrono::milliseconds(50));
  while (should_run_thread) {
    thread_counter %= REFRESH_RATE;
    if (thread_counter == 0) {
      k++;
      send_request("/get_messages", "", &response_get_messages);
      mvprintw(0, 0, "username: %s", username.c_str());
      mvprintw(0, 30, "updates: %d", k);
      for (int i = 0; i < MAX_TEXT_WIDTH; i++) mvprintw(1, i, " ");

      istringstream ss (response_get_messages);
      string line;
      for (int i = 2; getline(ss, line); i++) {

        //mvprintw(INPUT_LINE_X + 2 + i, 0, "%s", line.c_str());
        // separate into parts divided by "|"
        vector<char> prefix_buffer;
        vector<char> msg_buffer;
        int j = 0;
        for (; j < line.size() && line[j] != '|'; j++) prefix_buffer.push_back(line[j]);
        prefix_buffer.push_back('\0');
        j++;
        for (; j < line.size(); j++) msg_buffer.push_back(line[j]);
        msg_buffer.push_back('\0');
        char *prefix_str = &prefix_buffer[0];

        vector<char> character_codes;
        istringstream iss (&msg_buffer[0]);

        for (int code; iss >> code;) character_codes.push_back(code);
        character_codes.push_back('\0');
        string msg (&character_codes[0]);

        string decrypted = decrypt(msg, password);
        for (int kk = 0; kk < MAX_TEXT_WIDTH; kk++) mvprintw(i, kk, " ");
        mvprintw(i, 0, "%s: %s", prefix_str, decrypted.c_str());
      }
      response_get_messages = "";
      move(cursorX, cursorY);
      refresh();

    }
    thread_counter++;

    this_thread::sleep_for(chrono::milliseconds(100));
  }
  pthread_exit(NULL);
}

string buffer_to_string()
{
  char tmp[message_buffer.size()+1];
  for (int i = 0; i < message_buffer.size(); i++) {
    tmp[i] = message_buffer[i];
    if (tmp[i] == 0x0A) tmp[i] = '?';
  }
  tmp[message_buffer.size()] = '\0';
  string str (tmp);
  return str;
}

void password_username_sequence()
{
  cout << "Username: ";
  cin >> username;
  //cout << "Password: ";
  char *tmp_password = getpass("Password: ");
  password = string (tmp_password);
}

int main()
{
  password_username_sequence();

  init_curl();
  init_ncurses();
  init_screen();
  pthread_t thread;
  int rc = pthread_create(&thread, NULL, DisplayScreen, (void *)0);


  while (1) {
    char cur = getch();
    if (cur == 3) break;
    if (cur == 10) { // enter
      if (message_buffer.size() == 0) continue;
      string encrypted = encrypt(buffer_to_string(), password);
      char *str = curl_easy_escape(curl, encrypted.c_str(), message_buffer.size());
      string strr (str);
      send_request(
          "/new_message", "?username=" + username + "&message=" + strr,
          &response_new_message);
      if (response_new_message == string ("ok")) {
        message_buffer.clear();
        response_new_message = "";
        mvprintw(INPUT_LINE_X + 2, 0, "                                                  ");
      } else {
        mvprintw(INPUT_LINE_X + 2, 0, response_new_message.c_str());
      }

    }
    if (cur == 7) { // backspace
      if (message_buffer.size() != 0)
        message_buffer.pop_back();
    } else if (cur == 8) { // ctrl-bkspace
      if (message_buffer.size() != 0)
        message_buffer.pop_back();
    } else if (cur == 22) {
      string fuck_you ("fuck you");
      for (char l: fuck_you) message_buffer.push_back(l);
    } else if (32 <= cur && cur <= 126) {
      if (message_buffer.size() < MAX_TEXT_WIDTH)
        message_buffer.push_back(cur);
    }

    // print the current stored message
    int len = message_buffer.size();
    char message_built[MAX_TEXT_WIDTH+1];
    int i = 0;
    for (; i < len; i++) message_built[i] = message_buffer[i];
    for (; i < MAX_TEXT_WIDTH; i++) message_built[i] = ' ';
    message_built[MAX_TEXT_WIDTH] = '\0';
    mvprintw(INPUT_LINE_X, 0, " > %s", message_built);
    cursorY = len + 3;
    move(cursorX, cursorY);
    refresh();

  }

  should_run_thread = false;

  void *ret;
  pthread_join(thread, &ret);

  curl_easy_cleanup(curl);

  endwin();

  //send_request("/get_messages", "?username=" + message);
  //printf("response: \n%s======", response.c_str());
  return 0;
}
