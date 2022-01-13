#include <avr/io.h>
#include <avr/interrupt.h>
#include <LiquidCrystal.h>
LiquidCrystal lcd(7, 6, 5, 4, 3, 2);

#define INF 1294967295   //big enough and does not overflow at addition on int
#define START_BUT 19 //BUT0 port 22
#define PLAYER_0 20 //BUT1 port 23
#define PLAYER_1 21 //BUT2 port 25
#define BUZZ_DURATION 200 //how much time the beep is on
#define NO_ROUNDS 3 //the number of rounds
#define NO_PLAYERS 2 //the number of players (same as number of buttons others than start one
#define BUZZER_PIN 10
#define RESET_BUT 18
#define MAX_GAME_DURATION 4000
#define led_pin 8

volatile long game_duration;

volatile bool foul[NO_PLAYERS] = {false, false};

volatile bool start_pressed_but = false;
volatile bool player_1_pressed_but = false;
volatile bool player_2_pressed_but = false;

volatile long buzzerPeriod = 0;
volatile int buzzerDelay;

volatile bool game_ended = false;

volatile long timer = 0;
volatile long starting_time;
volatile long reaction[NO_ROUNDS][NO_PLAYERS] = {{INF, INF},{INF,INF},{INF,INF}};//store the reaction in every round for every player
volatile int wins[NO_PLAYERS];//wins[i] - number of rounds won by player i
volatile float avg[NO_PLAYERS];//avg[i] - the average response time of player i
volatile bool roundStarted;//true if a round is in progress
volatile int roundNr = 0; //current round number
volatile bool not_pressed[NO_PLAYERS];//not_pressed[i] == 1 if player i has not pressed his button in current round
volatile bool game_started = false;

volatile bool timeout = false;

void setup() {
  Serial.begin(9600);
  pinMode(PLAYER_0, INPUT);
  pinMode(PLAYER_1, INPUT);
  pinMode(START_BUT, INPUT);
  pinMode(RESET_BUT, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);



  attachInterrupt(digitalPinToInterrupt(PLAYER_0), player_0_pressed, RISING);
  attachInterrupt(digitalPinToInterrupt(PLAYER_1), player_1_pressed, RISING);
  attachInterrupt(digitalPinToInterrupt(START_BUT), start_game, RISING);
  attachInterrupt(digitalPinToInterrupt(RESET_BUT), reset_button_state, RISING);
  lcd.begin(16, 2); 
//change to timer one
  TCCR0A=(1<<WGM01);    //Set the CTC mode   
  OCR0A=0xF9; //Value for ORC0A for 1ms 
  
  TIMSK0|=(1<<OCIE0A);   //Set the interrupt request
  sei(); //Enable interrupt
  
  TCCR0B|=(1<<CS01);    //Set the prescale 1/64 clock
  TCCR0B|=(1<<CS00);

}


ISR(TIMER0_COMPA_vect){    //This is the interrupt request
  timer++;

  if(buzzerDelay > 0)
  {
    buzzerDelay--;

  }
  else if (buzzerDelay == 0)
  {
    analogWrite(BUZZER_PIN, 150);
    analogWrite(led_pin, 150);
    buzzerPeriod = 250;
    buzzerDelay--;
    game_started = true;
    timer = 0;
    starting_time = timer;
  }

  else if(buzzerPeriod > 0)
  {
        buzzerPeriod--;
  }
   else if (buzzerPeriod == 0){
      buzzerPeriod--;
      
      analogWrite(BUZZER_PIN, 0);
      analogWrite(led_pin, 0);
    }
   else if((timer - starting_time == MAX_GAME_DURATION) && !game_ended)
  {
    timeout = true;
  }
  

}

void reset_button_state()
{

  start_pressed_but = false;
  player_1_pressed_but = false;
  player_2_pressed_but = false;
  roundNr++;

}

void start_game(){
  if(!start_pressed_but)
  {
  start_pressed_but = true;
    game_started = false;

    Serial.println(reaction[roundNr][0]);
  
    Serial.println(reaction[roundNr][1]);
  lcd.setCursor(0,0);
  game_ended = false;
  foul[0] = false;
  foul[1] = false;
  timeout = false;
  timer = 0;
  buzzerDelay = random(3000,7000);
  lcd.clear();
  }
}

void printRoundResults(int nr);


void loop() {
  
  if(((reaction[roundNr][0] != INF || foul[0]) && (reaction[roundNr][1] != INF || foul[1]) && !game_ended && game_started) || timeout)
    {
      Serial.println(game_ended);
  Serial.println(game_started);

      game_ended = true;
      printRoundResults(roundNr);
      //game ended, print results
    }
    
}

void player_0_pressed(){   
  if(!player_1_pressed_but)
{
  player_1_pressed_but = true;
  if(game_started){
    reaction[roundNr][0] = timer - starting_time;
  }
   else
  {
      foul[0] = true;
  }
}

}

void player_1_pressed() {

   if(!player_2_pressed_but)
   { 
      player_2_pressed_but = true;
      if(game_started){
        reaction[roundNr][1] = timer - starting_time;
      }
      else{
        foul[1] = true;
      }
    }
  }

/** Prints on the right side of the lcd who is the winner.
* Smaller is better.
*/
void printOutcome()
{
  if(reaction[roundNr][0] == reaction[roundNr][1])
  {
    lcd.setCursor(12, 0);
    lcd.print("DRAW");
    lcd.setCursor(12, 1);
    lcd.print("DRAW");
  } else
  {
    if(reaction[roundNr][0] < reaction[roundNr][1]) lcd.setCursor(13, 0); //v0 is winner
    else lcd.setCursor(13, 1); //v1 is winner
    lcd.print("Win");
  }
}

void printRoundResults(int nr)
{
  //display reaction times of players
  //if a player did foul, print FOUL instead of INF

  lcd.clear();
  for(int i = 0; i < NO_PLAYERS; ++i)
  {
    Serial.println(timeout);
    Serial.println(reaction[nr][i] );
    if(reaction[nr][i] != INF)
    {
      lcd.setCursor(0, i);
      lcd.print("P");
      lcd.print(i+1);
      lcd.print("   ");
      lcd.print(reaction[nr][i]);
      if(reaction[nr][i] == min(reaction[nr][0], reaction[nr][1]))
      {
        lcd.setCursor(13, i);
        lcd.print("Win");
      }
      else
      {
        long res = abs(reaction[nr][0] - reaction[nr][1]);
        int cursorPos = 15;
        while(res) 
        {
          lcd.setCursor(cursorPos, i);
          lcd.print(res % 10);
          res /= 10;
          cursorPos--;
        }
        lcd.setCursor(cursorPos, i);
        lcd.print("+");
      }
    }
    else if(foul[i]){
      lcd.print("P");
      lcd.print(i+1);
      lcd.print("   FOUL");
      
    }
    else
    {
      lcd.print("P");
      lcd.print(i+1);
      lcd.print("   DNF");
    }
  //print "Win" or "Draw"
  //printOutcome();
  //print the time difference between winner and loser.
  //Print it on the row with the loser.
  //if one of the players has made a fault do not print the time difference
 // if(reaction[nr][0] != INF && reaction[nr][1] != INF) {
  //  if(reaction[nr][0] > reaction[nr][1]) lcd.setCursor(10, 0);
  //  else lcd.setCursor(10, 1);
    //lcd.printf("%+6d", abs(reaction[nr][0] - reaction[nr][1]));
  }
}
