#include "Keyboard.h"
#include "Mouse.h"
// Pin definitions for Pro Micro

// 1열과 엔터, 스페이스는 아날로그 핀의 저항값으로 컨트롤 합니다.
#define ANALOG_PIN       A0
#define ESC_VALUE        640  // 1열 1행
#define TAB_VALUE        682  // 1열 2행
#define MODE_KEY_VALUE   735  // 1열 3행 (ModeChange)
#define BACKSPACE_VALUE  788  // 1열 4행
#define ENTER_VALUE      855  // 2열 4행
#define SPACE_VALUE      940  // 4열 4행

// Column 2 keys (except Enter)
#define COL2_R1_PIN    2  // 2열 1행
#define COL2_R2_PIN    3  // 2열 2행
#define COL2_R3_PIN    4  // 2열 3행

// Column 3 keys
#define COL3_R1_PIN    5  // 3열 1행
#define COL3_R2_PIN    6  // 3열 2행
#define COL3_R3_PIN    7  // 3열 3행

// Column 4 keys (except Space)
#define COL4_R1_PIN    8  // 4열 1행
#define COL4_R2_PIN    9  // 4열 2행
#define COL4_R3_PIN    10  // 4열 3행

// Column 5 key (except Jump and Dot)
#define JUMP_PIN       16  // 5열 1행 (Right key)
#define COL5_R2_PIN    14  // 5열 2행
#define DOT_PIN        15  // 5열 3행 (.?!)

#define LED1           19 // Mode LED 1
#define LED2           20  // Mode LED 2

#define MODE_KO       0   // 한글 모드 (10)
#define MODE_EN       1   // 영문 모드 (01)
#define MODE_NUM      2   // 숫자 모드 (00)
#define MODE_SPEC     3   // 특수문자 모드 (11)

#define MOUSE_MODE_TOUCH_PIN     A3 // 아날로그 핀에 신호에 따라 마우스 모드 전환
#define MOUSE_MODE_TOUCH_THRESHOLD  1015 // 마우스 모드 전환 임계치
#define MOUSE_MODE_HOLD_DURATION_MS  200 // 마우스 모드 최소 유지 시간

bool mouseMode = false;
unsigned long lastMouseModeActive = 0; // 마우스 모드 활성화 시각

int inputMode = MODE_KO;

unsigned long lastButtonPress = 0;

int lastPressed1;
int lastPressed2;
int lastPressed3;
int lastPressed4;
int lastPressed5;
int lastPressed6;

int prevAnalogValue = 0;

int prevCOL2_R1 = 0;
int prevCOL2_R2 = 0;
int prevCOL2_R3 = 0;
int prevCOL3_R1 = 0;
int prevCOL3_R2 = 0;
int prevCOL3_R3 = 0;
int prevCOL4_R1 = 0;
int prevCOL4_R2 = 0;
int prevCOL4_R3 = 0;
int prevJUMP = 0;
int prevCOL5_R2 = 0;
int prevDOT = 0;

// 영문 모드용 shift 전역 변수 추가
bool shiftPressed = false;

// fn 모드
bool fnPressed = false;
bool fnUsed = false;

// 처음 켜질때 한국어 셋팅을 위한 변수
bool firstTimeKorSettingRun = false;

// 핥,않,삶 의 입력시 ㄷ,ㅅ,ㅇ 이 먼저쳐지면 안되기 때문에 해당 상황을 위한 대기키 변수
int waitingKey = 0;
// 한글 모드의 각 키의 첫번째 키 보관용 변수
int pinToKorFirstKey[20];

void setup() {
  Serial.begin(9600);

  // 한글 모드의 각 키의 첫번째 키 보관
  pinToKorFirstKey[COL2_R1_PIN] = 'l';
  pinToKorFirstKey[COL2_R2_PIN] = '.';
  pinToKorFirstKey[COL2_R3_PIN] = 'm';
  pinToKorFirstKey[COL3_R1_PIN] = 'r';
  pinToKorFirstKey[COL3_R2_PIN] = 's';
  pinToKorFirstKey[COL3_R3_PIN] = 'e';
  pinToKorFirstKey[COL4_R1_PIN] = 'q';
  pinToKorFirstKey[COL4_R2_PIN] = 't';
  pinToKorFirstKey[COL4_R3_PIN] = 'w';
  pinToKorFirstKey[COL5_R2_PIN] = 'a';

  // 모든 핀 설정
  pinMode(COL2_R1_PIN, INPUT_PULLUP);
  pinMode(COL2_R2_PIN, INPUT_PULLUP);
  pinMode(COL2_R3_PIN, INPUT_PULLUP);
  
  pinMode(COL3_R1_PIN, INPUT_PULLUP);
  pinMode(COL3_R2_PIN, INPUT_PULLUP);
  pinMode(COL3_R3_PIN, INPUT_PULLUP);
  
  pinMode(COL4_R1_PIN, INPUT_PULLUP);
  pinMode(COL4_R2_PIN, INPUT_PULLUP);
  pinMode(COL4_R3_PIN, INPUT_PULLUP);
  
  pinMode(JUMP_PIN, INPUT_PULLUP);
  pinMode(COL5_R2_PIN, INPUT_PULLUP);
  pinMode(DOT_PIN, INPUT_PULLUP);

  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  
  setModeLED(inputMode);

  inputMode = MODE_KO;

  pinMode(MOUSE_MODE_TOUCH_PIN, INPUT_PULLUP);
}

void loop() {
  unsigned long now = millis();
  // 마우스 모드 처리
  int mouseModeValue = analogRead(MOUSE_MODE_TOUCH_PIN);
  // 임계치 초과 시 타이머 갱신
  if (mouseModeValue < MOUSE_MODE_TOUCH_THRESHOLD) {
    lastMouseModeActive = now;
  }
  // 최근 일정 시간 내에 임계치 초과 기록이 마우스 모드 있으면 유지
  bool newMouseMode = (now - lastMouseModeActive <= MOUSE_MODE_HOLD_DURATION_MS);

  // 값이 0인건 무시하고, 모드가 바뀌면 업데이트
  if (newMouseMode != mouseMode) {
    mouseMode = newMouseMode;
    Serial.println(mouseModeValue);
    Serial.print("마우스 모드: ");
    Serial.println(mouseMode);
  }

  // 처음 시작후 3초가 지나고 첫 언어 동기화
  if (!firstTimeKorSettingRun && millis() > 3000) {
    setLang();
    firstTimeKorSettingRun = true;
  }
  // 마지막 입력후 1초 이상 1.1초 이하면 모드에 맞는 언어 재설정
  int lastPressedTime = millis() - lastButtonPress;
  if (lastPressed1 != 0 && lastPressedTime > 1000 && lastPressedTime < 1100) {
    setLang();
  }
  // 1초이상 입력이 없으면 지난 입력 모두 초기화
  if (lastPressed1 != 0 && lastPressedTime > 2000) {
    resetAndRight();
  }

  // 기본 컨트롤 키 상태 확인
  int analogValue = analogRead(ANALOG_PIN);
  
  // 2열 키 상태 확인
  int col2R1State = digitalRead(COL2_R1_PIN);
  int col2R2State = digitalRead(COL2_R2_PIN);
  int col2R3State = digitalRead(COL2_R3_PIN);
  
  // 3열 키 상태 확인
  int col3R1State = digitalRead(COL3_R1_PIN);
  int col3R2State = digitalRead(COL3_R2_PIN);
  int col3R3State = digitalRead(COL3_R3_PIN);
  
  // 4열 키 상태 확인
  int col4R1State = digitalRead(COL4_R1_PIN);
  int col4R2State = digitalRead(COL4_R2_PIN);
  int col4R3State = digitalRead(COL4_R3_PIN);
  
  // 5열 키 상태 확인
  int jumpState = digitalRead(JUMP_PIN);
  int col5R2State = digitalRead(COL5_R2_PIN);
  int dotState = digitalRead(DOT_PIN);

  // 마우스 모드 처리
  if (mouseMode) {
    if (analogValue < 10) { Mouse.release(MOUSE_LEFT); Mouse.release(MOUSE_RIGHT); }
    else if (analogValue <= ESC_VALUE) { 
      if (prevAnalogValue < 10) {
        Serial.print("좌클릭: ");
        Serial.println(analogValue);
        Mouse.press(MOUSE_LEFT);
      }
    } else if (analogValue <= TAB_VALUE) { 
      Serial.println("위로 스크롤");
      Mouse.move(0, 0, -1);
    } else if (analogValue <= MODE_KEY_VALUE && prevAnalogValue < 10) {
      Serial.print("우클릭: ");
      Serial.println(analogValue);
      Mouse.press(MOUSE_RIGHT);
    }
    if (col2R2State == LOW) { 
      Serial.println("아래로 스크롤");
      Mouse.move(0, 0, 1);
    }
    prevAnalogValue = analogValue;
    delay(50);
    return;
  }
 
  /*
   * 키보드 입력 처리
   */

  // 아날로그 키 처리
  if (analogValue < 10) { }
  // ESC
  else if (analogValue <= ESC_VALUE && prevAnalogValue < 10) { 
    Serial.print("ESC: ");
    Serial.println(analogValue);
    pressButton(ESC_VALUE);
  }
  // TAB
  else if (analogValue <= TAB_VALUE && prevAnalogValue < 10) { 
    Serial.print("TAB: ");
    Serial.println(analogValue);
    pressButton(TAB_VALUE);
  }
  // 모드 변경
  else if (analogValue <= MODE_KEY_VALUE && prevAnalogValue < 10) {
    Serial.print("MODE: ");
    Serial.println(analogValue);
    pressButton(MODE_KEY_VALUE);
  }
  // 백스페이스
  else if (analogValue <= BACKSPACE_VALUE && prevAnalogValue < 10) { 
    Serial.print("BACKSPACE: ");
    Serial.println(analogValue);
    pressButton(BACKSPACE_VALUE);
  }
  // 엔터
  else if (analogValue <= ENTER_VALUE && prevAnalogValue < 10) {
    Serial.print("ENTER: ");
    Serial.println(analogValue);
    pressButton(ENTER_VALUE);
  }
  // 스페이스
  else if (analogValue <= SPACE_VALUE && prevAnalogValue < 10) {
    Serial.print("SPACE: ");
    Serial.println(analogValue);
    pressButton(SPACE_VALUE);
  }
  
  // 기타 기본 키 처리
  
  // 2열 키 처리
  if (col2R1State == LOW && prevCOL2_R1 == HIGH) { pressButton(COL2_R1_PIN); }
  if (col2R2State == LOW && prevCOL2_R2 == HIGH) { pressButton(COL2_R2_PIN); }
  if (col2R3State == LOW && prevCOL2_R3 == HIGH) { pressButton(COL2_R3_PIN); }
  
  // 3열 키 처리
  if (col3R1State == LOW && prevCOL3_R1 == HIGH) { pressButton(COL3_R1_PIN); }
  if (col3R2State == LOW && prevCOL3_R2 == HIGH) { pressButton(COL3_R2_PIN); }
  if (col3R3State == LOW && prevCOL3_R3 == HIGH) { pressButton(COL3_R3_PIN); }
  
  // 4열 키 처리
  if (col4R1State == LOW && prevCOL4_R1 == HIGH) { pressButton(COL4_R1_PIN); }
  if (col4R2State == LOW && prevCOL4_R2 == HIGH) { pressButton(COL4_R2_PIN); }
  if (col4R3State == LOW && prevCOL4_R3 == HIGH) { pressButton(COL4_R3_PIN); }
  
  // 5열 키 처리
  if (jumpState == LOW && prevJUMP == HIGH) { fnPressed = true; }
  if (jumpState == HIGH && prevJUMP == LOW) { 
    // fn 모드에서 사용되지 않았을때만 리셋키로 동작
    if (!fnUsed) {
      resetAndRight();
    }
    fnPressed = false; 
    fnUsed = false;
  }
  if (col5R2State == LOW && prevCOL5_R2 == HIGH) { pressButton(COL5_R2_PIN); }
  // 영문 모드에서는 SHIFT 키로 동작
  if (inputMode == MODE_EN && col5R2State == LOW && prevCOL5_R2 == HIGH) { shiftPressed = true; }
  if (inputMode == MODE_EN && col5R2State == HIGH && prevCOL5_R2 == LOW) { shiftPressed = false; }
  if (dotState == LOW && prevDOT == HIGH) { pressButton(DOT_PIN); }

  // 이전 상태 저장
  prevAnalogValue = analogValue;
  prevCOL2_R1 = col2R1State;
  prevCOL2_R2 = col2R2State;
  prevCOL2_R3 = col2R3State;
  prevCOL3_R1 = col3R1State;
  prevCOL3_R2 = col3R2State;
  prevCOL3_R3 = col3R3State;
  prevCOL4_R1 = col4R1State;
  prevCOL4_R2 = col4R2State;
  prevCOL4_R3 = col4R3State;
  prevJUMP = jumpState;
  prevCOL5_R2 = col5R2State;
  prevDOT = dotState;

  delay(1);
}

void pressButton(int pin) {
  Serial.print("버튼 눌림! : ");
  Serial.println(pin);

  // fn 모드에서 버튼 눌림시 fn으로 사용됨을 체크
  if (fnPressed) {
    // 위 화살표
    if (pin == COL2_R2_PIN) {
      fnUsed = true;
      Keyboard.write(KEY_UP_ARROW);
    // 좌 화살표
    } else if (pin == COL3_R1_PIN) {
      fnUsed = true;
      Keyboard.write(KEY_LEFT_ARROW);
    // 아래 화살표
    } else if (pin == COL3_R2_PIN) {
      fnUsed = true;
      Keyboard.write(KEY_DOWN_ARROW);
    // 우 화살표
    } else if (pin == COL3_R3_PIN) {
      fnUsed = true;
      Keyboard.write(KEY_RIGHT_ARROW);
    // 좌로 블럭지정
    } else if (pin == COL2_R1_PIN) {
      fnUsed = true;
      Keyboard.press(KEY_LEFT_SHIFT);
      Keyboard.write(KEY_LEFT_ARROW);
      Keyboard.release(KEY_LEFT_SHIFT);
    // 우로 블럭지정
    } else if (pin == COL2_R3_PIN) {
      fnUsed = true;
      Keyboard.press(KEY_LEFT_SHIFT);
      Keyboard.write(KEY_RIGHT_ARROW);
      Keyboard.release(KEY_LEFT_SHIFT);
    // undo
    } else if (pin == COL4_R1_PIN) {
      fnUsed = true;
      Keyboard.press(KEY_LEFT_GUI);
      Keyboard.write('z');
      Keyboard.release(KEY_LEFT_GUI);
    // cut
    } else if (pin == COL4_R2_PIN) {
      fnUsed = true;
      Keyboard.press(KEY_LEFT_GUI);
      Keyboard.write('x');
      Keyboard.release(KEY_LEFT_GUI);
    // paste
    } else if (pin == COL4_R3_PIN) {
      fnUsed = true;
      Keyboard.press(KEY_LEFT_GUI);
      Keyboard.write('v');
      Keyboard.release(KEY_LEFT_GUI);
    // option + space
    } else if (pin == COL5_R2_PIN) {
      fnUsed = true;
      Keyboard.press(KEY_LEFT_ALT);
      Keyboard.write(' ');
      Keyboard.release(KEY_LEFT_ALT);
    // select all
    } else if (pin == ESC_VALUE) {
      fnUsed = true;
      Keyboard.press(KEY_LEFT_GUI);
      Keyboard.write('a');
      Keyboard.release(KEY_LEFT_GUI);
    // redo
    } else if (pin == DOT_PIN) {
      fnUsed = true;
      Keyboard.press(KEY_LEFT_GUI);
      Keyboard.press(KEY_LEFT_SHIFT);
      Keyboard.write('z');
      Keyboard.release(KEY_LEFT_GUI);
      Keyboard.release(KEY_LEFT_SHIFT);
    // shift + enter
    } else if (pin == ENTER_VALUE) {
      fnUsed = true;
      Keyboard.press(KEY_LEFT_SHIFT);
      Keyboard.write(KEY_RETURN);
      Keyboard.release(KEY_LEFT_SHIFT);
    // shift + tab
    } else if (pin == TAB_VALUE) {
      fnUsed = true;
      Keyboard.press(KEY_LEFT_SHIFT);
      Keyboard.write(KEY_TAB);
      Keyboard.release(KEY_LEFT_SHIFT);
    // cmd + w
    } else if (pin == MODE_KEY_VALUE) {
      fnUsed = true;
      Keyboard.press(KEY_LEFT_GUI);
      Keyboard.write('w');
      Keyboard.release(KEY_LEFT_GUI);
    // cmd + r
    } else if (pin == BACKSPACE_VALUE) {
      fnUsed = true;
      Keyboard.press(KEY_LEFT_GUI);
      Keyboard.write('r');
      Keyboard.release(KEY_LEFT_GUI);
    // sleep
    } else if (pin == SPACE_VALUE) {
      fnUsed = true;
      Keyboard.press(KEY_LEFT_GUI);
      Keyboard.press(KEY_RIGHT_CTRL);
      Keyboard.write('q');
      Keyboard.release(KEY_LEFT_GUI);
      Keyboard.release(KEY_RIGHT_CTRL);
    }
    

  // fn 모드가 아닐때
  } else {

    // .,?!
    if(pin == DOT_PIN) {
      // .
      if (lastPressed1 != DOT_PIN) {
        Keyboard.write('.');
      // . => ,
      } else if (lastPressed2 != DOT_PIN) {
        Keyboard.write(KEY_BACKSPACE);
        Keyboard.write(',');
      // . => , => ?
      } else if (lastPressed3 != DOT_PIN) {
        Keyboard.write(KEY_BACKSPACE);
        Keyboard.press(KEY_LEFT_SHIFT);
        Keyboard.write('/');
        Keyboard.release(KEY_LEFT_SHIFT);
      // . => , => ? => !
      } else if (lastPressed4 != DOT_PIN) {
        Keyboard.write(KEY_BACKSPACE);
        Keyboard.press(KEY_LEFT_SHIFT);
        Keyboard.write('1');
        Keyboard.release(KEY_LEFT_SHIFT);
      // . => , => ? => ! => .
      } else if (lastPressed5 != DOT_PIN) {
        Keyboard.write(KEY_BACKSPACE);
        Keyboard.write('.');
      // . => , => ? => ! => . => ,
      } else if (lastPressed6 != DOT_PIN) {
        Keyboard.write(KEY_BACKSPACE);
        Keyboard.write(',');
      // . => , => ? => ! => . => , => ?
      } else {
        Keyboard.write(KEY_BACKSPACE);
        Keyboard.press(KEY_LEFT_SHIFT);
        Keyboard.write('/');
        Keyboard.release(KEY_LEFT_SHIFT);
      }
    }

    // ESC
    if (pin == ESC_VALUE) {
      Keyboard.write(KEY_ESC);
      setLang();
      reset();
    }
    // TAB
    else if (pin == TAB_VALUE) {
      Keyboard.write(KEY_TAB);
      reset();
    }
    // 모드 변경
    else if (pin == MODE_KEY_VALUE) {
      nextInputMode();
      reset();
    }
    // 백스페이스
    else if (pin == BACKSPACE_VALUE) {
      Keyboard.write(KEY_BACKSPACE);
      reset();
    }
    // 엔터
    else if (pin == ENTER_VALUE) {
      Keyboard.write(KEY_RETURN);
      reset();
    }
    // 스페이스
    else if (pin == SPACE_VALUE) {
      Keyboard.write(' ');
      reset();
    }

    // 모드별 처리
    switch (inputMode) {
      case MODE_KO:
        handleKoreanInput(pin);
        break;
      case MODE_EN:
        handleEnglishInput(pin);
        break;
      case MODE_NUM:
        handleNumberInput(pin);
        break;
      case MODE_SPEC:
        handleSpecialInput(pin);
        break;
    }
  }

  lastPressed6 = lastPressed5;
  lastPressed5 = lastPressed4;
  lastPressed4 = lastPressed3;
  lastPressed3 = lastPressed2;
  lastPressed2 = lastPressed1;
  lastPressed1 = pin;
  lastButtonPress = millis();
}

void handleKoreanInput(int pin) {
  int ei = COL2_R1_PIN;
  int dot = COL2_R2_PIN;
  int ue = COL2_R3_PIN;

  // 안 -> 않 으로 넘어갈때 ㅇ이 먼저쳐지면 안되기 때문에 해당상황을 위한 대기키
  // 기다리는 키가 한번 더 눌리지 않았다면 바로 먼저 입력
  if (waitingKey != pin) {
    Keyboard.write(pinToKorFirstKey[waitingKey]);
    waitingKey = 0;
  }

  // 2열 1행 - ㅣ
  if (pin == ei) {
    // ㅡ . . ㅣ ㅣ => ㅞ
    if (lastPressed4 == ue && lastPressed3 == dot && lastPressed2 == dot && lastPressed1 == ei) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('p');
    }
    // . ㅡ ㅣ . ㅣ => ㅙ
    else if (lastPressed4 == dot && lastPressed3 == ue && lastPressed2 == ei && lastPressed1 == dot) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('o');
    }
    // ㅡ . . ㅣ => ㅝ
    else if (lastPressed3 == ue && lastPressed2 == dot && lastPressed1 == dot) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('n');
      Keyboard.write('j');
    }
    // ㅡ . ㅣ => ㅟ
    else if (lastPressed2 == ue && lastPressed1 == dot) {
      Keyboard.write('l');
    }
    // . ㅡ ㅣ => ㅚ
    else if (lastPressed2 == dot && lastPressed1 == ue) {
      Keyboard.write('l');
    }
    // . . ㅣ ㅣ => ㅖ
    else if (lastPressed3 == dot && lastPressed2 == dot && lastPressed1 == ei) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('P');
    }
    // ㅣ . . ㅣ => ㅒ
    else if (lastPressed3 == ei && lastPressed2 == dot && lastPressed1 == dot) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('O');
    }
    // . ㅣ ㅣ => ㅔ
    else if (lastPressed2 == dot && lastPressed1 == ei) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('p');
    }
    // ㅣ . ㅣ => ㅐ
    else if (lastPressed2 == ei && lastPressed1 == dot) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('o');
    }
    // . . ㅣ => ㅕ
    else if (lastPressed1 == dot && lastPressed2 == dot) {
      Keyboard.write('u');
    }
    // . ㅣ => ㅓ
    else if (lastPressed1 == dot) {
      Keyboard.write('j');
    }
    // ㅡ ㅣ => ㅢ
    else if (lastPressed1 == ue) {
      Keyboard.write('l');
    }
    // ㅣ
    else {
      Keyboard.write('l');
    }
  }
  // 2열 2행 - 점
  else if (pin == dot) { 
    // ㅣ . => ㅏ (그전에 점을 치지않았을때 ㅏ로 (ㅓ나 ㅕ나ㅐ이후가 아님))
    if (lastPressed1 == ei && lastPressed2 != dot) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('k');
    // ㅣ . . => ㅑ
    } else if (lastPressed1 == dot && lastPressed2 == ei) {
        Keyboard.write(KEY_BACKSPACE);
        Keyboard.write('i');
    // ㅡ . => ㅜ (그전에 점을 치지 않았을때 ㅜ로 (ㅗ나 ㅛ이후가 아님))
    } else if (lastPressed1 == ue && lastPressed2 != dot) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('n');
    // ㅡ . . => ㅠ
    } else if (lastPressed1 == dot && lastPressed2 == ue) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('b');
    }
  }
  // 2열 3행 - ㅡ
  else if (pin == ue) {
    // . . ㅡ => ㅛ
    if (lastPressed1 == dot && lastPressed2 == dot) {
      Keyboard.write('y');
    }
    // . ㅡ => ㅗ
    else if (lastPressed1 == dot) {
      Keyboard.write('h');
    }
    // ㅡ
    else {
      Keyboard.write('m');
    }
  }
  // 3열 1행 - ㄱㅋㄲ
  else if (pin == COL3_R1_PIN) {
    if (lastPressed1 == COL3_R1_PIN) {
      if (lastPressed2 == COL3_R1_PIN) {
        Keyboard.write(KEY_BACKSPACE);
        Keyboard.write('R');
      } else {
        Keyboard.write(KEY_BACKSPACE);
        Keyboard.write('z');
      }
    } else {
      Keyboard.write('r');
    } 
  }
  // 3열 2행 - ㄴㄹ
  else if (pin == COL3_R2_PIN) {
    if (lastPressed1 == COL3_R2_PIN) {
      // 세번 누르면 처음으로
      if (lastPressed2 == COL3_R2_PIN && lastPressed3 != COL3_R2_PIN) {
        Keyboard.write(KEY_BACKSPACE);
        Keyboard.write('s');
      // ㄴ에서 ㄹ로
      } else {
        Keyboard.write(KEY_BACKSPACE);
        Keyboard.write('f');
      }
    } else {
      Keyboard.write('s');
    } 
  }
  // 3열 3행 - ㄷㅌㄸ
  else if (pin == COL3_R3_PIN) {
    // 할 => 핥 으로 넘어가기위해 이전에 ㄷ이 대기중이라면 ㅌ입력
    if (waitingKey == COL3_R3_PIN) {
      Keyboard.write('x');
      waitingKey = 0;
    // 할 => 핥 으로 넘어가기위해 ㄹ다음에 ㄷ입력시 waitingKey로 설정
    } else if (lastPressed1 == COL3_R2_PIN && lastPressed2 == COL3_R2_PIN) {
      waitingKey = COL3_R3_PIN;
    } else if (lastPressed1 == COL3_R3_PIN) {
      if (lastPressed2 == COL3_R3_PIN) {
        Keyboard.write(KEY_BACKSPACE);
        Keyboard.write('E');
      } else {
        Keyboard.write(KEY_BACKSPACE);
        Keyboard.write('x');
      }
    } else {
      Keyboard.write('e');
    } 
  }
  // 4열 1행 - ㅂㅍㅃ
  else if (pin == COL4_R1_PIN) {
    if (lastPressed1 == COL4_R1_PIN) {
      if (lastPressed2 == COL4_R1_PIN) {
        Keyboard.write(KEY_BACKSPACE);
        Keyboard.write('Q');
      } else {
        Keyboard.write(KEY_BACKSPACE);
        Keyboard.write('v');
      }
    } else {
      Keyboard.write('q');
    } 
  }
  // 4열 2행 - ㅅㅎ
  else if (pin == COL4_R2_PIN) {
    // 안 -> 않 으로 넘어갈때 ㅇ이 대기중이라면 바로 ㅎ입력
    if (waitingKey == COL4_R2_PIN) {
      Keyboard.write('g');
      waitingKey = 0;
    // 안 -> 않 으로 넘어가기위해 ㄴ다음에 ㅇ입력시 waitingKey로 설정
    } else if (lastPressed1 == COL3_R2_PIN && lastPressed2 != COL3_R2_PIN) {
      waitingKey = COL4_R2_PIN;
    } else if (lastPressed1 == COL4_R2_PIN) {
      // 세번 누르면 처음으로
      if (lastPressed2 == COL4_R2_PIN) {
        Keyboard.write(KEY_BACKSPACE);
        Keyboard.write('T');
      // ㅇ에서 ㅁ로
      } else {
        Keyboard.write(KEY_BACKSPACE);
        Keyboard.write('g');
      }
    } else {
      Keyboard.write('t');
    } 
  }
  // 4열 3행 - ㅈㅊ
  else if (pin == COL4_R3_PIN) {
    if (lastPressed1 == COL4_R3_PIN) {
      if (lastPressed2 == COL4_R3_PIN) {
        Keyboard.write(KEY_BACKSPACE);
        Keyboard.write('W');
      } else {
        Keyboard.write(KEY_BACKSPACE);
        Keyboard.write('c');
      }
    } else {
      Keyboard.write('w');
    } 
  }
  // 5열 2행 - ㅇㅁ
  else if (pin == COL5_R2_PIN) {
    // 살 -> 삶 으로 넘어갈때 ㅇ이 대기중이라면 바로 ㅁ입력
    if (waitingKey == COL5_R2_PIN) {
      Keyboard.write('a');
      waitingKey = 0;
    // 살 -> 삶 으로 넘어가기위해 ㄹ다음에 ㅇ입력시 waitingKey로 설정
    } else if (lastPressed1 == COL3_R2_PIN && lastPressed2 == COL3_R2_PIN) {
      waitingKey = COL5_R2_PIN;
    } else if (lastPressed1 == COL5_R2_PIN) {
      // 세번 누르면 처음으로
      if (lastPressed2 == COL5_R2_PIN && lastPressed3 != COL5_R2_PIN) {
        Keyboard.write(KEY_BACKSPACE);
        Keyboard.write('d');
      // ㅇ에서 ㅁ로
      } else {
        Keyboard.write(KEY_BACKSPACE);
        Keyboard.write('a');
      }
    } else {
      Keyboard.write('d');
    } 
  }
}

void handleEnglishInput(int pin) {
  if (shiftPressed) {
    Keyboard.press(KEY_LEFT_SHIFT);
  }

  // 2열 키 처리
  // qwer
  if (pin == COL2_R1_PIN) { 
    // q
    if (lastPressed1 != COL2_R1_PIN) {
      Keyboard.write('q');
    // q => w
    } else if (lastPressed2 != COL2_R1_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('w');
    // q => w => e
    } else if (lastPressed3 != COL2_R1_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('e');
    // q => w => e => r
    } else if (lastPressed4 != COL2_R1_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('r');
    // q => w => e => r => q
    } else if (lastPressed5 != COL2_R1_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('q');
    // q => w => e => r => q => w
    } else if (lastPressed6 != COL2_R1_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('w');
    // q => w => e => r => q => w => e
    } else {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('e');
    }
  // tyu
  } else if (pin == COL2_R2_PIN) {
    // t
    if (lastPressed1 != COL2_R2_PIN) {
      Keyboard.write('t');
    // t => y
    } else if (lastPressed2 != COL2_R2_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('y');
    // t => y => u
    } else if (lastPressed3 != COL2_R2_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('u');
    // t => y => u => t
    } else if (lastPressed4 != COL2_R2_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('t');
    // t => y => u => t => y
    } else if (lastPressed5 != COL2_R2_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('y');
    // t => y => u => t => y => u
    } else if (lastPressed6 != COL2_R2_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('u');
    // t => y => u => t => y => u => t
    } else {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('t');
    }
  // iop
  } else if (pin == COL2_R3_PIN) {
    // i
    if (lastPressed1 != COL2_R3_PIN) {
      Keyboard.write('i');
    // i => o
    } else if (lastPressed2 != COL2_R3_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('o');
    // i => o => p
    } else if (lastPressed3 != COL2_R3_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('p');
    // i => o => p => i
    } else if (lastPressed4 != COL2_R3_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('i');
    // i => o => p => i => o
    } else if (lastPressed5 != COL2_R3_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('o');
    // i => o => p => i => o => p
    } else if (lastPressed6 != COL2_R3_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('p');
    // i => o => p => i => o => p => i
    } else {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('i');
    }
  }
  // 3열 키 처리 
  // asdf
  else if (pin == COL3_R1_PIN) { 
    // a
    if (lastPressed1 != COL3_R1_PIN) {
      Keyboard.write('a');
    // a => s
    } else if (lastPressed2 != COL3_R1_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('s');
    // a => s => d
    } else if (lastPressed3 != COL3_R1_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('d');
    // a => s => d => f
    } else if (lastPressed4 != COL3_R1_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('f');
    // a => s => d => f => a
    } else if (lastPressed5 != COL3_R1_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('a');
    // a => s => d => f => a => s
    } else if (lastPressed6 != COL3_R1_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('s');
    // a => s => d => f => a => s => d
    } else {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('d');
    }
  // gh
  } else if (pin == COL3_R2_PIN) { 
    // g
    if (lastPressed1 != COL3_R2_PIN) {
      Keyboard.write('g');
    // g => h
    } else if (lastPressed2 != COL3_R2_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('h');
    // g => h => g
    } else if (lastPressed3 != COL3_R2_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('g');
    // g => h => g => h => g
    } else if (lastPressed4 != COL3_R2_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('g');
    // g => h => g => h => g => h
    } else if (lastPressed5 != COL3_R2_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('g');
    // g => h => g => h => g => h => g
    } else if (lastPressed6 != COL3_R2_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('g');
    // g => h => g => h => g => h => g => h
    } else {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('h');
    }
  // jkl
  } else if (pin == COL3_R3_PIN) { 
    // j
    if (lastPressed1 != COL3_R3_PIN) {
      Keyboard.write('j');
    // j => k
    } else if (lastPressed2 != COL3_R3_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('k');
    // j => k => l
    } else if (lastPressed3 != COL3_R3_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('l');
    // j => k => l => j 
    } else if (lastPressed4 != COL3_R3_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('j');
    // j => k => l => j => k
    } else if (lastPressed5 != COL3_R3_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('k');
    // j => k => l => j => k => l
    } else if (lastPressed6 != COL3_R3_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('l');
    // j => k => l => j => k => l => j
    } else {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('j');
    }
  }
  // 4열 키 처리
  // zxcv
  else if (pin == COL4_R1_PIN) { 
    // z
    if (lastPressed1 != COL4_R1_PIN) {
      Keyboard.write('z');
    // z => x
    } else if (lastPressed2 != COL4_R1_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('x');
    // z => x => c
    } else if (lastPressed3 != COL4_R1_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('c');
    // z => x => c => v
    } else if (lastPressed4 != COL4_R1_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('v');
    // z => x => c => v => z
    } else if (lastPressed5 != COL4_R1_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('z');
    // z => x => c => v => z => x
    } else if (lastPressed6 != COL4_R1_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('x');
    // z => x => c => v => z => x => c
    } else {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('c');
    }
  // b
  } else if (pin == COL4_R2_PIN) { 
    Keyboard.write('b');

  // nm
  } else if (pin == COL4_R3_PIN) { 
    // n
    if (lastPressed1 != COL4_R3_PIN) {
      Keyboard.write('n');
    // n => m
    } else if (lastPressed2 != COL4_R3_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('m');
    // n => m => n
    } else if (lastPressed3 != COL4_R3_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('n');
    // n => m => n => m
    } else if (lastPressed4 != COL4_R3_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('m');
    // n => m => n => m => n
    } else if (lastPressed5 != COL4_R3_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('n');
    // n => m => n => m => n => m
    } else if (lastPressed6 != COL4_R3_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('m');
    // n => m => n => m => n => m => n
    } else {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('n');
    }
  }

  if (shiftPressed) {
    Keyboard.release(KEY_LEFT_SHIFT);
  }
}

void handleNumberInput(int pin) {
  // 2열 숫자 처리 (1, 2, 3)
  if (pin == COL2_R1_PIN) { Keyboard.write('7'); }
  else if (pin == COL2_R2_PIN) { Keyboard.write('8'); }
  else if (pin == COL2_R3_PIN) { Keyboard.write('9'); }
  // 3열 숫자 처리 (4, 5, 6)
  else if (pin == COL3_R1_PIN) { Keyboard.write('4'); }
  else if (pin == COL3_R2_PIN) { Keyboard.write('5'); }
  else if (pin == COL3_R3_PIN) { Keyboard.write('6'); }
  // 4열 숫자 처리 (7, 8, 9)
  else if (pin == COL4_R1_PIN) { Keyboard.write('1'); }
  else if (pin == COL4_R2_PIN) { Keyboard.write('2'); }
  else if (pin == COL4_R3_PIN) { Keyboard.write('3'); }
  // 5열 숫자 처리 (0)
  else if (pin == COL5_R2_PIN) { Keyboard.write('0'); }

  // 숫자에서 jump키를 대쉬로 사용
  else if (pin == JUMP_PIN) { Keyboard.write('-'); }
}

void handleSpecialInput(int pin) {
  // -~_
  if (pin == COL2_R1_PIN) {
    // -
    if (lastPressed1 != COL2_R1_PIN) {
      Keyboard.write('-');
    // - => ~
    } else if (lastPressed2 != COL2_R1_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('~');
    // - => ~ => _
    } else if (lastPressed3 != COL2_R1_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('_');
    // - => ~ => _ => -
    } else if (lastPressed4 != COL2_R1_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('-');
    // - => ~ => _ => - => ~
    } else if (lastPressed5 != COL2_R1_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('~');
    // - => ~ => _ => - => ~ => _
    } else if (lastPressed6 != COL2_R1_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('~');
    // - => ~ => _ => - => ~ => _ => -
    } else {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('-');
    }
  }
  // +*=
  else if (pin == COL2_R2_PIN) {
    // +
    if (lastPressed1 != COL2_R2_PIN) {
      Keyboard.write('+');
    } 
    // + => *
    else if (lastPressed2 != COL2_R2_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('*');
    } 
    // + => * => =
    else if (lastPressed3 != COL2_R2_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('=');
    }
    // + => * => = => +
    else if (lastPressed4 != COL2_R2_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('+');
    }
    // + => * => = => + => *
    else if (lastPressed5 != COL2_R2_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('*');
    }
    // + => * => = => + => * => + 
    else if (lastPressed6 != COL2_R2_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('*');
    }
    // + => * => = => + => * => + => *
    else {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('+');
    }
  }
  // /|"\"
  else if (pin == COL2_R3_PIN) {
    // /
    if (lastPressed1 != COL2_R3_PIN) {
      Keyboard.write('/');
    // / => |
    } else if (lastPressed2 != COL2_R3_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('|');
    // / => | => "\"
    } else if (lastPressed3 != COL2_R3_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('\\');
    // / => | => "\" => /
    } else if (lastPressed4 != COL2_R3_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('/');
    // / => | => "\" => / => |
    } else if (lastPressed5 != COL2_R3_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('|');
    // / => | => "\" => / => | => "\"
    } else if (lastPressed6 != COL2_R3_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('\\');
    // / => | => "\" => / => | => "\" => /
    } else {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('/');
    }
  }
  // (<
  else if (pin == COL3_R1_PIN) {
    // (
    if (lastPressed1 != COL3_R1_PIN) {
      Keyboard.write('(');
    // ( => <
    } else if (lastPressed2 != COL3_R1_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('<');
    // ( => < => (
    } else if (lastPressed3 != COL3_R1_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('(');
    // ( => < => ( => <
    } else if (lastPressed4 != COL3_R1_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('<');
    // ( => < => ( => < => (
    } else if (lastPressed5 != COL3_R1_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('<');
    // ( => < => ( => < => ( => <
    } else if (lastPressed6 != COL3_R1_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('(');
    } else {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('(');
    }
  }
  // )>
  else if (pin == COL3_R2_PIN) {
    // )
    if (lastPressed1 != COL3_R2_PIN) {
      Keyboard.write(')');
    // ) => >
    } else if (lastPressed2 != COL3_R2_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('>');
    // ) => > => )
    } else if (lastPressed3 != COL3_R2_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write(')');
    // ) => > => ) => >
    } else if (lastPressed4 != COL3_R2_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('>');
    // ) => > => ) => > => )
    } else if (lastPressed5 != COL3_R2_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write(')');
    // ) => > => ) => > => ) => >
    } else if (lastPressed6 != COL3_R2_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('>');
    // ) => > => ) => > => ) => > => )
    } else {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write(')');
    }
  }
  // '"`
  else if (pin == COL3_R3_PIN) {
    // '
    if (lastPressed1 != COL3_R3_PIN) {
      Keyboard.write('\'');
    // ' => "
    } else if (lastPressed2 != COL3_R3_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('"');
    // ' => " => `
    } else if (lastPressed3 != COL3_R3_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('`');
    // ' => " => ` => '
    } else if (lastPressed4 != COL3_R3_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('\'');
    // ' => " => ` => ' => "
    } else if (lastPressed5 != COL3_R3_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('"');
    // ' => " => ` => ' => " => `
    } else if (lastPressed6 != COL3_R3_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('`');
    // ' => " => ` => ' => " => ` => '
    } else {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('\'');
    }
  }
  // [{
  else if (pin == COL4_R1_PIN) {
    // [
    if (lastPressed1 != COL4_R1_PIN) {
      Keyboard.write('[');
    // [ => {
    } else if (lastPressed2 != COL4_R1_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('{');
    // [ => { => [
    } else if (lastPressed3 != COL4_R1_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('{');
    // [ => { => [ => {
    } else if (lastPressed4 != COL4_R1_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('[');
    // [ => { => [ => { => [
    } else if (lastPressed5 != COL4_R1_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('{');
    // [ => { => [ => { => [ => {
    } else if (lastPressed6 != COL4_R1_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('[');
    // [ => { => [ => { => [ => { => [
    } else {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('[');
    }
  }
  // ]}
  else if (pin == COL4_R2_PIN) {
    // ]
    if (lastPressed1 != COL4_R2_PIN) {
      Keyboard.write(']');
    }
    // ] => }
    else if (lastPressed2 != COL4_R2_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('}');
    // ] => } => ]
    } else if (lastPressed3 != COL4_R2_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('}');
    // ] => } => ] => }
    } else if (lastPressed4 != COL4_R2_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('}');
    }
    // ] => } => ] => } => ]
    else if (lastPressed5 != COL4_R2_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('}');
    }
    // ] => } => ] => } => ] => }
    else if (lastPressed6 != COL4_R2_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('}');
    } else {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('}');
    }
  }
  // :;
  else if (pin == COL4_R3_PIN) {
    // :
    if (lastPressed1 != COL4_R3_PIN) {
      Keyboard.write(':');
    }
    // : => ;
    else if (lastPressed2 != COL4_R3_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write(';');
    // : => ; => :
    } else if (lastPressed3 != COL4_R3_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write(':');
    }
    // : => ; => : => ;
    else if (lastPressed4 != COL4_R3_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write(';');
    }
    // : => ; => : => ; => :
    else if (lastPressed5 != COL4_R3_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write(';');
    }
    // : => ; => : => ; => : => ;
    else if (lastPressed6 != COL4_R3_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write(';');
    } else {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write(':');
    }
  }
  // @#$%&
  else if (pin == COL5_R2_PIN) {
    // @
    if (lastPressed1 != COL5_R2_PIN) {
      Keyboard.write('@');
    }
    // @ => #
    else if (lastPressed2 != COL5_R2_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('#');
    }
    // @ => # => $
    else if (lastPressed3 != COL5_R2_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('$');
    }
    // @ => # => $ => %
    else if (lastPressed4 != COL5_R2_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('%');
    }
    // @ => # => $ => % => &
    else if (lastPressed5 != COL5_R2_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('&');
    }
    // @ => # => $ => % => & => @
    else if (lastPressed6 != COL5_R2_PIN) {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('@');
    // @ => # => $ => % => & => @ => #
    } else {
      Keyboard.write(KEY_BACKSPACE);
      Keyboard.write('#');
    }
  }
}

void reset() {
  lastPressed6 = 0;
  lastPressed5 = 0;
  lastPressed4 = 0;
  lastPressed3 = 0;
  lastPressed2 = 0;
  lastPressed1 = 0;
}

void resetAndRight() {
  if (waitingKey != 0) {
    Keyboard.write(pinToKorFirstKey[waitingKey]);
    waitingKey = 0;
  }
  reset();
  // 한글모드인 경우만 우화살표로 조합형 정리
  if(inputMode == MODE_KO) {
    Keyboard.write(KEY_RIGHT_ARROW);
  }
}

void setModeLED(int mode) {
  // 10 = 한글, 01 = 영문, 00 = 숫자, 11 = 특수
  switch (mode) {
    case MODE_KO:
      digitalWrite(LED1, HIGH);
      digitalWrite(LED2, LOW);
      break;
    case MODE_EN:
      digitalWrite(LED1, LOW);
      digitalWrite(LED2, HIGH);
      break;
    case MODE_NUM:
      digitalWrite(LED1, HIGH);
      digitalWrite(LED2, HIGH);
      break;
    case MODE_SPEC:
      digitalWrite(LED1, LOW);
      digitalWrite(LED2, LOW);
      break;
  }
}

void nextInputMode() {
  // 한글 => 영문
  if (inputMode == MODE_KO) {
    inputMode = MODE_EN;
    setEnLang();
  // 영문 => 숫자
  } else if (inputMode == MODE_EN) {
    inputMode = MODE_NUM;
  // 숫자 => 특수
  } else if (inputMode == MODE_NUM) {
    inputMode = MODE_SPEC;
  // 특수 => 한글
  } else if (inputMode == MODE_SPEC) {
    inputMode = MODE_KO;
    setKoLang();
  }
  Serial.print("모드교체: ");
  Serial.println(inputMode);
  setModeLED(inputMode);
}

// Karabiner 설정에서 오른쪽 컨트롤 누르면 영문 모드로 설정
void setEnLang() {
  Serial.println("영어로 언어설정");
  Keyboard.write(KEY_RIGHT_CTRL);
}
// Karabiner 설정 JSON 예시
/**
{
  "description": "right_control => en",
    "manipulators": [
        {
            "from": { "key_code": "right_control" },
            "to": [{ "select_input_source": { "language": "^en$" } }],
            "type": "basic"
        }
    ]
}
 */


// 영문으로 설정후 언어 변경해 한국어 설정
void setKoLang() {
  Serial.println("한글로 언어설정");
  Keyboard.write(KEY_RIGHT_CTRL);
  delay(100);
  Keyboard.press(KEY_RIGHT_GUI);
  Keyboard.write(' ');
  Keyboard.release(KEY_RIGHT_GUI);
}

void setLang() {
  if (inputMode == MODE_KO) {
    setKoLang();
  } else if (inputMode == MODE_EN) {
    setEnLang();
  }
}
